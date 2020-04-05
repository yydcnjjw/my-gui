#pragma once

#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/thread/future.hpp>
#include <iostream>

namespace my {

template <class Fun> class y_combinator_result {
    Fun fun_;

  public:
    explicit y_combinator_result(Fun &&fun) : fun_(std::forward<Fun>(fun)) {}

    template <class... Args> decltype(auto) operator()(Args &&... args) {
        return fun_(std::ref(*this), std::forward<Args>(args)...);
    }
};

template <class Fun> decltype(auto) y_combinator(Fun &&fun) {
    return y_combinator_result<Fun>(std::forward<Fun>(fun));
}

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

    template <typename Callback> class Timer {
      public:
        Timer(boost::asio::thread_pool &pool, Callback &&callback,
              std::chrono::milliseconds interval)
            : _timer(pool), _callback(std::forward<Callback>(callback)),
              _interval(interval) {}

        void set_interval(const std::chrono::milliseconds &interval) {
            this->_interval = interval;
        }

        const std::chrono::milliseconds &get_interval() {
            return this->_interval;
        }

        void start() {
            this->_timer.expires_after(this->_interval);
            this->_cancel = false;
            this->_timer.async_wait(boost::bind<void>(
                on_timer, boost::asio::placeholders::error, this));
        }

        void cancel() {
            this->_cancel = true;
            this->_timer.cancel();
        }

      private:
        boost::asio::steady_timer _timer;
        Callback _callback;
        std::chrono::milliseconds _interval;
        bool _cancel = false;

        static void on_timer(const boost::system::error_code &,
                             Timer<Callback> *timer) {
            if (!timer->_cancel) {
                timer->_callback();
                timer->_timer.expires_at(timer->_timer.expiry() +
                                         timer->_interval);
                timer->_timer.async_wait(boost::bind<void>(
                    on_timer, boost::asio::placeholders::error, timer));
            }
        }
    };

    template <typename Callback>
    std::shared_ptr<Timer<Callback>>
    create_timer_interval(Callback &&callback,
                          const std::chrono::milliseconds &interval) {
        return std::make_shared<Timer<Callback>>(
            this->_pool, std::forward<Callback>(callback), interval);
    }

    static std::unique_ptr<AsyncTask> create();

  private:
    boost::asio::thread_pool _pool;
};

} // namespace my
