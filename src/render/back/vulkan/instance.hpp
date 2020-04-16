#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <my_gui.h>
#include <render/instance.h>
#include <render/vulkan/device.hpp>
#include <render/window/window_mgr.h>
#include <util/logger.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace {
void vk_dynamic_loader_init() {
    vk::DynamicLoader dl;
    auto vkGetInstanceProcAddr =
        dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
}

bool validation_layer_support(const std::vector<const char *> &layers) {
    auto available_layers = vk::enumerateInstanceLayerProperties();
    for (const auto &layer : layers) {
        auto result = std::find_if(
            available_layers.cbegin(), available_layers.cend(),
            [&layer](const auto &available_layer) {
                return !std::strcmp(available_layer.layerName, layer);
            });
        if (result == std::cend(available_layers)) {
            GLOG_D("validation layer: %s is not available!", layer);
            return false;
        }
    }
    return true;
}

VkBool32
vk_debug_msg_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                      VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                      void *) {
    auto level = vk::to_string(
        vk::DebugUtilsMessageSeverityFlagBitsEXT(messageSeverity));
    auto type = vk::to_string(vk::DebugUtilsMessageTypeFlagsEXT(messageTypes));

    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        GLOG_E("[%s:%s]: %s", level.c_str(), type.c_str(),
               pCallbackData->pMessage);
    } else {
        GLOG_D("[%s:%s]: %s", level.c_str(), type.c_str(),
               pCallbackData->pMessage);
    }
    return VK_FALSE;
}

} // namespace

namespace my {
namespace render {
namespace vulkan {

class Instance;

class Platform {
  public:
    virtual ~Platform() = default;
    virtual std::vector<const char *> require_extension() = 0;
    virtual vk::SurfaceKHR create_surface(Window *, const Instance &) = 0;
    static std::shared_ptr<Platform> create();
};

class Instance : public render::Instance {
  public:
    Instance() : _platform(Platform::create()) {
        vk_dynamic_loader_init();
        this->_create_vk_instance();
        this->_pick_physical_device();
    }

    std::shared_ptr<render::Device> make_device(Window *win) {
        vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>
            _deleter(this->_vk_instance.get());
        return std::make_shared<Device>(
            this->_physical_device,
            vk::UniqueSurfaceKHR(this->_platform->create_surface(win, *this),
                                 _deleter));
    }

    vk::UniqueInstance _vk_instance;
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic>
        _debug_messenger;
    vk::PhysicalDevice _physical_device;

    friend class Platform;
    std::shared_ptr<Platform> _platform;

    void _create_vk_instance() {
        const std::vector<const char *> require_validation_layers{
#ifdef MY_DEBUG
            "VK_LAYER_KHRONOS_validation"
#endif // MY_DEBUG
        };

        if (!validation_layer_support(require_validation_layers)) {
            throw std::runtime_error("validation layers is not available!");
        }

        std::vector<const char *> require_extensions{
#ifdef MY_DEBUG
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif // MY_DEBUG
        };

        auto platform_require_extensions = this->_platform->require_extension();
        require_extensions.insert(require_extensions.end(),
                                  platform_require_extensions.begin(),
                                  platform_require_extensions.end());

#ifdef MY_DEBUG
        GLOG_D("require validation layers: ");
        std::for_each(require_validation_layers.begin(),
                      require_validation_layers.end(),
                      [](const char *layer_name) { GLOG_D(layer_name); });
        GLOG_D("require extensions: ");
        std::for_each(
            require_extensions.begin(), require_extensions.end(),
            [](const char *extension_name) { GLOG_D(extension_name); });
#endif // MY_DEBUG

        vk::InstanceCreateInfo create_info(
            {}, {}, require_validation_layers.size(),
            require_validation_layers.data(), require_extensions.size(),
            require_extensions.data());

        this->_vk_instance = vk::createInstanceUnique(create_info);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(this->_vk_instance.get());

#ifdef MY_DEBUG

        vk::DebugUtilsMessengerCreateInfoEXT debug_create_info(
            {},
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                // vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                // vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
            vk_debug_msg_callback);

        this->_debug_messenger =
            this->_vk_instance->createDebugUtilsMessengerEXTUnique(
                debug_create_info);

#endif // MY_DEBUG
    }

    void _pick_physical_device() {
        auto physical_device = this->_vk_instance->enumeratePhysicalDevices();
        if (physical_device.empty()) {
            throw std::runtime_error(
                "failed to find GPUs with Vulkan support!");
        }
        // TODO: pick suitable device
        // for (const auto &device : physical_device) {

        // }

        this->_physical_device = physical_device.front();
    }
};

} // namespace vulkan
} // namespace render
} // namespace my
