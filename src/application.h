#pragma once

#include <my_gui.hpp>
// #include <media/audio_mgr.h>
// #include <render/window/window_mgr.h>
// #include <storage/font_mgr.h>
#include <storage/resource_mgr.hpp>
#include <util/async_task.hpp>
#include <util/event_bus.hpp>

namespace my {

class main_loop {
    typedef rxcpp::schedulers::run_loop run_loop_type;

  public:
    typedef rxcpp::observe_on_one_worker coordination_type;
    main_loop() {
        _rl.set_notify_earlier_wakeup([this](auto) {
            GLOG_D("notify main loop");
            this->_cv.notify_one();
        });
    }

    coordination_type coordination() {
        return rxcpp::observe_on_run_loop(this->_rl);
    }

    void quit() {
        this->_need_quit = true;
        this->_cv.notify_one();
    }

    void run() {
        while (!this->_need_quit) {
            GLOG_D("peek task");
            while (!this->_need_quit && !this->_rl.empty() &&
                   this->_rl.peek().when < this->_rl.now()) {
                this->_rl.dispatch();
            }

            std::unique_lock<std::mutex> l_lock(this->_lock);
            if (!this->_rl.empty() && this->_rl.peek().when > this->_rl.now()) {
                GLOG_D("wait util main loop");
                this->_cv.wait_until(l_lock, this->_rl.peek().when);
            } else {
                GLOG_D("wait main loop");
                this->_cv.wait(l_lock, [this](){
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

class Application : public my::EventBus {
  public:
    typedef main_loop::coordination_type coordination_type;
    Application(){};
    virtual ~Application() = default;
    virtual void run() = 0;
    virtual void quit() = 0;

    virtual coordination_type coordination() = 0;

    // virtual WindowMgr *win_mgr() const = 0;
    // virtual AsyncTask *async_task() const = 0;
    // virtual ResourceMgr *resource_mgr() const = 0;

    virtual bool
    get_program_option(const std::string &option,
                       program_options::variable_value &value) const = 0;
    static std::unique_ptr<Application>
    create(int argc, char **argv,
           const program_options::options_description & = {});
};
} // namespace my
