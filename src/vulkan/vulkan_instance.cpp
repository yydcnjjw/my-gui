#include "vulkan_instance.h"

namespace {

VkApplicationInfo initApplicationInfo() {
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    return appInfo;
}
} // namespace

VulkanInstance::VulkanInstance(const std::shared_ptr<Window> &win)
    : _window(win) {
    auto app_info = initApplicationInfo();
    VkInstanceCreateInfo createInfo = {};

    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &app_info;

    if (_window != nullptr) {
        auto extensions = GetWindowManager()->GetRequiredExtensions();
        _instance_extensions.insert(_instance_extensions.end(),
                                    extensions.begin(), extensions.end());
    }

#ifdef VULKAN_DEBUG
    _instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(_instance_extensions.size());
    createInfo.ppEnabledExtensionNames = _instance_extensions.data();

#ifdef VULKAN_DEBUG
    auto &validation_layers = VulkanUtil::GetValidationLayers();
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validation_layers.size());
    createInfo.ppEnabledLayerNames = validation_layers.data();
#else
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
#endif

    if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

VulkanInstance::~VulkanInstance() { vkDestroyInstance(_instance, nullptr); }
