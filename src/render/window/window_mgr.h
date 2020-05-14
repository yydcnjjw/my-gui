#pragma once

#include <LLGL/LLGL.h>
#include <LLGL/Platform/NativeHandle.h>

#include <util/event_bus.hpp>
#include <render/window/event.h>
#include <my_render.hpp>

namespace my {
using WindowID = uint32_t;

class Window {
  public:
    virtual ~Window() = default;

    virtual std::shared_ptr<LLGL::Surface> get_surface() = 0;
    virtual sk_sp<SkSurface> get_sk_surface() = 0;
    virtual void swap_window() = 0;
    virtual WindowID get_window_id() = 0;
    virtual void hide() = 0;
    virtual void show() = 0;
    virtual void set_visible(bool visible) = 0;
    virtual bool is_visible() = 0;

    virtual void show_cursor(bool) = 0;
    virtual void set_mouse_pos(const IPoint2D &) = 0;
    virtual ISize2D get_frame_buffer_size() = 0;
    virtual void set_frame_buffer_size(const ISize2D &) = 0;
    virtual ISize2D get_min_size() = 0;
    virtual ISize2D get_max_size() = 0;
    virtual void set_min_size(const ISize2D &) = 0;
    virtual void set_max_size(const ISize2D &) = 0;
    virtual ISize2D get_size() = 0;
    virtual void set_size(const ISize2D &) = 0;
    virtual IPoint2D get_pos() = 0;
    virtual void set_pos(const IPoint2D &) = 0;
    virtual void set_full_screen(bool) = 0;
};

struct DisplayMode {
    uint32_t format;
    ISize2D size;
    int refresh_rate;
};

class WindowMgr {
  public:
    virtual ~WindowMgr() = default;

    // TODO: 考虑使用 window mgr 管理 window
    // return Window * 而不是 std::shared_ptr
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
