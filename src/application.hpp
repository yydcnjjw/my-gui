#pragma once

#include <core/config.hpp>
#include <core/coordination.hpp>
#include <core/event_bus.hpp>
#include <core/logger.hpp>
#include <storage/resource_service.hpp>
#include <window/window_service.hpp>

namespace my {

class ApplicationError : public std::runtime_error {
public:
  ApplicationError(const std::string &msg)
      : std::runtime_error(msg), _msg(msg) {}

  const char *what() const noexcept override { return this->_msg.c_str(); }

private:
  std::string _msg;
};

class Application : public EventBus {
  using self_type = Application;

public:
  using coordination_type = main_loop::coordination_type;
  using options_description = po::options_description;

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

  coordination_type coordination() { return this->_main_loop.coordination(); };

  template <typename Service, typename = std::enable_if_t<
                                  std::is_base_of_v<BasicService, Service>>>
  Service *service() {
    if (this->_services.count(typeid(Service)) == 0) {
      // TODO: More detailed error information
      throw ApplicationError("no service");
    }
    return dynamic_cast<Service *>(this->_services.at(typeid(Service)).get());
  }

  bool program_option(std::string const &option,
                      po::variable_value &value) const {
    if (this->_program_option_map.count(option)) {
      value = this->_program_option_map[option];
      return true;
    } else {
      return false;
    }
  }

  static Application *get() {
    assert(Application::_instance);
    return Application::_instance.get();
  }

  static Application *create(int argc, char **argv,
                             options_description const & = {});

protected:
  Application(int argc, char **argv, options_description const &opts_desc) {
    pthread_setname_np(pthread_self(), "application service");
    this->parse_program_options(argc, argv, opts_desc);
    this->register_service(WindowService::create());
    this->register_service(ResourceService::create());
  };

private:
  static unique_ptr<Application> _instance;
  my::main_loop _main_loop;
  options_description _opts_desc;
  po::variables_map _program_option_map;
  std::map<std::type_index, unique_ptr<BasicService>> _services;

  template <typename Service, typename = std::enable_if_t<
                                  std::is_base_of_v<BasicService, Service>>>
  void register_service(unique_ptr<Service> service) {
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
      unique_ptr<_Observer> const &observer,
      typename std::enable_if_t<std::is_base_of_v<Observer, _Observer>> * = 0) {
    observer->subscribe(this);
  }

  template <typename _Observer>
  void dispatch_event(
      unique_ptr<_Observer> const &,
      typename std::enable_if_t<!std::is_base_of_v<Observer, _Observer>> * =
          0) {}

  template <typename _Observable>
  void subscribe_event(
      unique_ptr<_Observable> const &observable,
      typename std::enable_if_t<std::is_base_of_v<Observable, _Observable>> * =
          0) {
    this->subscribe(observable.get());
  }

  template <typename _Observable>
  void subscribe_event(
      unique_ptr<_Observable> const &,
      typename std::enable_if_t<!std::is_base_of_v<Observable, _Observable>> * =
          0) {}

  void parse_program_options(int argc, char **argv,
                             options_description const &opts) {
    po::store(po::command_line_parser(argc, argv)
                  .options(opts)
                  .allow_unregistered()
                  .run(),
              this->_program_option_map);
    po::notify(this->_program_option_map);
  }
};
} // namespace my
