#include "window_mgr.h"

// #define GLFW_INCLUDE_VULKAN
// #include <GLFW/glfw3.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "logger.h"
#include "util.hpp"
#include "vulkan_ctx.h"

namespace {

class SDLWindow : public my::Window {
  public:
    SDLWindow(SDL_Window *win) : _sdl_window(win) {}

    ~SDLWindow() { SDL_DestroyWindow(this->_sdl_window); }

    std::pair<uint32_t, uint32_t> get_frame_buffer_size() override {
        // TODO: gl vulkan
        int w, h;
        SDL_Vulkan_GetDrawableSize(this->_sdl_window, &w, &h);
        return {w, h};
    }

    [[nodiscard]] virtual std::vector<const char *>
    get_require_surface_extension() const override {
        unsigned int count = 0;
        if (!SDL_Vulkan_GetInstanceExtensions(this->_sdl_window, &count,
                                              nullptr)) {
            throw std::runtime_error(SDL_GetError());
        }

        std::vector<const char *> extensions(count);

        if (!SDL_Vulkan_GetInstanceExtensions(this->_sdl_window, &count,
                                              extensions.data())) {
            throw std::runtime_error(SDL_GetError());
        }

        return extensions;
    }

    [[nodiscard]] virtual vk::SurfaceKHR
    get_surface(vk::Instance instance) const override {
        VkSurfaceKHR s;
        if (!SDL_Vulkan_CreateSurface(this->_sdl_window, instance, &s)) {
            throw std::runtime_error(SDL_GetError());
        }
        return s;
    }

  private:
    SDL_Window *_sdl_window;
};

class SDLWindowMgr : public my::WindowMgr {

  public:
    SDLWindowMgr(my::EventBus *bus) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
            throw std::runtime_error(SDL_GetError());
        }

        this->_win_mgr_thread = std::thread([bus]() {
            pthread_setname_np(pthread_self(), "win mgr");
            GLOG_D("window loop running!");
            bool is_exit = false;
            while (!is_exit) {
                SDL_Event sdl_event{};
                SDL_WaitEvent(&sdl_event);

                switch (sdl_event.type) {
                case SDL_QUIT:
                    bus->notify<my::QuitEvent>();
                    is_exit = true;
                    break;
                case SDL_MOUSEMOTION:
                    bus->notify<my::MouseMotionEvent>(
                        {my::PixelPos{sdl_event.motion.x, sdl_event.motion.y},
                         sdl_event.motion.xrel, sdl_event.motion.yrel,
                         sdl_event.motion.state});
                    break;
                case SDL_KEYUP:
                case SDL_KEYDOWN:
                    bus->notify<my::KeyboardEvent>({sdl_event.key.state,
                                                    sdl_event.key.repeat != 0,
                                                    sdl_event.key.keysym});
                    break;
                default:
                    break;
                }
            }
            GLOG_D("window loop exit!");
        });
    }

    ~SDLWindowMgr() {
        SDL_Quit();
        if (this->_win_mgr_thread.joinable()) {
            this->_win_mgr_thread.join();
        }
    }
    my::Window *create_window(const std::string &title, uint32_t w,
                              uint32_t h) override {
        // TODO: 考虑 继承
        SDL_Window *sdl_win = SDL_CreateWindow(
            title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w,
            h,
            SDL_WINDOW_VULKAN | SDL_WINDOW_UTILITY | SDL_WINDOW_ALLOW_HIGHDPI);

        if (sdl_win == nullptr) {
            throw std::runtime_error(SDL_GetError());
        }
        auto win = std::make_unique<SDLWindow>(sdl_win);
        auto win_ptr = win.get();
        windows[win_ptr] = std::move(win);
        return win_ptr;
    }

    void remove_window(my::Window *win) override { windows.erase(win); }

  private:
    std::map<my::Window *, std::unique_ptr<SDLWindow>> windows;
    std::thread _win_mgr_thread;
};

} // namespace

namespace my {
WindowMgr *get_window_mgr(EventBus *ev_bus) {
    static auto window_mgr = SDLWindowMgr(ev_bus);
    return &window_mgr;
}
} // namespace my

// class GlfwWindow : public my::VulkanPlatformSurface {
//   public:
//     GlfwWindow() {
//         ::glfwInit();
//         ::glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

//         this->_glfw_window =
//             ::glfwCreateWindow(800, 600, "Test", nullptr, nullptr);
//     }

//     ~GlfwWindow() {
//         ::glfwDestroyWindow(this->_glfw_window);
//         ::glfwTerminate();
//     }

//     bool is_should_close() {
//         return ::glfwWindowShouldClose(this->_glfw_window);
//     }

//     std::pair<uint32_t, uint32_t> get_frame_buffer_size() {
//         int w, h;
//         ::glfwGetFramebufferSize(this->_glfw_window, &w, &h);
//         return {w, h};
//     }

//     void swap_buffers() { ::glfwSwapBuffers(this->_glfw_window); }

//     void poll_events() { ::glfwPollEvents(); }

//     [[nodiscard]] std::vector<const char *>
//     get_require_surface_extension() const override {
//         uint32_t count = 0;
//         auto extensions = ::glfwGetRequiredInstanceExtensions(&count);
//         return std::vector(extensions, extensions + count);
//     }

//     [[nodiscard]] vk::SurfaceKHR
//     get_surface(vk::Instance instance) const override {
//         VkSurfaceKHR s;
//         if (::glfwCreateWindowSurface(instance, this->_glfw_window, nullptr,
//                                       &s) != VK_SUCCESS) {
//             throw std::runtime_error("failed to create window surface");
//         }
//         return vk::SurfaceKHR(s);
//     }

//   private:
//     GLFWwindow *_glfw_window;
// };
