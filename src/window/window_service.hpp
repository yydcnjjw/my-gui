#pragma once

#include <core/core.hpp>
#include <window/type.hpp>
#include <window/window_instance.hpp>

namespace my {

struct DisplayMode {
    uint32_t format;
    ISize2D size;
    int refresh_rate;
};

class WindowService : public BasicService, public Observable, public Observer {
  public:
    virtual ~WindowService() = default;

    virtual future<WindowPtr>
    create_window(const std::string &title, const ISize2D &size) = 0;

    virtual future<DisplayMode> display_mode() = 0;
    // virtual future<Keymod> key_mod() = 0;
    // virtual future<MouseState> mouse_state() = 0;

    static unique_ptr<WindowService> create();
};

} // namespace my
