#pragma once

#include <vulkan/vulkan.hpp>

namespace my {

class VulkanPlatformSurface {
  public:
    virtual ~VulkanPlatformSurface() = default;

    [[nodiscard]] virtual std::vector<const char *>
    get_require_surface_extension() const = 0;
    [[nodiscard]] virtual vk::SurfaceKHR get_surface(vk::Instance) const = 0;
};

enum VulkanQueueType {
    VK_QUEUE_GRAPHICS,
    VK_QUEUE_COMPUTE,
    VK_QUEUE_TRANSFER,
    VK_QUEUE_SPARSE_BINDING,
    VK_QUEUE_PROTECTED,
    VK_QUEUE_PRESENT
};

enum VulkanCommandPoolType { VK_COMMAND_POOL_COMMON, VK_COMMAND_POOL_MEM };

struct VulkanSwapchain {
    size_t image_count;
    vk::UniqueSwapchainKHR handle;
    std::vector<vk::Image> images;
    vk::Format format;
    vk::Extent2D extent;
    vk::PresentModeKHR present_mode;

};
    
class RenderDevice;
class VulkanCtx {
  public:
    VulkanCtx() = default;
    virtual ~VulkanCtx() = default;
    
    virtual const vk::Device &get_device() = 0;
    virtual const vk::PhysicalDevice &get_physical_device() = 0;
    
    virtual uint32_t
    find_memory_type(uint32_t type_filter,
                     const vk::MemoryPropertyFlags &properties) = 0;
    
    virtual vk::Format get_depth_format() const = 0;
    
    virtual const vk::Queue &get_queue(const VulkanQueueType &) = 0;
    
    virtual const vk::CommandPool &
    get_command_pool(const VulkanCommandPoolType &) = 0;
    
    virtual const VulkanSwapchain &get_swapchain() const = 0;
    
    virtual void prepare_buffer() = 0;
    
    virtual void sumit_command_buffer(const vk::CommandBuffer &cb) = 0;
    
    virtual void swap_buffer() = 0;
    virtual const uint32_t &get_current_buffer_index() const = 0;

    virtual vk::SampleCountFlagBits get_max_usable_sample_count() const = 0;
};
class Window;
std::shared_ptr<VulkanCtx> make_vulkan_ctx(const std::shared_ptr<Window> &);

} // namespace my
