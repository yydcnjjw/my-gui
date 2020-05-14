#pragma once

#include <SDL2/SDL.h>
#include <my_render.hpp>

namespace my {
using WindowID = uint32_t;
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
    IPoint2D pos;
    MouseState(uint32_t mask, IPoint2D pos) : mask(mask), pos(pos) {}
};
    
struct MouseMotionEvent {
    WindowID win_id;
    IPoint2D pos;
    ISize2D rel;
    uint32_t state;

    MouseMotionEvent(WindowID win_id, IPoint2D pos, ISize2D rel, uint32_t state)
        : win_id(win_id), pos(pos), rel(rel), state(state) {}
};

struct MoushWheelEvent {
    WindowID win_id;
    IPoint2D pos;
    uint32_t which;
    uint32_t direction;

    MoushWheelEvent(WindowID win_id, IPoint2D pos, uint32_t which,
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
    IPoint2D pos;
    MouseButtonEvent(WindowID win_id, uint32_t which, uint8_t button,
                     uint8_t state, uint8_t clicks, IPoint2D pos)
        : win_id(win_id), which(which), button(button), state(state),
          clicks(clicks), pos(pos) {}
};
 
struct WindowEvent {
    WindowID win_id;
    uint8_t event;
    WindowEvent(WindowID win_id, uint8_t event)
        : win_id(win_id), event(event) {}
};
}
