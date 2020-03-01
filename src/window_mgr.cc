#include "window_mgr.h"

// #define GLFW_INCLUDE_VULKAN
// #include <GLFW/glfw3.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

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
    SDLWindowMgr() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
            throw std::runtime_error(SDL_GetError());
        }

        auto thread = rxcpp::serialize_event_loop();
        rxcpp::observable<>::create<std::shared_ptr<my::Event>>(
            [](rxcpp::subscriber<std::shared_ptr<my::Event>> s) {
                for (;;) {
                    SDL_Event sdl_event = {};
                    SDL_PollEvent(&sdl_event);

                    my::Event *event = nullptr;
                    switch (sdl_event.type) {
                    case SDL_QUIT:
                        event = new my::Event{my::EventType::EVENT_QUIT,
                                              sdl_event.quit.timestamp};
                        break;
                    case SDL_MOUSEMOTION:
                        event = new my::MouseMotionEvent{
                            my::EventType::EVENT_MOUSE_MOTION,
                            sdl_event.motion.timestamp,
                            {sdl_event.motion.x, sdl_event.motion.y},
                            sdl_event.motion.xrel,
                            sdl_event.motion.yrel,
                            sdl_event.motion.state,
                        };
                        break;
                    case SDL_KEYUP:
                    case SDL_KEYDOWN:
                        event = new my::KeyboardEvent{
                            my::EventType::EVENT_KEYBOARD,
                            sdl_event.key.timestamp, sdl_event.key.state,
                            sdl_event.key.repeat != 0, sdl_event.key.keysym};
                        break;
                    default:
                        break;
                    }
                    if (!event) {
                        continue;
                    }
                    s.on_next(std::shared_ptr<my::Event>(event));                   
                    if (event->type == my::EventType::EVENT_QUIT) {
                        s.on_completed();
                        break;
                    }
                }
            })
            .subscribe_on(thread)
            .observe_on(thread)
            .subscribe([this](const std::shared_ptr<my::Event> &e) {
                this->_sdl_event.get_subscriber().on_next(e);
            });
    }

    ~SDLWindowMgr() { SDL_Quit(); }
    std::shared_ptr<my::Window> create_window(const std::string &title,
                                              uint32_t w, uint32_t h) override {
        // TODO: 考虑 继承
        SDL_Window *win = SDL_CreateWindow(
            title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w,
            h, SDL_WINDOW_VULKAN | SDL_WINDOW_UTILITY);

        if (win == nullptr) {
            throw std::runtime_error(SDL_GetError());
        }

        return std::make_shared<SDLWindow>(win);
    }

    void remove_window(std::shared_ptr<my::Window>) override {}

    rxcpp::observable<std::shared_ptr<my::Event>> get_observable() override {
        return _sdl_event.get_observable();
    };

  private:
    rxcpp::subjects::subject<std::shared_ptr<my::Event>> _sdl_event;
};

} // namespace

namespace my {
WindowMgr *get_window_mgr() {
    static auto window_mgr = new SDLWindowMgr();
    return window_mgr;
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
