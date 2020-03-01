#include "window.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <map>
#include <sstream>
#include <stack>
#include <stdexcept>

namespace GLFW {

class GLFWWindowManager : public WindowManager {
  private:
    static std::map<GLFWwindow *, std::shared_ptr<Window>> _win_map;
    GLFWwindow *getGLFWwindow(const std::shared_ptr<Window> &win_ptr) {
        GLFWwindow *window = nullptr;
        for (const auto &entry : _win_map) {
            if (entry.second == win_ptr) {
                window = entry.first;
            }
        }
        if (window == nullptr) {
            std::ostringstream oss;
            oss << "window is not be registered";
            throw std::runtime_error(oss.str());
        }
        return window;
    }

  public:
    GLFWWindowManager() {
        glfwInit();
        // TODO: 目前仅实现了 vulkan
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }
    ~GLFWWindowManager() { glfwTerminate(); }

    void GetWindowSurface(const std::shared_ptr<Window> &win_ptr,
                          const VkInstance &instance,
                          VkSurfaceKHR &surface) override {
        GLFWwindow *window = getGLFWwindow(win_ptr);

        VkResult code;
        if ((code = glfwCreateWindowSurface(instance, window, nullptr,
                                            &surface)) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }
    void RegisterWindow(const std::shared_ptr<Window> &win) override {
        int w, h;
        win->GetWindowSize(w, h);
        auto window =
            glfwCreateWindow(w, h, win->GetTitle().c_str(), nullptr, nullptr);
        if (!window) {
            throw std::runtime_error("glfw window create failure");
        }

        glfwGetFramebufferSize(window, &w, &h);
        win->SetFrameBufferSize(w, h);

        glfwSetWindowFocusCallback(window, window_focus_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        glfwSetScrollCallback(window, scroll_callback);
        glfwSetCursorPosCallback(window, cursor_position_callback);
        glfwSetKeyCallback(window, key_callback);
        glfwSetCharCallback(window, character_callback);
        glfwSetWindowCloseCallback(window, window_close_callback);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetWindowSizeCallback(window, window_size_callback);

        win->SetWindowSize(w, h);
        _win_map[window] = win;
    }

    void UnregisterWindow(const std::shared_ptr<Window> &win_ptr) override {
        auto win = getGLFWwindow(win_ptr);
        _win_map.erase(win);
        glfwDestroyWindow(win);
    }

    std::vector<const char *> GetRequiredExtensions() override {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions = NULL;
        // required extensions of glfw
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        if (glfwExtensions == NULL) {
            std::runtime_error("glfw get required instance extensions failure");
        }
        std::vector<const char *> extensions(
            glfwExtensions, glfwExtensions + glfwExtensionCount);
        return extensions;
    }

    void Poll() override {
        glfwPollEvents();
        for (const auto &win : _win_map) {
            win.second->OnPaint();
        }
    }

    bool shouldExit() override{
        for (const auto &win : _win_map) {
            if (glfwWindowShouldClose(win.first)) {
                return false;
            }
        }
        return true;
    }

    static std::shared_ptr<Window> getWindow(GLFWwindow *window) {
        return _win_map[window];
    }

    static void window_focus_callback(GLFWwindow *window, int focused) {
        auto win = getWindow(window);
        // active
        // focus
        if (focused) {
            win->OnActive();
            win->OnFocus();
        } else {
            win->OnDeactive();
            win->OnFocusLost();
        }
    }

    static void cursor_position_callback(GLFWwindow *window, double xpos,
                                         double ypos) {
        auto win = getWindow(window);
        static std::pair<int, int> point = {xpos, ypos};
        for (int btn = GLFW_MOUSE_BUTTON_1; btn < GLFW_MOUSE_BUTTON_8; btn++) {
            int state = glfwGetMouseButton(window, btn);
            if (state == GLFW_PRESS) {
                if (point.first != xpos || point.second != ypos) {
                    win->OnMouseMove(static_cast<MouseBtn>(btn), xpos, ypos);
                    point = {xpos, ypos};
                }
            }
        }
    }

    static void scroll_callback(GLFWwindow *window, double xoffset,
                                double yoffset) {
        getWindow(window)->OnMouseWheel(yoffset);
    }

    static void mouse_button_callback(GLFWwindow *window, int button,
                                      int action, int mods) {
        auto win = getWindow(window);
        typedef std::chrono::system_clock::time_point time;
        typedef int btn_t;
        static std::map<btn_t, time> btn_before;

        auto btn = static_cast<MouseBtn>(button);

        if (action == GLFW_RELEASE) {
            if (btn_before.find(button) == btn_before.end()) {
                btn_before[button] = std::chrono::system_clock::now();
            }
            int i = GLFW_MOUSE_BUTTON_1;
            auto before = btn_before[button];
            auto now = std::chrono::system_clock::now();
            double diff_ms =
                std::chrono::duration<double, std::milli>(now - before).count();
            btn_before[button] = now;
            // TODO: def const
            if (diff_ms > 10 && diff_ms < 200) {
                win->OnMouseDoubleClick(btn);
            } else {
                win->OnMouseClick(btn);
            }
            win->OnMouseRelease(btn);
        } else if (action == GLFW_PRESS) {
            win->OnMousePress(btn);
        }
    }

    static void key_callback(GLFWwindow *window, int key, int scancode,
                             int action, int mods) {
        auto win = getWindow(window);
        auto k = static_cast<Key>(key);
        if (action == GLFW_PRESS) {
            win->OnKeyPress(k);
        } else if (action == GLFW_RELEASE) {
            win->OnKeyRelease(k);
        } else if (action == GLFW_REPEAT) {
            win->OnKeyRepeat(k);
        }
    }
    static void character_callback(GLFWwindow *window, unsigned int codepoint) {
        auto win = getWindow(window);
        win->OnCharacter(codepoint);
    }

    static void framebuffer_size_callback(GLFWwindow *window, int width,
                                          int height) {
        auto win = getWindow(window);
        win->SetFrameBufferSize(width, height);
        win->onFrameBufferResize(width, height);
    }

    static void window_close_callback(GLFWwindow *window) {
        auto win = getWindow(window);
        win->OnClose();
    }

    static void window_size_callback(GLFWwindow *window, int width,
                                     int height) {
        auto win = getWindow(window);
        win->SetWindowSize(width, height);
        win->OnResize(width, height);
    }
};

std::map<GLFWwindow *, std::shared_ptr<Window>> GLFWWindowManager::_win_map;
} // namespace GLFW

WindowManager *GetWindowManager() {
    static Utils::Singleton<GLFW::GLFWWindowManager> singleton;
    return singleton.get();
}
