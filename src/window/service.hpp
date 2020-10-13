#pragma once
#include <memory>
#include <util/async_task.hpp>

#include <my_render.hpp>
#include <window/event.hpp>

namespace my {

class Window {
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

struct DisplayMode {
    uint32_t format;
    ISize2D size;
    int refresh_rate;
};

class WindowService : public BasicService, public Observable, public Observer {
  public:
#define SERVICE_MAP(XX)                                                        \
    XX(WindowService, create_window, std::shared_ptr<Window>,                  \
       const std::string &, const ISize2D &)

    SERVICE_API(WindowService, SERVICE_MAP)

    static std::unique_ptr<WindowService> create();
};

} // namespace my
