#pragma once

#include <util/event_bus.hpp>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <LLGL/LLGL.h>
#include <LLGL/Platform/NativeHandle.h>

namespace my {
typedef uint32_t WindowID;
typedef glm::i32vec2 PixelPos;

struct MouseMotionEvent {
    WindowID win_id;
    PixelPos pos;
    int32_t xrel;
    int32_t yrel;
    uint32_t state;

    MouseMotionEvent(WindowID win_id, PixelPos pos, int32_t xrel, int32_t yrel,
                     uint32_t state)
        : win_id(win_id), pos(pos), xrel(xrel), yrel(yrel), state(state) {}
};

struct MoushWheelEvent {
    WindowID win_id;
    PixelPos pos;
    uint32_t which;
    uint32_t direction;

    MoushWheelEvent(WindowID win_id, PixelPos pos, uint32_t which,
                    uint32_t direction)
        : win_id(win_id), pos(pos), which(which), direction(direction) {}
};

struct KeyboardEvent {
    WindowID win_id;
    uint8_t state;
    bool repeat;
    SDL_Keysym keysym;
    KeyboardEvent(WindowID win_id, uint8_t state, bool repeat,
                  SDL_Keysym keysym)
        : win_id(win_id), state(state), repeat(repeat), keysym(keysym) {}
};

struct MouseButtonEvent {
    WindowID win_id;
    uint32_t which;
    uint8_t button;
    uint8_t state;
    uint8_t clicks;
    PixelPos pos;
    MouseButtonEvent(WindowID win_id, uint32_t which, uint8_t button,
                     uint8_t state, uint8_t clicks, PixelPos pos)
        : win_id(win_id), which(which), button(button), state(state),
          clicks(clicks), pos(pos) {}
};

struct WindowEvent {
    WindowID win_id;
    uint8_t event;
    WindowEvent(WindowID win_id, uint8_t event)
        : win_id(win_id), event(event) {}
};

struct Size2D {
    u_int32_t w;
    u_int32_t h;
    Size2D() : Size2D(0, 0) {}
    Size2D(u_int32_t w, u_int32_t h) : w(w), h(h) {}

    bool operator==(Size2D &size) {
        return this->w == size.w && this->h == size.h;
    }
};

class Window {
  public:
    virtual ~Window() = default;

    virtual std::shared_ptr<LLGL::Surface> get_surface() = 0;
    virtual WindowID get_window_id() = 0;
    virtual void hide() = 0;
    virtual void show() = 0;
    virtual void set_visible(bool visible) = 0;
    virtual bool is_visible() = 0;

    virtual void show_cursor(bool) = 0;
    virtual void set_mouse_pos(const PixelPos&) = 0;
    virtual Size2D get_frame_buffer_size() = 0;
    virtual void set_frame_buffer_size(const Size2D &) = 0;
    virtual Size2D get_min_size() = 0;
    virtual Size2D get_max_size() = 0;
    virtual void set_min_size(const Size2D &) = 0;
    virtual void set_max_size(const Size2D &) = 0;
    virtual Size2D get_size() = 0;
    virtual void set_size(const Size2D &) = 0;
    virtual PixelPos get_pos() = 0;
    virtual void set_pos(const PixelPos &) = 0;
    virtual void set_full_screen(bool) = 0;
};

struct DisplayMode {
    uint32_t format;
    int w;
    int h;
    int refresh_rate;
};

enum Keymod {
    KMOD_NONE = 0x0000,
    KMOD_LSHIFT = 0x0001,
    KMOD_RSHIFT = 0x0002,
    KMOD_LCTRL = 0x0040,
    KMOD_RCTRL = 0x0080,
    KMOD_LALT = 0x0100,
    KMOD_RALT = 0x0200,
    KMOD_LGUI = 0x0400,
    KMOD_RGUI = 0x0800,
    KMOD_NUM = 0x1000,
    KMOD_CAPS = 0x2000,
    KMOD_MODE = 0x4000,
    KMOD_RESERVED = 0x8000
};

#define BUTTON(X) (1 << ((X)-1))
#define BUTTON_LEFT 1
#define BUTTON_MIDDLE 2
#define BUTTON_RIGHT 3
#define BUTTON_X1 4
#define BUTTON_X2 5
#define BUTTON_LMASK BUTTON(BUTTON_LEFT)
#define BUTTON_MMASK BUTTON(BUTTON_MIDDLE)
#define BUTTON_RMASK BUTTON(BUTTON_RIGHT)
#define BUTTON_X1MASK BUTTON(BUTTON_X1)
#define BUTTON_X2MASK BUTTON(BUTTON_X2)

struct MouseState {
    uint32_t mask;
    PixelPos pos;
    MouseState(uint32_t mask, PixelPos pos) : mask(mask), pos(pos) {}
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
