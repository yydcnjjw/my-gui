#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace my {

class VulkanPlatformSurface {
  public:
    virtual ~VulkanPlatformSurface() = default;

    [[nodiscard]] virtual std::vector<const char *>
    get_require_surface_extension() const = 0;
    [[nodiscard]] virtual vk::SurfaceKHR get_surface(vk::Instance) const = 0;
};

class VulkanCtx {
  public:
    VulkanCtx() = default;
    virtual ~VulkanCtx() = default;

    virtual void draw() = 0;
};
class Window;
std::shared_ptr<VulkanCtx> make_vulkan_ctx(std::shared_ptr<Window>);

} // namespace my
