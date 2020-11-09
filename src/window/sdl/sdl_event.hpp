#pragma once

#include <window/event.hpp>
#include <window/exception.hpp>

#include <SDL2/SDL.h>

namespace my {

struct SDLMouseButtonEvent : public MouseButtonEvent {
    SDLMouseButtonEvent(WindowPtr ptr, SDL_MouseButtonEvent const &e)
        : MouseButtonEvent(ptr), _e(e) {}

    ButtonType button_type() override {
        switch (this->_e.button) {
        case SDL_BUTTON_LEFT:
            return MouseButtonEvent::kLeft;
        case SDL_BUTTON_MIDDLE:
            return MouseButtonEvent::kMiddle;
        case SDL_BUTTON_RIGHT:
            return MouseButtonEvent::kRight;
        case SDL_BUTTON_X1:
            return MouseButtonEvent::kX1;
        case SDL_BUTTON_X2:
            return MouseButtonEvent::kX2;
        }
        throw WindowServiceError("unknown button type");
    }
    ButtonState button_state() override {
        switch (this->_e.state) {
        case SDL_PRESSED:
            return MouseButtonEvent::kPressed;
        case SDL_RELEASED:
            return MouseButtonEvent::kReleased;
        }
        throw WindowServiceError("unknown button state");
    }

    uint8_t clicks() override { return this->_e.clicks; }
    IPoint2D pos() override { return IPoint2D::Make(this->_e.x, this->_e.y); }

  private:
    SDL_MouseButtonEvent _e;
};

} // namespace my
