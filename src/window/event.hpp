#pragma once

#include <core/core.hpp>
#include <window/type.hpp>
#include <window/window_instance.hpp>

namespace my {

// struct MouseState {
//     uint32_t mask;
//     IPoint2D pos;
//     MouseState(uint32_t mask, IPoint2D pos) : mask(mask), pos(pos) {}
// };

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

struct MouseButtonEvent
    : public GetWindowPtr,
      public std::enable_shared_from_this<MouseButtonEvent> {
    enum ButtonType { kLeft, kMiddle, kRight, kX1, kX2 };
    enum ButtonState { kPressed, kReleased };

    inline shared_ptr<MouseButtonEvent> make_translate(IPoint2D translate);

    virtual ButtonType button_type() = 0;
    virtual ButtonState button_state() = 0;
    virtual uint8_t clicks() = 0;
    virtual IPoint2D pos() = 0;

    bool is_pressed() { return this->button_state() == kPressed; }
    bool is_released() { return this->button_state() == kReleased; }

    MouseButtonEvent(WindowPtr ptr) : GetWindowPtr(ptr) {}
};

struct MouseButtonEventWithTranslate : public MouseButtonEvent {
    MouseButtonEventWithTranslate(shared_ptr<MouseButtonEvent> e,
                                  IPoint2D translate)
        : MouseButtonEvent(e->window_ptr()), _e(e), _translate(translate) {}

    ButtonType button_type() override { return this->_e->button_type(); }
    ButtonState button_state() override { return this->_e->button_state(); }
    uint8_t clicks() override { return this->_e->clicks(); }
    IPoint2D pos() override { return this->_e->pos() + this->_translate; }

  private:
    shared_ptr<MouseButtonEvent> _e;
    IPoint2D _translate;
};

shared_ptr<MouseButtonEvent>
MouseButtonEvent::make_translate(IPoint2D translate) {
    return std::make_shared<MouseButtonEventWithTranslate>(
        this->shared_from_this(), translate);
}

// struct WindowStateEvent : public WindowEvent {

//     enum event_type {

//     };

//     uint8_t event;
//     WindowStateEvent(WidnowPtr win, uint8_t event)
//         : WindowEvent(win), event(event) {}
// };
} // namespace my
