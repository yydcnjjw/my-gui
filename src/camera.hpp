#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "event_bus.hpp"
#include "logger.h"
#include "window_mgr.h"

namespace my {
constexpr float SENSITIVITY = 0.1f;
constexpr float MOVE_SPEED = 0.01f;
class Camera {
  public:
    Camera(EventBus *bus, glm::vec3 postion = {0.0f, 0.0f, 0.0f},
           glm::vec3 up = {0.0f, 1.0f, 0.0f})
        : _position(postion), _up(up) {
        this->view = glm::lookAt(postion, this->_front, up);

        assert(bus);
        this->_subscribe_event(bus);
    }

    glm::mat4 perspective;
    glm::mat4 view;

  private:
    glm::vec3 _position;
    glm::vec3 _up;
    glm::vec3 _front{0.0f, 0.0f, 0.0f};
    glm::vec3 _right{this->_get_right(this->_front)};

    PixelPos _mouse_pos;
    float _yaw{0.0f};
    float _pitch{0.0f};

    inline glm::vec3 _get_right(const glm::vec3 &front) {
        return glm::normalize(glm::cross(front, {0.0f, 1.0f, 0.0f}));
    }

    inline glm::vec3 _get_front(float yaw, float pitch) {
        return glm::normalize(glm::vec3{
            cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
            sin(glm::radians(pitch)),
            sin(glm::radians(yaw)) * cos(glm::radians(pitch)),
        });
    }
    inline glm::vec3 _get_up(const glm::vec3 &front, const glm::vec3 &right) {
        return glm::normalize(glm::cross(right, front));
    }

    void _subscribe_event(EventBus *bus) {
        bus->on_event<MouseMotionEvent>()
            .subscribe_on(bus->ev_bus_worker())
            .observe_on(bus->ev_bus_worker())
            .subscribe(
                [this](const std::shared_ptr<Event<MouseMotionEvent>> &e) {
                    this->_update_mouse_motion(e->data);
                });
        bus->on_event<KeyboardEvent>()
            .subscribe_on(bus->ev_bus_worker())
            .observe_on(bus->ev_bus_worker())
            .subscribe(
                [this](const std::shared_ptr<my::Event<KeyboardEvent>> &e) {
                    this->_update_keyboard(e->data);
                });
    }

    void _update_view() {
        this->view = glm::lookAt(this->_position,
                                 this->_position + this->_front, this->_up);
    }

    void _update_mouse_motion(const std::shared_ptr<my::MouseMotionEvent> &e) {
        if (this->_mouse_pos == e->pos) {
            return;
        }
        this->_mouse_pos = e->pos;

        this->_yaw += e->xrel * SENSITIVITY;
        this->_pitch += e->yrel * SENSITIVITY;

        if (this->_pitch > 89.0f)
            this->_pitch = 89.0f;
        if (this->_pitch < -89.0f)
            this->_pitch = -89.0f;

        this->_front = this->_get_front(this->_yaw, this->_pitch);

        this->_right = this->_get_right(this->_front);

        this->_up = this->_get_up(this->_front, this->_right);

        this->_update_view();

        // this->perspective =
        // glm::perspective(T fovy, T aspect, T zNear, T zFar)
    }

    std::map<uint32_t, uint32_t> input_map{{SDL_SCANCODE_W, 0},
                                           {SDL_SCANCODE_S, 0},
                                           {SDL_SCANCODE_A, 0},
                                           {SDL_SCANCODE_D, 0}};

    void _update_keyboard(const std::shared_ptr<my::KeyboardEvent> &e) {

        if (e->state != SDL_PRESSED) {
            return;
        }

        auto velocity = MOVE_SPEED * 10;

        switch (e->keysym.scancode) {
        case SDL_SCANCODE_W:
            this->_position += this->_front * velocity;
            break;
        case SDL_SCANCODE_S:
            this->_position -= this->_front * velocity;
            break;
        case SDL_SCANCODE_A:
            this->_position -= this->_right * velocity;
            break;
        case SDL_SCANCODE_D:
            this->_position += this->_right * velocity;
            break;
        default:
            break;
        }
        this->_update_view();
    }
};
} // namespace my
