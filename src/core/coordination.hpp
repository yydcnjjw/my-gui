#pragma once

#include <core/config.hpp>

namespace my {
class observe_on_one_thread {
  public:
    typedef std::thread::native_handle_type thread_type;
    typedef rx::observe_on_one_worker coordination_type;
    typedef coordination_type::coordinator_type coordinator_type;
    typedef rx::composite_subscription coordinator_state_type;

    observe_on_one_thread()
        : _coordinator(observe_on_one_thread::create_coordinator(
              this->_thread, this->_coordinator_state)) {}
    ~observe_on_one_thread() {
        // exit thread
        this->coordinator_state().unsubscribe();
    }

    coordination_type get() {
        return rx::observe_on_one_worker(this->_coordinator.get_scheduler());
    }

    coordinator_state_type &coordinator_state() {
        return this->_coordinator_state;
    }
    coordinator_type &coordinator() { return this->_coordinator; }

    thread_type &thread() { return this->_thread; }

  private:
    thread_type _thread;
    coordinator_state_type _coordinator_state;
    coordinator_type _coordinator;

    static coordinator_type create_coordinator(thread_type &t,
                                               coordinator_state_type &s) {
        return rx::observe_on_one_worker(
                   rx::rxsc::make_new_thread([&t](std::function<void()> f) {
                       auto thread = std::thread(std::bind(
                           [](std::function<void()> f) { f(); }, std::move(f)));
                       t = thread.native_handle();
                       return thread;
                   }))
            .create_coordinator(s);
    }
};

class main_loop {
    using run_loop_type = rx::schedulers::run_loop;

  public:
    using coordination_type = rx::observe_on_one_worker;
    main_loop() {
        _rl.set_notify_earlier_wakeup([this](auto) { this->_cv.notify_one(); });
    }

    coordination_type coordination() {
        return rx::observe_on_run_loop(this->_rl);
    }

    void quit() {
        this->_need_quit = true;
        this->_cv.notify_one();
    }

    void run() {
        while (!this->_need_quit) {
            while (!this->_need_quit && !this->_rl.empty() &&
                   this->_rl.peek().when < this->_rl.now()) {
                this->_rl.dispatch();
            }

            std::unique_lock<std::mutex> l_lock(this->_lock);
            if (!this->_rl.empty() && this->_rl.peek().when > this->_rl.now()) {
                this->_cv.wait_until(l_lock, this->_rl.peek().when);
            } else {
                this->_cv.wait(l_lock, [this]() {
                    return this->_need_quit || !this->_rl.empty();
                });
            }
        }
    }

  private:
    run_loop_type _rl;
    std::condition_variable _cv;
    std::mutex _lock;
    bool _need_quit{false};
};

} // namespace my
