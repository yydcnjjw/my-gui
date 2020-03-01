#include "MyVulkan.h"

#include <cstring>

namespace VulkanUtil {
const std::vector<VkLayerProperties> &GetAvailableLayers() {
    static std::unique_ptr<std::vector<VkLayerProperties>> availableLayers;
    if (!availableLayers) {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        availableLayers.reset(new std::vector<VkLayerProperties>(layerCount));
        vkEnumerateInstanceLayerProperties(&layerCount,
                                           availableLayers->data());
    }
    return *availableLayers;
}

bool checkValidationLayerSupport(
    const std::vector<const char *> &validation_layers) {
    auto &availableLayers = GetAvailableLayers();
    for (const char *layerName : validation_layers) {
        bool layerFound = false;
        for (const auto &layerProperties : availableLayers) {
            if (::strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            return false;
        }
    }
    return true;
}

const std::vector<const char *> &GetValidationLayers() {
    static const std::vector<const char *> validation_layers = {
        "VK_LAYER_KHRONOS_validation"};

    if (checkValidationLayerSupport(validation_layers)) {
        return validation_layers;
    } else {
        throw std::runtime_error("check validation layer support failure");
    }
}

} // namespace VulkanUtil
