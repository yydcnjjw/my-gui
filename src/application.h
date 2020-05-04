#pragma once

#include <media/audio_mgr.h>
#include <my_gui.h>
#include <render/window/window_mgr.h>
#include <render/canvas.h>
#include <storage/font_mgr.h>
#include <storage/resource_mgr.hpp>
#include <util/async_task.hpp>
#include <util/event_bus.hpp>

namespace my {

class Application {
  public:
    Application(){};
    virtual ~Application() = default;
    virtual void run() = 0;

    virtual void quit(bool is_error = false) = 0;

    virtual EventBus *ev_bus() const = 0;
    virtual WindowMgr *win_mgr() const = 0;
    virtual FontMgr *font_mgr() const = 0;
    virtual AsyncTask *async_task() const = 0;
    virtual ResourceMgr *resource_mgr() const = 0;
    virtual AudioMgr *audio_mgr() const = 0;
    virtual LLGL::RenderSystem *renderer() const = 0;
    virtual std::shared_ptr<Canvas> make_canvas(Window *) = 0;
    virtual bool
    get_program_option(const std::string &option,
                       program_options::variable_value &value) const = 0;
};

std::shared_ptr<Application>
new_application(int argc, char **argv,
                const program_options::options_description &);
} // namespace my
