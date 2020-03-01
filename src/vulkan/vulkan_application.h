#pragma once

#include "MyVulkan.h"
#include "vulkan_instance.h"

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "window.h"

class VulkanApplication {
  public:
    VulkanApplication(const std::shared_ptr<Window> &window);
    ~VulkanApplication();
    static const std::vector<VkLayerProperties> &GetAvailableLayers();
    static const std::vector<VkExtensionProperties> &GetSupportedExtension();
    const std::vector<VkPhysicalDevice> &GetAvailablePhysicalDevice();

    void Draw();

  private:
    std::shared_ptr<Window> _window;
    VulkanInstance _instance;

    VkSurfaceKHR _surface;
    void createSurface();

    // NOTE: the object will be implicitly destroyed when the VKInstance is
    // destroyed
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    void choosePhysicalDevice();
    bool isDeviceSuitable(const VkPhysicalDevice &device);

    VkSwapchainKHR _swapchain;
    std::vector<VkImage> _swapchainImages;
    VkFormat _swapchainImageFormat;
    VkExtent2D _swapchainExtent;
    void createSwapchain();
    void cleanupSwapchain();

    std::vector<VkImageView> _swapchainImageViews;
    void createImageViews();

    std::vector<VkFramebuffer> _swapchainFramebuffers;
    void createCommandBuffers();
    VkCommandPool _commandPool;
    VkCommandPool _memPool;
    void createCommandPool();
    std::vector<VkCommandBuffer> _commandBuffers;
    void createFramebuffers();

    VkRenderPass _renderPass;
    void createRenderPass();

    VkPipelineLayout _pipelineLayout;
    void createGraphicsPipeline();
    VkShaderModule createShaderModule(const std::vector<char> &code);
    bool isFramebufferResized();
    VkPipeline _graphicsPipeline;

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    void createSyncObjects();
    std::vector<VkFence> _inFlightFences;
    size_t _currentFrame = 0;
    void recreateSwapchain();

    enum VulkanQueueType {
        VK_QUEUE_GRAPHICS,
        VK_QUEUE_COMPUTE,
        VK_QUEUE_TRANSFER,
        VK_QUEUE_SPARSE_BINDING,
        VK_QUEUE_PROTECTED,
        VK_QUEUE_PRESENT,
        VK_QUEUE_NONE
    };
    std::map<VulkanQueueType, std::pair<VkQueue, VkDeviceQueueInfo2>>
        _deviceQueues;
    void createLogicalDevice();
    std::vector<std::vector<VkDeviceQueueInfo2>> collectDeviceQueueInfo();

    // debug messenger
    void populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    void enableDebugMessenger();
    VkDebugUtilsMessengerEXT debugMessenger;
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                  void *pUserData);

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties, VkBuffer &buffer,
                      VkDeviceMemory &bufferMemory);

    VkBuffer _indexBuffer;
    VkDeviceMemory _indexBufferMemory;
    void createIndexBuffer();
    VkBuffer _vertexBuffer;
    VkDeviceMemory _vertexBufferMemory;
    void createVertexBuffer();

    uint32_t findMemoryType(uint32_t typeFilter,
                            VkMemoryPropertyFlags properties);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    std::vector<VkBuffer> _uniformBuffers;
    std::vector<VkDeviceMemory> _uniformBuffersMemory;
    VkDescriptorSetLayout _descriptorSetLayout;

    void createDescriptorSetLayout();
    void createUniformBuffers();
    void updateUniformBuffer(uint32_t currentImage);

    VkDescriptorPool _descriptorPool;
    std::vector<VkDescriptorSet> _descriptorSets;
    void createDescriptorPool();
    void createDescriptorSets();
    std::vector<VkFence> _imagesInFlight;

    void createTextureImage();
    void createImage(uint32_t width, uint32_t height, VkFormat format,
                     VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkImage &image,
                     VkDeviceMemory &imageMemory);

    VkCommandBuffer beginSingleTimeCommands(VkCommandPool &commandPool);
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void endSingleTimeCommands(VkCommandPool &commandPool,
                               VkCommandBuffer &commandBuffer,
                               VulkanQueueType type);
    void transitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                           uint32_t height);
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkImageView createImageView(VkImage image, VkFormat format);
    void createTextureImageView();

    VkSampler textureSampler;
    void createTextureSampler();
};
