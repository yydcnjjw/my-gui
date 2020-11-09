#pragma once

#include <core/core.hpp>
#include <window/type.hpp>

namespace my {
using WindowID = uint32_t;

class Window : public Observable, public Observer {
  public:
    virtual ~Window() = default;

    virtual WindowID window_id() = 0;
    virtual void hide() = 0;
    virtual void show() = 0;

    virtual void visible(bool visible) = 0;
    virtual bool visible() = 0;

    virtual void show_cursor() = 0;
    virtual void hide_cursor() = 0;

    virtual void mouse_pos(const IPoint2D &) = 0;

    virtual ISize2D frame_buffer_size() = 0;
    virtual void frame_buffer_size(const ISize2D &) = 0;
    virtual ISize2D min_size() = 0;
    virtual void min_size(const ISize2D &) = 0;
    virtual ISize2D max_size() = 0;
    virtual void max_size(const ISize2D &) = 0;
    virtual ISize2D size() = 0;
    virtual void size(const ISize2D &) = 0;
    virtual IPoint2D pos() = 0;
    virtual void pos(const IPoint2D &) = 0;
    virtual void full_screen(bool) = 0;
};

using WindowPtr = shared_ptr<Window>;
struct GetWindowPtr {
    GetWindowPtr(WindowPtr ptr) : _ptr(ptr) {}
    WindowPtr window_ptr() { return this->_ptr; }

  private:
    WindowPtr _ptr;
};

} // namespace my
