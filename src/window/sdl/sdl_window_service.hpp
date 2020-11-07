#pragma once
#include <window/window_service.hpp>
#include <window/exception.hpp>

#include <SDL2/SDL.h>

namespace my {
    struct SDLEvent {
    SDL_Event e;
};

class SDLWindow : public Window,
                  public std::enable_shared_from_this<SDLWindow> {
  public:
    SDLWindow(SDL_Window *win)
        : _sdl_window(win), _win_id(SDL_GetWindowID(this->_sdl_window)) {}

    ~SDLWindow() { ::SDL_DestroyWindow(this->_sdl_window); }

    ISize2D frame_buffer_size() override {
        // TODO: gl vulkan
        int w, h;
        // SDL_Vulkan_GetDrawableSize(this->_sdl_window, &w, &h);
        ::SDL_GL_GetDrawableSize(this->_sdl_window, &w, &h);
        return ISize2D::Make(w, h);
    }

    void frame_buffer_size(const ISize2D &size) override { this->size(size); }

    ISize2D min_size() override {
        int w, h;
        ::SDL_GetWindowMinimumSize(this->_sdl_window, &w, &h);
        return ISize2D::Make(w, h);
    }
    void min_size(const ISize2D &size) override {
        ::SDL_SetWindowMinimumSize(this->_sdl_window, size.width(),
                                   size.height());
    }

    ISize2D max_size() override {
        int w, h;
        ::SDL_GetWindowMaximumSize(this->_sdl_window, &w, &h);
        return ISize2D::Make(w, h);
    }

    void max_size(const ISize2D &size) override {
        ::SDL_SetWindowMaximumSize(this->_sdl_window, size.width(),
                                   size.height());
    }

    ISize2D size() override {
        int w, h;
        ::SDL_GetWindowSize(this->_sdl_window, &w, &h);
        return ISize2D::Make(w, h);
    }

    void size(const ISize2D &size) override {
        ::SDL_SetWindowSize(this->_sdl_window, size.width(), size.height());
    }
    IPoint2D pos() override {
        int x, y;
        ::SDL_GetWindowPosition(this->_sdl_window, &x, &y);
        return {x, y};
    }
    void pos(const IPoint2D &pos) override {
        ::SDL_SetWindowPosition(this->_sdl_window, pos.x(), pos.y());
    }

    virtual void full_screen(bool full_screen) override {
        ::SDL_SetWindowFullscreen(this->_sdl_window,
                                  full_screen ? SDL_WINDOW_FULLSCREEN : 0);
    }

    WindowID window_id() override { return this->_win_id; }

    void hide() override { this->visible(false); };
    void show() override { this->visible(true); }

    void visible(bool visible) override {
        if (visible == this->_is_visible) {
            return;
        }
        if (visible) {
            ::SDL_ShowWindow(this->_sdl_window);
        } else {
            ::SDL_HideWindow(this->_sdl_window);
        }
        this->_is_visible = visible;
    }

    bool visible() override { return this->_is_visible; }

    void show_cursor() override { ::SDL_ShowCursor(SDL_ENABLE); }
    void hide_cursor() override { ::SDL_ShowCursor(SDL_DISABLE); }

    void mouse_pos(const IPoint2D &pos) override {
        ::SDL_WarpMouseInWindow(this->_sdl_window, pos.x(), pos.y());
    }

    Observable::observable_type event_source() override {
        return this->_event_source;
    }

    void subscribe(Observable *o) override {
        this->_event_source =
            on_event<SDLEvent>(o)
                .map([self = this->shared_from_this()](auto e) {
                    SDL_Event &sdl_event = e->e;
                    std::shared_ptr<IEvent> event;
                    WindowID id;
                    switch (sdl_event.type) {
                    case SDL_MOUSEMOTION:
                        id = sdl_event.motion.windowID;
                        event = Event<MouseMotionEvent>::make(
                            self,
                            IPoint2D::Make(sdl_event.motion.x,
                                           sdl_event.motion.y),
                            ISize2D::Make(sdl_event.motion.xrel,
                                          sdl_event.motion.yrel),
                            sdl_event.motion.state);
                        break;
                    case SDL_MOUSEWHEEL:
                        id = sdl_event.wheel.windowID;
                        event = Event<MoushWheelEvent>::make(
                            self,
                            IPoint2D{sdl_event.wheel.x, sdl_event.wheel.y},
                            sdl_event.wheel.which, sdl_event.wheel.direction);
                        break;
                    case SDL_KEYUP:
                    case SDL_KEYDOWN:
                        id = sdl_event.key.windowID;
                        event = Event<KeyboardEvent>::make(
                            self, sdl_event.key.state,
                            sdl_event.key.repeat != 0, sdl_event.key.keysym);
                        break;
                    case SDL_WINDOWEVENT:
                        id = sdl_event.window.windowID;
                        event = Event<WindowStateEvent>::make(
                            self, sdl_event.window.event);
                        break;
                    case SDL_MOUSEBUTTONDOWN:
                    case SDL_MOUSEBUTTONUP:
                        id = sdl_event.button.windowID;
                        event = Event<MouseButtonEvent>::make(
                            self, sdl_event.button.which,
                            sdl_event.button.button, sdl_event.button.state,
                            sdl_event.button.clicks,
                            IPoint2D{sdl_event.button.x, sdl_event.button.y});
                        break;
                    }
                    return std::make_pair(id, event);
                })
                .filter([](auto event) {
                    return event.first ==
                           std::dynamic_pointer_cast<WindowEvent>(event.second)
                               ->win->window_id();
                })
                .map([](auto e) { return e.second; });
    }

  private:
    SDL_Window *_sdl_window{};
    WindowID _win_id{};

    bool _is_visible{true};

    Observable::observable_type _event_source;
};

class SDLWindowService : public WindowService {
    typedef WindowService base_type;
    typedef base_type::observable_type observable_type;

  public:
    SDLWindowService() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
            throw WindowServiceError(SDL_GetError());
        }
        this->init_event_source();
        pthread_setname_np(this->coordination().get_thread_info().handle,
                           "window service");
    }

    observable_type event_source() override {
        return this->_event_source.get_observable();
    }

    void subscribe(Observable *source) override { source->event_source(); }

    future<std::shared_ptr<Window>>
    create_window(const std::string &title, const ISize2D &size) override {
        return this->schedule<std::shared_ptr<Window>>([&]() {
            auto [w, h] = size;
            SDL_Window *sdl_win =
                SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED, w, h,
                                 SDL_WINDOW_UTILITY | SDL_WINDOW_ALLOW_HIGHDPI |
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
                                 // | SDL_WINDOW_VULKAN
                );

            if (sdl_win == nullptr) {
                throw WindowServiceError(SDL_GetError());
            }

            auto win = std::make_shared<SDLWindow>(sdl_win);
            win->subscribe(this);
            return win;
        });
    }

    future<DisplayMode> display_mode() override {
        return this->schedule<DisplayMode>([]() {
            SDL_DisplayMode mode;
            if (SDL_GetDesktopDisplayMode(0, &mode) != 0) {
                throw WindowServiceError(SDL_GetError());
            }
            return DisplayMode{
                mode.format, {mode.w, mode.h}, mode.refresh_rate};
        });
    }

    future<Keymod> key_mod() override {
        return this->schedule<Keymod>(
            []() { return static_cast<Keymod>(SDL_GetModState()); });
    }
    future<MouseState> mouse_state() override {
        return this->schedule<MouseState>([]() {
            int x, y;
            auto mask = SDL_GetMouseState(&x, &y);
            return MouseState{mask, {x, y}};
        });
    }

  private:
    rxcpp::subjects::subject<std::shared_ptr<IEvent>> _event_source;

    void init_event_source() {
        rxcpp::observable<>::interval(std::chrono::milliseconds(100),
                                      this->coordination().get())
            .flat_map([](auto) {
                std::vector<std::shared_ptr<IEvent>> v;
                while (true) {
                    auto e = Event<SDLEvent>::make();
                    if (SDL_PollEvent(&e->e)) {
                        switch (e->e.type) {
                        case SDL_QUIT:
                            v.push_back(Event<QuitEvent>::make());
                            break;
                        default:
                            v.push_back(e);
                            break;
                        }
                    } else {
                        break;
                    }
                }
                return rxcpp::observable<>::iterate(v);
            })
            .multicast(this->_event_source)
            .connect();
    }
};
std::unique_ptr<WindowService> WindowService::create() {
    return std::make_unique<SDLWindowService>();
}
}
