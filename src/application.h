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

class ApplicationError : public std::runtime_error {
  public:
    ApplicationError(const std::string &msg)
        : std::runtime_error(msg), _msg(msg) {}

    const char *what() const noexcept override { return this->_msg.c_str(); }

  private:
    std::string _msg;
};

class Application : public EventBus {
  public:
    using coordination_type = main_loop::coordination_type;
    using options_description = po::options_description;
    Application(int argc, char **argv, const options_description &opts_desc) {
        pthread_setname_np(pthread_self(),
                           "application service");
        this->parse_program_options(argc, argv, opts_desc);
        this->register_service(WindowService::create());
    };

    virtual ~Application() = default;
    void run() {
        on_event<my::QuitEvent>(this)
            .observe_on(this->coordination())
            .subscribe([this](auto) { this->quit(); });

        try {
            this->_main_loop.run();
        } catch (std::exception &e) {
            GLOG_E(e.what());
        }
    };
    void quit() { this->_main_loop.quit(); };

    coordination_type coordination() {
        return this->_main_loop.coordination();
    };

    template <typename Service, typename = std::enable_if_t<
                                    std::is_base_of_v<BasicService, Service>>>
    Service *service() {
        if (this->_services.count(typeid(Service)) == 0) {
            // TODO: More detailed error information
            throw ApplicationError("no service");
        }
        return dynamic_cast<Service *>(
            this->_services.at(typeid(Service)).get());
    }

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
    my::main_loop _main_loop;
    options_description _opts_desc;
    po::variables_map _program_option_map;
    std::map<std::type_index, std::unique_ptr<BasicService>> _services;

    template <typename Service, typename = std::enable_if_t<
                                    std::is_base_of_v<BasicService, Service>>>
    void register_service(std::unique_ptr<Service> service) {
        if (this->_services.count(typeid(Service))) {
            // TODO: More detailed error information
            throw ApplicationError("service duplicate");
        }

        this->dispatch_event(service);
        this->subscribe_event(service);

        this->_services.emplace(typeid(Service), std::move(service));
    }

    template <typename _Observer>
    void dispatch_event(
        const std::unique_ptr<_Observer> &observer,
        typename std::enable_if_t<std::is_base_of_v<Observer, _Observer>> * =
            0) {
        observer->subscribe(this);
    }

    template <typename _Observer>
    void dispatch_event(
        const std::unique_ptr<_Observer> &,
        typename std::enable_if_t<!std::is_base_of_v<Observer, _Observer>> * =
            0) {}

    template <typename _Observable>
    void subscribe_event(
        const std::unique_ptr<_Observable> &observable,
        typename std::enable_if_t<std::is_base_of_v<Observable, _Observable>>
            * = 0) {
        observable->event_source().subscribe([this](auto e) { this->post(e); });
    }

    template <typename _Observable>
    void subscribe_event(
        const std::unique_ptr<_Observable> &,
        typename std::enable_if_t<!std::is_base_of_v<Observable, _Observable>>
            * = 0) {}

    void parse_program_options(int argc, char **argv,
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
