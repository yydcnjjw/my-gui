#pragma once

#include <render/queue.h>
#include <render/vulkan/device.hpp>

namespace my {
namespace render {
namespace vulkan {

struct QueueInfo {
    vk::Queue queue;
    std::optional<uint32_t> queue_family_index;
    uint32_t queue_index = 0;
};

class Queue : public render::Queue {
  public:
    Queue() {}

    void present() override {
        
    }
    void submit() override {
        
    }
    void wait_idle() override {
        
    }

    void collect(vk::PhysicalDevice physical_device,
                 vk::SurfaceKHR surface) {

        auto queue_families = physical_device.getQueueFamilyProperties();

        int i = 0;
        for (const auto &queue_family : queue_families) {
            if ((queue_family.queueFlags & vk::QueueFlagBits::eGraphics) &&
                !this->_queues[QUEUE_GRAPHICS].queue_family_index.has_value()) {
                this->_queues[QUEUE_GRAPHICS].queue_family_index = i;
            }
            if ((queue_family.queueFlags & vk::QueueFlagBits::eTransfer) &&
                !this->_queues[QUEUE_TRANSFER].queue_family_index.has_value()) {
                this->_queues[QUEUE_TRANSFER].queue_family_index = i;
            }

            if (physical_device.getSurfaceSupportKHR(i, surface) &&
                !this->_queues[QUEUE_PRESENT].queue_family_index.has_value()) {
                this->_queues[QUEUE_PRESENT].queue_family_index = i;
            }
            i++;
        }

        for (const auto &info : this->_queues) {
            if (!info.second.queue_family_index.has_value()) {
                throw std::runtime_error("collect device queue info failure");
            }
        }
    }

    std::vector<vk::DeviceQueueCreateInfo> build_device_queue_create_info() {
        std::set<uint32_t> unique_queue_families;

        std::transform(
            this->_queues.begin(), this->_queues.end(),
            std::inserter(unique_queue_families, unique_queue_families.end()),
            [](const auto &queue) {
                return queue.second.queue_family_index.value();
            });

        std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
        float queue_priority = 1.0f;
        for (uint32_t index : unique_queue_families) {
            vk::DeviceQueueCreateInfo info({}, index, 1, &queue_priority);
            queue_create_infos.push_back(info);
        }
        return queue_create_infos;
    }

    void set_queues(vk::Device &device) {
        for (auto &queue : this->_queues) {
            queue.second.queue =
                device.getQueue(queue.second.queue_family_index.value(),
                                queue.second.queue_index);
#ifdef MY_DEBUG
            const char *queue_type;
            switch (queue.first) {
            case QUEUE_COMPUTE: {
                queue_type = "compute";
                break;
            }
            case QUEUE_PRESENT: {
                queue_type = "present";
                break;
            }
            case QUEUE_GRAPHICS: {
                queue_type = "graphics";
                break;
            }
            case QUEUE_TRANSFER: {
                queue_type = "transfer";
                break;
            }
            }
            GLOG_D("create %s queue family index: %d, index: %d", queue_type,
                   queue.second.queue_family_index.value(),
                   queue.second.queue_index);

#endif // MY_DEBUG
        }
    }

  private:
    std::map<QueueType, QueueInfo> _queues = {
        {QUEUE_GRAPHICS, {}}, {QUEUE_TRANSFER, {}}, {QUEUE_PRESENT, {}}};
};
} // namespace vulkan
} // namespace render
} // namespace my
