#include "window_mgr.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <util/logger.h>

namespace {

class SDLWindow : public my::Window {
  public:
    class SDLSurface : public LLGL::Surface {
      public:
        SDLSurface(SDLWindow *win) : _win(win) {}

        bool GetNativeHandle(void *nativeHandle, std::size_t) const override {
            auto handle = reinterpret_cast<LLGL::NativeHandle *>(nativeHandle);
            SDL_SysWMinfo info;
            SDL_VERSION(&info.version);
            if (!::SDL_GetWindowWMInfo(this->_win->_sdl_window, &info)) {
                throw std::runtime_error(SDL_GetError());
            }
            handle->display = info.info.x11.display;
            handle->window = info.info.x11.window;

            return true;
        };

        LLGL::Extent2D GetContentSize() const override {
            auto size = this->_win->get_frame_buffer_size();
            return {size.w, size.h};
        };

        bool
        AdaptForVideoMode(LLGL::VideoModeDescriptor &videoModeDesc) override {
            this->_win->set_size({videoModeDesc.resolution.width,
                                  videoModeDesc.resolution.height});
            this->_win->set_full_screen(videoModeDesc.fullscreen);
            return true;
        }

        void ResetPixelFormat() override {}

        bool ProcessEvents() override { return true; }

        std::unique_ptr<LLGL::Display> FindResidentDisplay() const override {
            return std::move(LLGL::Display::InstantiateList().front());
        };

      private:
        SDLWindow *_win;
    };

    SDLWindow(SDL_Window *win)
        : _sdl_window(win), _win_id(SDL_GetWindowID(this->_sdl_window)),
          _surface(std::make_shared<SDLSurface>(this)) {}

    ~SDLWindow() { SDL_DestroyWindow(this->_sdl_window); }

    std::shared_ptr<LLGL::Surface> get_surface() override {
        return this->_surface;
    };

    my::Size2D get_frame_buffer_size() override {
        // TODO: gl vulkan
        int w, h;
        SDL_Vulkan_GetDrawableSize(this->_sdl_window, &w, &h);
        return my::Size2D(w, h);
    }

    void set_frame_buffer_size(const my::Size2D &size) override {
        this->set_size(size);
    }

    my::Size2D get_min_size() override {
        int w, h;
        SDL_GetWindowMinimumSize(this->_sdl_window, &w, &h);
        return my::Size2D(w, h);
    }
    my::Size2D get_max_size() override {
        int w, h;
        SDL_GetWindowMaximumSize(this->_sdl_window, &w, &h);
        return my::Size2D(w, h);
    }

    void set_min_size(const my::Size2D &size) override {
        SDL_SetWindowMinimumSize(this->_sdl_window, size.w, size.h);
    }
    void set_max_size(const my::Size2D &size) override {
        SDL_SetWindowMaximumSize(this->_sdl_window, size.w, size.h);
    }

    my::Size2D get_size() override {
        int w, h;
        SDL_GetWindowSize(this->_sdl_window, &w, &h);
        return my::Size2D(w, h);
    }

    void set_size(const my::Size2D &size) override {
        SDL_SetWindowSize(this->_sdl_window, size.w, size.h);
    }
    my::PixelPos get_pos() override {
        int x, y;
        SDL_GetWindowPosition(this->_sdl_window, &x, &y);
        return {x, y};
    }
    void set_pos(const my::PixelPos &pos) override {
        SDL_SetWindowPosition(this->_sdl_window, pos.x, pos.y);
    }

    virtual void set_full_screen(bool full_screen) override {
        SDL_SetWindowFullscreen(this->_sdl_window,
                                full_screen ? SDL_WINDOW_FULLSCREEN : 0);
    }

    my::WindowID get_window_id() override { return this->_win_id; }

    void hide() override { this->set_visible(false); };
    void show() override { this->set_visible(true); }

    void set_visible(bool visible) override {
        if (visible == this->_is_visible) {
            return;
        }
        if (visible) {
            SDL_ShowWindow(this->_sdl_window);
        } else {
            SDL_HideWindow(this->_sdl_window);
        }
        this->_is_visible = visible;
    }

    bool is_visible() override { return this->_is_visible; }

    void show_cursor(bool toggle) override {
        SDL_ShowCursor(toggle ? SDL_ENABLE : SDL_DISABLE);
    }

    void set_mouse_pos(const my::PixelPos &pos) override {
        SDL_WarpMouseInWindow(this->_sdl_window, pos.x, pos.y);
    }

    SDL_Window *_sdl_window;
    my::WindowID _win_id;
    std::shared_ptr<SDLSurface> _surface;
    bool _is_visible{true};
};

class SDLWindowMgr : public my::WindowMgr {

  public:
    SDLWindowMgr(my::EventBus *bus) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO) != 0) {
            throw std::runtime_error(SDL_GetError());
        }

        this->_init_desktop_display_mode();
        bus->on_event<my::QuitEvent>()
            .observe_on(bus->ev_bus_worker())
            .subscribe([this](const auto &) { this->_is_exit = true; });
        this->_win_mgr_thread = std::thread([this, bus]() {
            pthread_setname_np(pthread_self(), "win mgr");
            GLOG_D("window loop running!");

            while (!this->_is_exit) {
                SDL_Event sdl_event{};
                SDL_WaitEventTimeout(&sdl_event, 200);
                switch (sdl_event.type) {
                case SDL_QUIT:
                    // bus->post<my::QuitEvent>();
                    // this->_is_exit = true;
                    break;
                case SDL_MOUSEMOTION:
                    bus->post<my::MouseMotionEvent>(
                        sdl_event.motion.windowID,
                        my::PixelPos{sdl_event.motion.x, sdl_event.motion.y},
                        sdl_event.motion.xrel, sdl_event.motion.yrel,
                        sdl_event.motion.state);
                    break;
                case SDL_MOUSEWHEEL:
                    bus->post<my::MoushWheelEvent>(
                        sdl_event.wheel.windowID,
                        my::PixelPos{sdl_event.wheel.x, sdl_event.wheel.y},
                        sdl_event.wheel.which, sdl_event.wheel.direction);
                    break;
                case SDL_KEYUP:
                case SDL_KEYDOWN:
                    bus->post<my::KeyboardEvent>(
                        sdl_event.key.windowID, sdl_event.key.state,
                        sdl_event.key.repeat != 0, sdl_event.key.keysym);
                    break;
                case SDL_WINDOWEVENT:
                    bus->post<my::WindowEvent>(sdl_event.window.windowID,
                                               sdl_event.window.event);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    bus->post<my::MouseButtonEvent>(
                        sdl_event.button.windowID, sdl_event.button.which,
                        sdl_event.button.button, sdl_event.button.state,
                        sdl_event.button.clicks,
                        my::PixelPos{sdl_event.button.x, sdl_event.button.y});
                    break;
                default:
                    break;
                }
            }
            GLOG_D("window loop exit!");
        });
    }

    ~SDLWindowMgr() {
        if (!this->_is_exit) {
            this->_is_exit = true;
        }
        if (this->_win_mgr_thread.joinable()) {
            this->_win_mgr_thread.join();
        }
        SDL_Quit();
    }
    my::Window *create_window(const std::string &title, uint32_t w,
                              uint32_t h) override {
        // TODO: 考虑 继承
        SDL_Window *sdl_win = SDL_CreateWindow(
            title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w,
            h,
            SDL_WINDOW_VULKAN | SDL_WINDOW_UTILITY | SDL_WINDOW_ALLOW_HIGHDPI |
                SDL_WINDOW_RESIZABLE);

        if (sdl_win == nullptr) {
            throw std::runtime_error(SDL_GetError());
        }
        auto win = std::make_unique<SDLWindow>(sdl_win);
        auto id = win->get_window_id();
        GLOG_D("----------create window %d %s", id, title.c_str());
        this->_windows[id] = std::move(win);
        return this->_windows.at(id).get();
    }

    void remove_window(my::Window *win) override {
        GLOG_D("----------remove window %d", win->get_window_id());
        this->_windows.erase(win->get_window_id());
    }

    const my::DisplayMode &get_desktop_display_mode() override {
        return this->_desktop_display_mode;
    }

    my::Keymod get_key_mode() override {
        return static_cast<my::Keymod>(SDL_GetModState());
    }

    my::MouseState get_mouse_state() override {
        int x, y;
        auto mask = SDL_GetMouseState(&x, &y);
        return my::MouseState(mask, {x, y});
    }

    bool has_window(my::WindowID id) override {
        return this->_windows.find(id) != this->_windows.end();
    }

  private:
    std::map<my::WindowID, std::unique_ptr<SDLWindow>> _windows;
    std::thread _win_mgr_thread;
    my::DisplayMode _desktop_display_mode;
    std::atomic_bool _is_exit = false;
    void _init_desktop_display_mode() {
        SDL_DisplayMode mode;
        if (SDL_GetDesktopDisplayMode(0, &mode) != 0) {
            throw std::runtime_error(SDL_GetError());
        }
        this->_desktop_display_mode = {mode.format, mode.w, mode.h,
                                       mode.refresh_rate};
    }
};

} // namespace

namespace my {
std::unique_ptr<WindowMgr> WindowMgr::create(EventBus *bus) {
    return std::make_unique<SDLWindowMgr>(bus);
}

} // namespace my
