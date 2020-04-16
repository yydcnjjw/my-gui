#pragma once

#include <render/device.h>
#include <render/vulkan/instance.hpp>
#include <render/vulkan/queue.hpp>

namespace my {
namespace render {
namespace vulkan {
// uint32_t find_mem_type(uint32_t type_filter,
//                                  const vk::MemoryPropertyFlags &properties) {
//     auto mem_properties = this->_physical_device.getMemoryProperties();
//     for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
//         if ((type_filter & (1UL << i)) &&
//             (mem_properties.memoryTypes[i].propertyFlags & properties) ==
//                 properties) {
//             return i;
//         }
//     }

//     throw std::runtime_error("failed to find suitable memory type!");
// }

class Device : public render::Device {
  public:
    Device(vk::PhysicalDevice &physical_device, vk::UniqueSurfaceKHR &&surface)
        : _physical_device(physical_device), _surface(std::move(surface)) {

        this->_queue.collect(physical_device, surface.get());
        
        auto queue_create_infos = this->_queue.build_device_queue_create_info();

        vk::PhysicalDeviceFeatures device_features = {};
        device_features.setSamplerAnisotropy(true);

        std::vector<const char *> require_deivce_extensions{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        vk::DeviceCreateInfo create_info(
            {}, queue_create_infos.size(), queue_create_infos.data(), {}, {},
            require_deivce_extensions.size(), require_deivce_extensions.data(),
            &device_features);

        this->_device = this->_physical_device.createDeviceUnique(create_info);

        this->_queue.set_queues(this->_device.get());
    }

  private:
    vk::PhysicalDevice _physical_device;
    vk::UniqueSurfaceKHR _surface;
    vk::UniqueDevice _device;

    Queue _queue;
};

} // namespace vulkan
} // namespace render
} // namespace my
