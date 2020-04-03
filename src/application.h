#pragma once

#include <filesystem>
#include <memory>

#include <boost/program_options.hpp>

#include <async_task.h>
#include <event_bus.hpp>
#include <font_mgr.h>
#include <resource_mgr.h>
#include <window_mgr.h>

namespace my {
namespace fs = std::filesystem;
namespace program_options = boost::program_options;
class Application {
  public:
    Application(){};
    virtual ~Application() = default;
    virtual void run() = 0;

    virtual void quit() = 0;

    virtual EventBus *ev_bus() const = 0;
    virtual WindowMgr *win_mgr() const = 0;
    virtual FontMgr *font_mgr() const = 0;
    virtual AsyncTask *async_task() const = 0;
    virtual ResourceMgr *resource_mgr() const = 0;
    virtual bool
    get_program_option(const std::string &option,
                       program_options::variable_value &value) const = 0;
};

std::shared_ptr<Application>
new_application(int argc, char **argv,
                const program_options::options_description &);
} // namespace my
