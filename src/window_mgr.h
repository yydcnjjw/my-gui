#pragma once

#include <SDL2/SDL.h>
#include <rx.hpp>

#include "vulkan_ctx.h"

namespace my {
enum EventType { EVENT_QUIT, EVENT_MOUSE_MOTION, EVENT_KEYBOARD };

struct Event {
    EventType type;
    uint32_t timestamp;
};

struct Position {
    int32_t x;
    int32_t y;

    Position() : Position(0, 0) {}
    Position(int32_t x, int32_t y) : x(x), y(y) {}

    bool operator==(const Position &pos) const {
        return this->x == pos.x && this->y == pos.y;
    }
};

struct MouseMotionEvent : public Event {
    Position pos;
    int32_t xrel;
    int32_t yrel;
    uint32_t state;
};

struct KeyboardEvent : public Event {
    uint8_t state;
    bool repeat;
    SDL_Keysym keysym;
};

class Window : public VulkanPlatformSurface {
  public:
    virtual ~Window() = default;

    virtual std::pair<uint32_t, uint32_t> get_frame_buffer_size() = 0;

    [[nodiscard]] virtual std::vector<const char *>
    get_require_surface_extension() const = 0;

    [[nodiscard]] virtual vk::SurfaceKHR
    get_surface(vk::Instance instance) const = 0;
};

class WindowMgr {
  public:
    virtual ~WindowMgr() = default;

    // TODO: 考虑使用 window mgr 管理 window
    // return Window * 而不是 std::shared_ptr
    virtual Window *create_window(const std::string &title, uint32_t w,
                                  uint32_t h) = 0;
    virtual void remove_window(Window *) = 0;
    virtual rxcpp::observable<std::shared_ptr<Event>> get_observable() = 0;
    virtual rxcpp::observable<std::shared_ptr<Event>>
    event(const EventType &) = 0;
};

WindowMgr *get_window_mgr();

} // namespace my
