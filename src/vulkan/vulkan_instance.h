#pragma once

#include "MyVulkan.h"
#include <memory>
#include <vector>

class VulkanInstance {
  public:
    VulkanInstance(const std::shared_ptr<Window> &win);
    ~VulkanInstance();

    const VkInstance &Get() {
        return _instance;
    }

  private:
    std::shared_ptr<Window> _window;
    VkInstance _instance = VK_NULL_HANDLE;
    std::vector<const char *> _instance_extensions;
};
