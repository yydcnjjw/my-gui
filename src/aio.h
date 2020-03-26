#pragma once

#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/thread/future.hpp>

#include <filesystem>
#include <fstream>
namespace my {
namespace fs = std::filesystem;
template <typename T> using future = boost::future<T>;
template <typename T> using promise = boost::promise<T>;
namespace aio {

class AIOThreadPool {
  public:
    AIOThreadPool() : _pool(4) {}
    ~AIOThreadPool() { this->_pool.join(); }
    static boost::asio::thread_pool &get();

  private:
    boost::asio::thread_pool _pool;
};

template <typename T>
using async_callback = std::function<void(std::shared_ptr<promise<T>>)>;

template <typename T, typename Callback>
future<T> do_async(Callback &&callback) {
    auto prom = std::make_shared<promise<T>>();
    future<T> future = prom->get_future();
    boost::asio::post(AIOThreadPool::get(), std::bind(callback, prom));

    return future;
}

future<std::string> file_read_all(const fs::path &);
// future<std::string> wirte();

} // namespace aio
} // namespace my
