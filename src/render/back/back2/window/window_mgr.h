#pragma once

#include <LLGL/LLGL.h>
#include <LLGL/Platform/NativeHandle.h>

#include <util/event_bus.hpp>
#include <my_render.hpp>

namespace my {

class WindowMgr {
  public:
    virtual ~WindowMgr() = default;

    virtual Window *create_window(const std::string &title, uint32_t w,
                                  uint32_t h) = 0;
    virtual void remove_window(Window *) = 0;

    virtual Keymod get_key_mode() = 0;

    virtual MouseState get_mouse_state() = 0;

    virtual const DisplayMode &get_desktop_display_mode() = 0;

    virtual bool has_window(WindowID id) = 0;

    static std::unique_ptr<WindowMgr> create(EventBus *);

  protected:
    WindowMgr() = default;
};

} // namespace my
