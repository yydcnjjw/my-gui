#pragma once

#include <my_gui.hpp>
// #include <media/audio_mgr.h>
#include <window/service.hpp>
// #include <storage/font_mgr.h>
#include <storage/resource_mgr.hpp>
#include <util/async_task.hpp>
#include <util/event_bus.hpp>

namespace my {

class main_loop {
    using run_loop_type = rxcpp::schedulers::run_loop;

  public:
    using coordination_type = rxcpp::observe_on_one_worker;
    main_loop() {
        _rl.set_notify_earlier_wakeup([this](auto) { this->_cv.notify_one(); });
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

class Application : public EventBus {
  public:
    using coordination_type = main_loop::coordination_type;
    using options_description = po::options_description;
    Application(int argc, char **argv, const options_description &opts_desc) {
        this->_parse_program_options(argc, argv, opts_desc);
    };

    virtual ~Application() = default;
    virtual void run() = 0;
    virtual void quit() = 0;

    virtual coordination_type coordination() = 0;

    // virtual WindowMgr *win_mgr() const = 0;
    // virtual AsyncTask *async_task() const = 0;
    // virtual ResourceMgr *resource_mgr() const = 0;

    bool get_program_option(const std::string &option,
                            po::variable_value &value) const {
        if (this->_program_option_map.count(option)) {
            value = this->_program_option_map[option];
            return true;
        } else {
            return false;
        }
    }

    static std::unique_ptr<Application>
    create(int argc, char **argv, const options_description & = {});

  private:
    options_description _opts_desc;
    po::variables_map _program_option_map;

    void _parse_program_options(int argc, char **argv,
                                const options_description &opts) {
        po::store(po::command_line_parser(argc, argv)
                      .options(opts)
                      .allow_unregistered()
                      .run(),
                  this->_program_option_map);
        po::notify(this->_program_option_map);
    }
};
} // namespace my
