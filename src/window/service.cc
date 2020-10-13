#include "service.hpp"

#include <util/event_bus.hpp>

#include <SDL2/SDL.h>

namespace {

class WindowServiceError : public std::runtime_error {
  public:
    WindowServiceError(const std::string &msg)
        : std::runtime_error(msg), _msg(msg) {}

    const char *what() const noexcept override { return this->_msg.c_str(); }

  private:
    std::string _msg;
};

class SDLWindowService : public my::WindowService {
    typedef my::WindowService base_type;
    typedef base_type::observable_type observable_type;

  public:
    SDLWindowService() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
            throw WindowServiceError(SDL_GetError());
        }
        this->init_event_source();
    }

    observable_type event_source() override { return this->_event_source; }

    void subscribe(my::Observable *source) override { source->event_source(); }

    std::shared_ptr<my::Window> create_window(const std::string &title,
                                              const my::ISize2D &) override {}

  private:
    observable_type _event_source;

    void init_event_source() {
        this->_event_source =
            rxcpp::observable<>::interval(std::chrono::milliseconds(100),
                                          this->coordination().get())
                .flat_map([](auto) {
                    SDL_Event sdl_event;
                    std::vector<std::shared_ptr<my::IEvent>> v;
                    while (SDL_PollEvent(&sdl_event)) {
                        std::shared_ptr<my::IEvent> event;
                        switch (sdl_event.type) {
                        case SDL_QUIT:
                            event = my::Event<my::QuitEvent>::make();
                            break;
                        case SDL_MOUSEMOTION:
                            event = my::Event<my::MouseMotionEvent>::make(
                                sdl_event.motion.windowID,
                                my::IPoint2D::Make(sdl_event.motion.x,
                                                   sdl_event.motion.y),
                                my::ISize2D::Make(sdl_event.motion.xrel,
                                                  sdl_event.motion.yrel),
                                sdl_event.motion.state);
                            break;
                        case SDL_MOUSEWHEEL:
                            event = my::Event<my::MoushWheelEvent>::make(
                                sdl_event.wheel.windowID,
                                my::IPoint2D{sdl_event.wheel.x,
                                             sdl_event.wheel.y},
                                sdl_event.wheel.which,
                                sdl_event.wheel.direction);
                            break;
                        case SDL_KEYUP:
                        case SDL_KEYDOWN:
                            event = my::Event<my::KeyboardEvent>::make(
                                sdl_event.key.windowID, sdl_event.key.state,
                                sdl_event.key.repeat != 0,
                                sdl_event.key.keysym);
                            break;
                        case SDL_WINDOWEVENT:
                            event = my::Event<my::WindowEvent>::make(
                                sdl_event.window.windowID,
                                sdl_event.window.event);
                            break;
                        case SDL_MOUSEBUTTONDOWN:
                        case SDL_MOUSEBUTTONUP:
                            event = my::Event<my::MouseButtonEvent>::make(
                                sdl_event.button.windowID,
                                sdl_event.button.which, sdl_event.button.button,
                                sdl_event.button.state, sdl_event.button.clicks,
                                my::IPoint2D{sdl_event.button.x,
                                             sdl_event.button.y});
                            break;
                        default:
                            break;
                        }
                        v.push_back(event);
                    }
                    return rxcpp::observable<>::iterate(v);
                });
    }
};
} // namespace

namespace my {
std::unique_ptr<WindowService> WindowService::create() {
    return std::make_unique<SDLWindowService>();
}
} // namespace my
