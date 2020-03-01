#pragma once

#include "window.h"
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#define VULKAN_DEBUG

namespace VK {
    
}

class NonCopyable {
    constexpr NonCopyable(const NonCopyable &) = delete;
    constexpr NonCopyable &operator=(const NonCopyable &) = delete;

  protected:
    constexpr NonCopyable() noexcept = default;
    constexpr NonCopyable(NonCopyable &&) noexcept = default;
    constexpr NonCopyable &operator=(NonCopyable &&) noexcept = default;
};

class NonMovable {
    constexpr NonMovable(const NonMovable &) = delete;
    constexpr NonMovable &operator=(const NonMovable &) = delete;
    constexpr NonMovable(NonMovable &&) = delete;
    constexpr NonMovable &operator=(NonMovable &&) = delete;

  protected:
    constexpr NonMovable() noexcept = default;
};

template <typename T> class Handle {
  public:
    Handle() : _handle(VK_NULL_HANDLE) {}
    Handle(T handle) : _handle(handle) {}
    Handle(const Handle<T> &) = default;
    Handle(Handle<T> &&copy) : _handle(copy._handle) {
        copy._handle = VK_NULL_HANDLE;
    }

    Handle &operator=(const Handle<T> &) = delete;
    Handle &operator=(Handle<T> &&copy) {
        _handle = copy._handle;
        copy._handle = VK_NULL_HANDLE;
        return *this;
    }

    operator T &() { return _handle; }
    operator const T &() const { return _handle; }
    T *operator&() { return &_handle; }
    const T *operator&() const { return &_handle; }

  private:
    T _handle;
};

namespace VulkanUtil {
const std::vector<VkLayerProperties> &GetAvailableLayers();
const std::vector<const char *> &GetValidationLayers();
} // namespace VulkanUtil

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3>
    getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions =
            {};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
