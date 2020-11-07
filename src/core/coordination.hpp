#pragma once

#include <core/config.hpp>
#include <core/type.hpp>

namespace my {

struct thread_info {
    typedef std::thread::native_handle_type handle_type;
    typedef std::thread::id thread_id_type;
    thread_info(std::thread &thread)
        : handle(thread.native_handle()), thread_id(thread.get_id()) {}
    handle_type handle;
    thread_id_type thread_id;
};

class observe_on_one_thread {
  public:
    using coordination_type = rx::observe_on_one_worker;
    using coordinator_type = coordination_type::coordinator_type;
    using coordinator_state_type = rx::composite_subscription;
    using thread_info_type = thread_info;

    observe_on_one_thread(shared_ptr<promise<thread_info_type>> prom =
                              std::make_shared<promise<thread_info_type>>())
        : _thread_info_f(prom->get_future()),
          _coordinator(observe_on_one_thread::create_coordinator(
              prom, this->_coordinator_state)) {}
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

    thread_info_type const &get_thread_info() {
        if (!this->_thread_info) {
            this->_thread_info = this->_thread_info_f.get();
        }
        return *this->_thread_info;
    }

  private:
    coordinator_state_type _coordinator_state;
    future<thread_info_type> _thread_info_f;
    std::optional<thread_info_type> _thread_info;
    coordinator_type _coordinator;

    static coordinator_type
    create_coordinator(shared_ptr<promise<thread_info_type>> prom,
                       coordinator_state_type &s) {
        return rx::observe_on_one_worker(
                   rx::rxsc::make_new_thread([prom](std::function<void()> f) {
                       auto thread = std::thread(
                           [](std::function<void()> f) { f(); }, std::move(f));
                       prom->set_value(thread_info_type(thread));
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
