#pragma once

#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/thread/future.hpp>

namespace my {

template <typename T> using future = boost::future<T>;
template <typename T> using promise = boost::promise<T>;

template <typename T>
using async_callback = std::function<void(std::shared_ptr<promise<T>>)>;

class AsyncTask {
  public:
    AsyncTask() : _pool(4) {}    
    ~AsyncTask() { this->_pool.join(); };

    template <typename T, typename Callback>
    future<T> do_async(Callback &&callback) {
        auto prom = std::make_shared<promise<T>>();
        future<T> future = prom->get_future();
        boost::asio::post(this->_pool, std::bind(callback, prom));
        return future;
    }

    static std::unique_ptr<AsyncTask> create();

  private:
    boost::asio::thread_pool _pool;
};

} // namespace my
