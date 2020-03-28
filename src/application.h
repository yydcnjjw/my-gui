#pragma once

#include <memory>

#include <boost/program_options.hpp>

#include <event_bus.hpp>

namespace my {
namespace program_options = boost::program_options;
class Application {
  public:
    Application(int, char *[]){};
    virtual ~Application() = default;
    virtual void run() = 0;

    virtual EventBus *get_ev_bus() const = 0;
    virtual const program_options::variable_value &
    get_program_option_value(const std::string &option) const = 0;
};

std::shared_ptr<Application> new_application(int argc, char *argv[]);
} // namespace my
