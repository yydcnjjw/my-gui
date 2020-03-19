#pragma once
#include <vulkan/vulkan.hpp>
namespace my {
template <class T> class VulkanBuffer {
    T handle;
    vk::UniqueDeviceMemory mapping;    
};
} // namespace my
