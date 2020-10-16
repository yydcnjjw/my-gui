#pragma once

#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>

#include <core/config.hpp>

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

template <typename T>
using async_callback = std::function<void(std::shared_ptr<promise<T>>)>;

class AsyncTask {
  public:
    AsyncTask() : _pool(2) {}
    ~AsyncTask() {
        this->_pool.stop();
        this->_pool.join();
    };

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
            this->_is_enable = true;
            this->_timer.async_wait(boost::bind<void>(
                on_timer, boost::asio::placeholders::error, this));
        }

        bool is_enable() { return this->_is_enable; }
        bool is_cancel() { return !this->_is_enable; }

        void cancel() {
            if (this->is_cancel()) {
                return;
            }
            this->_is_enable = false;
            this->_timer.cancel();
        }
        void wait() { this->_timer.wait(); }

      private:
        boost::asio::steady_timer _timer;
        Callback _callback;
        std::chrono::milliseconds _interval;
        std::atomic_bool _is_enable{false};

        static void on_timer(const boost::system::error_code &ec,
                             Timer<Callback> *timer) {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }
            if (timer->is_enable()) {
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

    template <typename Callback,
              typename = std::enable_if_t<std::is_invocable<
                  Callback, boost::system::error_code &,
                  std::shared_ptr<boost::asio::steady_timer>>::value>>
    auto do_timer_for(Callback &&callback,
                      const std::chrono::milliseconds &time) {
        auto timer =
            std::make_shared<boost::asio::steady_timer>(this->_pool, time);
        timer->async_wait(boost::bind<void>(
            callback, boost::asio::placeholders::error, timer));
        return timer;
    }

    static std::unique_ptr<AsyncTask> create() {
        return std::make_unique<AsyncTask>();
    }

  private:
    boost::asio::thread_pool _pool;
};

} // namespace my
