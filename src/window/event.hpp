#pragma once

#include <window/type.hpp>

namespace my {

// struct MouseState {
//     uint32_t mask;
//     IPoint2D pos;
//     MouseState(uint32_t mask, IPoint2D pos) : mask(mask), pos(pos) {}
// };
class Window;
using WidnowPtr = std::shared_ptr<Window>;

// struct WindowEvent {
//     WidnowPtr win;
//     WindowEvent(WidnowPtr win) : win(win) {}
// };

// struct MouseMotionEvent : public WindowEvent {
//     IPoint2D pos;
//     ISize2D rel;
//     uint32_t state;

//     MouseMotionEvent(WidnowPtr win, IPoint2D pos, ISize2D rel, uint32_t
//     state)
//         : WindowEvent(win), pos(pos), rel(rel), state(state) {}
// };

// struct MoushWheelEvent : public WindowEvent {
//     IPoint2D pos;
//     uint32_t which;
//     uint32_t direction;

//     MoushWheelEvent(WidnowPtr win, IPoint2D pos, uint32_t which,
//                     uint32_t direction)
//         : WindowEvent(win), pos(pos), which(which), direction(direction) {}
// };

// struct KeyboardEvent : public WindowEvent {
//     uint8_t state;
//     bool repeat;
//     Keysym keysym;
//     KeyboardEvent(WidnowPtr win, uint8_t state, bool repeat, Keysym keysym)
//         : WindowEvent(win), state(state), repeat(repeat), keysym(keysym) {}
// };

    struct WindowPtr {
        
    };
    
struct MouseButtonEvent {

    enum ButtonType { kLeft, kMiddle, kRight, kX1, kX2 };

    enum ButtonState { kPressed, kReleased };

    ButtonState state;
    ButtonType button;
    uint8_t clicks;
    IPoint2D pos;

    // uint32_t which;

    bool is_pressed() { return this->state == kPressed; }
    bool is_released() { return this->state == kReleased; }

    bool is_single_click() { return this->clicks == 1; }
    bool is_double_click() { return this->clicks == 2; }

    MouseButtonEvent(WidnowPtr win, uint32_t which, uint8_t button,
                     uint8_t state, uint8_t clicks, IPoint2D pos)
        : WindowEvent(win), which(which), button(button), state(state),
          clicks(clicks), pos(pos) {}
};

// struct WindowStateEvent : public WindowEvent {

//     enum event_type {

//     };

//     uint8_t event;
//     WindowStateEvent(WidnowPtr win, uint8_t event)
//         : WindowEvent(win), event(event) {}
// };
} // namespace my
