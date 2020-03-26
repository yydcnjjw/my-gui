#pragma once

#include "vulkan_ctx.h"
#include "event_bus.hpp"

namespace my {
class Window : public VulkanPlatformSurface {
  public:
    virtual ~Window() = default;

    virtual std::pair<uint32_t, uint32_t> get_frame_buffer_size() = 0;

    [[nodiscard]] virtual std::vector<const char *>
    get_require_surface_extension() const = 0;

    [[nodiscard]] virtual vk::SurfaceKHR
    get_surface(vk::Instance instance) const = 0;
};

class WindowMgr {
  public:
    virtual ~WindowMgr() = default;

    // TODO: 考虑使用 window mgr 管理 window
    // return Window * 而不是 std::shared_ptr
    virtual Window *create_window(const std::string &title, uint32_t w,
                                  uint32_t h) = 0;
    virtual void remove_window(Window *) = 0;
};

WindowMgr *get_window_mgr(EventBus *);

} // namespace my
