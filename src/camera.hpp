#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "logger.h"
#include "window_mgr.h"

namespace my {
constexpr float SENSITIVITY = 0.1f;
constexpr float MOVE_SPEED = 0.01f;
class Camera {
  public:
    Camera(WindowMgr *win_mgr = nullptr, glm::vec3 postion = {0.0f, 0.0f, 0.0f},
           glm::vec3 up = {0.0f, 0.0f, 1.0f})
        : _position(postion), _up(up) {
        this->view = glm::lookAt(postion, this->_front, up);

        if (win_mgr) {
            this->_subscribe_window_event(win_mgr);
        }
    }

    glm::mat4 perspective;
    glm::mat4 view;

  private:
    glm::vec3 _position;
    glm::vec3 _up;
    glm::vec3 _front{0.0f, 0.0f, 0.0f};
    glm::vec3 _right{this->_get_right(this->_front)};

    Position _mouse_pos;
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
        return glm::normalize(glm::cross(front, right));
    }

    void _subscribe_window_event(WindowMgr *win_mgr) {
        win_mgr->event(EventType::EVENT_MOUSE_MOTION)
            .subscribe([this](const std::shared_ptr<my::Event> &e) {
                this->_update_mouse_motion(
                    std::static_pointer_cast<my::MouseMotionEvent>(e));
            });
        win_mgr->event(EventType::EVENT_KEYBOARD)
            .subscribe([this](const std::shared_ptr<my::Event> &e) {
                this->_update_keyboard(
                    std::static_pointer_cast<my::KeyboardEvent>(e));
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
        this->_pitch += -e->yrel * SENSITIVITY;

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
        if (e->state == SDL_PRESSED) {
            input_map[e->keysym.scancode] = e->timestamp;
        } else if (e->state == SDL_RELEASED) {
            for (const auto &map : input_map) {
                GLOG_D("%d, %d", map.first, map.second);
            }

            auto current_timestamp = e->timestamp;
            GLOG_D("current timestamp %d", current_timestamp);

            if (input_map[SDL_SCANCODE_W]) {
                auto velocity =
                    (current_timestamp - input_map[SDL_SCANCODE_W]) *
                    MOVE_SPEED;
                this->_position += this->_front * velocity;
            }
            if (input_map[SDL_SCANCODE_S]) {
                auto velocity =
                    (current_timestamp - input_map[SDL_SCANCODE_S]) *
                    MOVE_SPEED;
                this->_position -= this->_front * velocity;
            }
            if (input_map[SDL_SCANCODE_A]) {
                auto velocity =
                    (current_timestamp - input_map[SDL_SCANCODE_A]) *
                    MOVE_SPEED;
                this->_position -= this->_right * velocity;
            }
            if (input_map[SDL_SCANCODE_D]) {
                auto velocity =
                    (current_timestamp - input_map[SDL_SCANCODE_D]) *
                    MOVE_SPEED;
                this->_position += this->_right * velocity;
            }

            input_map[e->keysym.scancode] = 0;
            this->_update_view();
        }
    }
};
} // namespace my
