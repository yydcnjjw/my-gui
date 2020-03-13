#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include "vulkan_ctx.h"

#include "env.h"
#include "logger.h"
#include "render_device.h"
#include "window_mgr.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace {
using namespace my;

constexpr int MAX_FRAMES_IN_FLIGHT = 1;

class MyVulkanCtx : public my::VulkanCtx {

  public:
    explicit MyVulkanCtx(my::Window *win) : _platform_win(win) {
        this->_init();
    }

    ~MyVulkanCtx() override { this->_device->waitIdle(); };

    const vk::Device &get_device() override { return this->_device.get(); }

    const vk::PhysicalDevice &get_physical_device() override {
        return this->_physical_device;
    }

    const vk::Queue &get_queue(const VulkanQueueType &type) override {
        return this->_queues[type].queue;
    }

    const vk::CommandPool &
    get_command_pool(const VulkanCommandPoolType &type) override {
        switch (type) {
        case VK_COMMAND_POOL_COMMON:
            return this->_common_command_pool.get();
        case VK_COMMAND_POOL_MEM:
            return this->_mem_command_pool.get();
        default:
            return this->_common_command_pool.get();
        }
    };

    const VulkanSwapchain &get_swapchain() const override {
        return this->_swapchain;
    };

    const uint32_t &get_current_buffer_index() const override {
        return this->_current_buffer_index;
    };

    uint32_t
    find_memory_type(uint32_t type_filter,
                     const vk::MemoryPropertyFlags &properties) override {
        auto mem_properties = this->_physical_device.getMemoryProperties();
        for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
            if ((type_filter & (1UL << i)) &&
                (mem_properties.memoryTypes[i].propertyFlags & properties) ==
                    properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    vk::Format get_depth_format() const override {
        return this->_find_supported_format(
            {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,
             vk::Format::eD24UnormS8Uint},
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    }

    void prepare_buffer() override {
        auto &current_render_frence =
            this->_render_frences[this->_current_frame].get();
        this->_device->waitForFences({current_render_frence}, VK_TRUE,
                                     std::numeric_limits<uint64_t>::max());
        this->_device->resetFences(1, &current_render_frence);

        auto wait_semaphore = this->_img_available[this->_current_frame].get();

        try {
            auto result = this->_device->acquireNextImageKHR(
                this->_swapchain.handle.get(),
                std::numeric_limits<uint64_t>::max(), wait_semaphore, {});
            this->_current_buffer_index = result.value;
        } catch (vk::OutOfDateKHRError err) {
            // this->_recreate_swapchain();
            return;
        } catch (...) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }
    }

    void sumit_command_buffer(const vk::CommandBuffer &cb) override {
        auto &wait_semaphore = this->_img_available[this->_current_frame].get();
        auto &signal_semaphore =
            this->_render_finished[this->_current_frame].get();
        auto &current_render_frence =
            this->_render_frences[this->_current_frame].get();

        vk::PipelineStageFlags wait_stages[] = {
            vk::PipelineStageFlagBits::eColorAttachmentOutput};
        vk::SubmitInfo submit_info(1, &wait_semaphore, wait_stages, 1, &cb, 1,
                                   &signal_semaphore);

        this->_queues[VK_QUEUE_GRAPHICS].queue.submit({submit_info},
                                                      current_render_frence);
    }

    void swap_buffer() override {
        auto &signal_semaphore =
            this->_render_finished[this->_current_frame].get();

        vk::SwapchainKHR swapchains[] = {this->_swapchain.handle.get()};
        vk::PresentInfoKHR present_info(1, &signal_semaphore, 1, swapchains,
                                        &this->_current_buffer_index);
        vk::Result result;
        try {
            result =
                this->_queues[VK_QUEUE_PRESENT].queue.presentKHR(present_info);
        } catch (vk::OutOfDateKHRError) {
            result = vk::Result::eErrorOutOfDateKHR;
        } catch (...) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        if (this->_is_frame_buffer_resized()) {
            // this->_recreate_swapchain();
            return;
        }

        switch (result) {
        case vk::Result::eSuccess:
            break;
        case vk::Result::eErrorOutOfDateKHR:
        case vk::Result::eSuboptimalKHR:
            // this->_recreate_swapchain();
            return;
        default:
            throw std::runtime_error("failed to present swap chain image!");
        }

        this->_current_frame =
            (this->_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    vk::SampleCountFlagBits get_max_usable_sample_count() const override {
        auto properties = this->_physical_device.getProperties();

        vk::SampleCountFlags counts =
            properties.limits.framebufferColorSampleCounts &
            properties.limits.framebufferDepthSampleCounts;

        if (counts & vk::SampleCountFlagBits::e64) {
            return vk::SampleCountFlagBits::e64;
        }
        if (counts & vk::SampleCountFlagBits::e32) {
            return vk::SampleCountFlagBits::e32;
        }
        if (counts & vk::SampleCountFlagBits::e16) {
            return vk::SampleCountFlagBits::e16;
        }
        if (counts & vk::SampleCountFlagBits::e8) {
            return vk::SampleCountFlagBits::e8;
        }
        if (counts & vk::SampleCountFlagBits::e4) {
            return vk::SampleCountFlagBits::e4;
        }
        if (counts & vk::SampleCountFlagBits::e2) {
            return vk::SampleCountFlagBits::e2;
        }
        return vk::SampleCountFlagBits::e1;
    }

  private:
    bool _enable_validation_layers = false;
    std::vector<const char *> _require_validation_layers;
    std::vector<const char *> _require_extensions;
    std::vector<const char *> _require_deivce_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    vk::UniqueInstance _instance;
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic>
        _debug_messenger;

    my::Window *_platform_win;
    vk::UniqueSurfaceKHR _surface;

    vk::PhysicalDevice _physical_device;
    vk::UniqueDevice _device;

    VulkanSwapchain _swapchain;

    vk::UniqueCommandPool _common_command_pool;
    vk::UniqueCommandPool _mem_command_pool;

    std::vector<vk::UniqueSemaphore> _img_available;
    std::vector<vk::UniqueSemaphore> _render_finished;
    std::vector<vk::UniqueFence> _render_frences;

    size_t _current_frame = 0;
    uint32_t _current_buffer_index;

    bool _is_validation_layer_support() {
        auto available_layers = vk::enumerateInstanceLayerProperties();
        for (const auto &layer : _require_validation_layers) {
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

    void _create_instance() {
        if (this->_enable_validation_layers) {
            this->_require_validation_layers = {"VK_LAYER_KHRONOS_validation"};

            if (!_is_validation_layer_support()) {
                GLOG_E("validation layers is not available!");
                return;
            }
            this->_require_extensions.push_back(
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        auto platform_require_extensions =
            this->_platform_win->get_require_surface_extension();
        this->_require_extensions.insert(this->_require_extensions.end(),
                                         platform_require_extensions.begin(),
                                         platform_require_extensions.end());

#ifdef MY_DEBUG
        GLOG_D("require validation layers: ");
        std::for_each(this->_require_validation_layers.begin(),
                      this->_require_validation_layers.end(),
                      [](const char *layer_name) { GLOG_D(layer_name); });
        GLOG_D("require extensions: ");
        std::for_each(
            this->_require_extensions.begin(), this->_require_extensions.end(),
            [](const char *extension_name) { GLOG_D(extension_name); });
#endif
        vk::InstanceCreateInfo create_info(
            {}, {}, this->_require_validation_layers.size(),
            this->_require_validation_layers.data(),
            this->_require_extensions.size(), this->_require_extensions.data());

        this->_instance = vk::createInstanceUnique(create_info);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(this->_instance.get());
    }

    void _setup_debug_messenger() {
        if (!this->_enable_validation_layers)
            return;

        vk::DebugUtilsMessengerCreateInfoEXT debug_create_info(
            {},
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                // vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                // vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
            _debug_msg_callback);

        this->_debug_messenger =
            this->_instance->createDebugUtilsMessengerEXTUnique(
                debug_create_info);
    }

    void _create_surface() {
        vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>
            _deleter(this->_instance.get());
        this->_surface = vk::UniqueSurfaceKHR(
            this->_platform_win->get_surface(this->_instance.get()), _deleter);
    }

    void _pick_physical_device() {
        auto physical_device = this->_instance->enumeratePhysicalDevices();
        if (physical_device.empty()) {
            throw std::runtime_error(
                "failed to find GPUs with Vulkan support!");
        }

        for (const auto &device : physical_device) {
            // TODO: pick suitable device
        }

        this->_physical_device = physical_device.front();
    }

    struct queue_info {
        std::optional<uint32_t> queue_family_index;
        uint32_t queue_index = 0;
        vk::Queue queue;
    };

    std::map<VulkanQueueType, queue_info> _queues = {
        {VK_QUEUE_GRAPHICS, queue_info()},
        {VK_QUEUE_PRESENT, queue_info()},
        {VK_QUEUE_TRANSFER, queue_info()}};

    const vk::Queue &get_queue(VulkanQueueType type) {
        if (_queues.find(type) == _queues.end()) {
            throw std::runtime_error("device is not be created");
        }
        return _queues[type].queue;
    }

    void _collect_device_queue_info() {
        auto queue_families = this->_physical_device.getQueueFamilyProperties();

        int i = 0;
        for (const auto &queue_family : queue_families) {
            if ((queue_family.queueFlags & vk::QueueFlagBits::eGraphics) &&
                !this->_queues[VK_QUEUE_GRAPHICS]
                     .queue_family_index.has_value()) {
                this->_queues[VK_QUEUE_GRAPHICS].queue_family_index = i;
            }
            if ((queue_family.queueFlags & vk::QueueFlagBits::eTransfer) &&
                !this->_queues[VK_QUEUE_TRANSFER]
                     .queue_family_index.has_value()) {
                this->_queues[VK_QUEUE_TRANSFER].queue_family_index = i;
            }

            if (this->_physical_device.getSurfaceSupportKHR(
                    i, this->_surface.get()) &&
                !this->_queues[VK_QUEUE_PRESENT]
                     .queue_family_index.has_value()) {
                this->_queues[VK_QUEUE_PRESENT].queue_family_index = i;
            }
            i++;
        }

        for (const auto &info : this->_queues) {
            if (!info.second.queue_family_index.has_value()) {
                throw std::runtime_error("collect device queue info failure");
            }
        }
    }

    void _create_logical_device() {
        this->_collect_device_queue_info();

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

        vk::PhysicalDeviceFeatures device_features = {};
        device_features.setSamplerAnisotropy(true);

        vk::DeviceCreateInfo create_info(
            {}, queue_create_infos.size(), queue_create_infos.data(),
            this->_require_validation_layers.size(),
            this->_require_validation_layers.data(),
            this->_require_deivce_extensions.size(),
            this->_require_deivce_extensions.data(), &device_features);

        this->_device = this->_physical_device.createDeviceUnique(create_info);

        for (auto &queue : this->_queues) {
            queue.second.queue =
                this->_device->getQueue(queue.second.queue_family_index.value(),
                                        queue.second.queue_index);
            std::string queue_type;
            switch (queue.first) {
            case VK_QUEUE_COMPUTE: {
                queue_type = "compute";
                break;
            }
            case VK_QUEUE_PRESENT: {
                queue_type = "present";
                break;
            }
            case VK_QUEUE_GRAPHICS: {
                queue_type = "graphics";
                break;
            }
            case VK_QUEUE_TRANSFER: {
                queue_type = "transfer";
                break;
            }
            case VK_QUEUE_PROTECTED: {
                queue_type = "protected";
                break;
            }
            case VK_QUEUE_SPARSE_BINDING: {
                queue_type = "sparse binding";
                break;
            }
            }
            GLOG_D("create %s queue family index: %d, index: %d",
                   queue_type.c_str(), queue.second.queue_family_index.value(),
                   queue.second.queue_index);
        }
    }

    struct SwapchainSupportDetail {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> present_modes;

        SwapchainSupportDetail(const vk::PhysicalDevice &device,
                               const vk::SurfaceKHR &surface)
            : capabilities(device.getSurfaceCapabilitiesKHR(surface)),
              formats(device.getSurfaceFormatsKHR(surface)),
              present_modes(device.getSurfacePresentModesKHR(surface)) {}

        const vk::SurfaceFormatKHR &choose_surface_format() {
            auto choose_format = std::find_if(
                this->formats.begin(), this->formats.end(),
                [](const vk::SurfaceFormatKHR &format) {
                    return format.format == vk::Format::eR8G8B8A8Unorm &&
                           format.colorSpace ==
                               vk::ColorSpaceKHR::eSrgbNonlinear;
                });
            if (choose_format != this->formats.end()) {
                return *choose_format;
            } else {
                return this->formats.front();
            }
        }

        const vk::PresentModeKHR &choose_present_mode() {
            auto ret = std::find(this->present_modes.begin(),
                                 this->present_modes.end(),
                                 vk::PresentModeKHR::eMailbox);
            if (ret != this->present_modes.end()) {
                return *ret;
            }

            ret = std::find(this->present_modes.begin(),
                            this->present_modes.end(),
                            vk::PresentModeKHR::eImmediate);

            if (ret != this->present_modes.end()) {
                return *ret;
            }

            static auto default_mode = vk::PresentModeKHR::eFifo;
            return default_mode;
        }

        vk::Extent2D choose_extent(my::Window *win) {
            if (capabilities.currentExtent.width !=
                std::numeric_limits<uint32_t>::max()) {
                return capabilities.currentExtent;
            } else {
                auto [w, h] = win->get_frame_buffer_size();
                vk::Extent2D actual_extent(
                    std::max(capabilities.maxImageExtent.width, w),
                    std::max(capabilities.maxImageExtent.height, h));
                return actual_extent;
            }
        }
    };

    void _create_swapchain() {
        GLOG_D("swapchain creating ...");

        SwapchainSupportDetail detail(this->_physical_device,
                                      this->_surface.get());
        auto &capabilities = detail.capabilities;
        auto &surface_format = detail.choose_surface_format();
        auto &present_mode = detail.choose_present_mode();
        auto extent = detail.choose_extent(this->_platform_win);

        auto img_count = detail.capabilities.minImageCount + 1;
        if (detail.capabilities.maxImageCount > 0 &&
            img_count > detail.capabilities.maxImageCount) {
            img_count = detail.capabilities.maxImageCount;
        }

        GLOG_D("swapchain image count %d", img_count);
        GLOG_D("surface color space %s",
               vk::to_string(surface_format.colorSpace).c_str());
        GLOG_D("surface format %s",
               vk::to_string(surface_format.format).c_str());
        GLOG_D("present mode %s", vk::to_string(present_mode).c_str());
        auto [w, h] = extent;
        GLOG_D("extent %d %d", w, h);

        vk::SwapchainCreateInfoKHR create_info = {};
        create_info.setSurface(this->_surface.get());
        create_info.setMinImageCount(img_count);
        create_info.setImageFormat(surface_format.format);
        create_info.setImageColorSpace(surface_format.colorSpace);
        create_info.setImageExtent(extent);
        create_info.setImageArrayLayers(1);
        create_info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

        create_info.setImageSharingMode(vk::SharingMode::eExclusive);
        create_info.setPreTransform(capabilities.currentTransform);
        create_info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        create_info.setPresentMode(present_mode);
        create_info.setClipped(VK_TRUE);
        create_info.setOldSwapchain(this->_swapchain.handle.get());

        this->_swapchain.handle =
            this->_device->createSwapchainKHRUnique(create_info);
        this->_swapchain.images =
            std::move(this->_device->getSwapchainImagesKHR(
                this->_swapchain.handle.get()));

        this->_swapchain.format = surface_format.format;
        this->_swapchain.extent = extent;
        this->_swapchain.present_mode = present_mode;
        this->_swapchain.image_count = img_count;
    }

    void _create_command_pool() {
        this->_common_command_pool =
            this->_device->createCommandPoolUnique(vk::CommandPoolCreateInfo(
                vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                this->_queues[VK_QUEUE_GRAPHICS].queue_family_index.value()));

        this->_mem_command_pool =
            this->_device->createCommandPoolUnique(vk::CommandPoolCreateInfo(
                {},
                this->_queues[VK_QUEUE_TRANSFER].queue_family_index.value()));
    }

    void _create_sync_objects() {
        this->_img_available.resize(MAX_FRAMES_IN_FLIGHT);
        this->_render_finished.resize(MAX_FRAMES_IN_FLIGHT);
        this->_render_frences.resize(MAX_FRAMES_IN_FLIGHT);

        for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            this->_img_available[i] = this->_device->createSemaphoreUnique({});
            this->_render_finished[i] =
                this->_device->createSemaphoreUnique({});
            this->_render_frences[i] = this->_device->createFenceUnique(
                {vk::FenceCreateFlagBits::eSignaled});
        }
    }

    void _init() {

#ifdef MY_DEBUG
        this->_enable_validation_layers = true;
#endif
        vk::DynamicLoader dl;
        auto vkGetInstanceProcAddr =
            dl.getProcAddress<PFN_vkGetInstanceProcAddr>(
                "vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        this->_create_instance();
        this->_setup_debug_messenger();
        this->_create_surface();
        this->_pick_physical_device();
        this->_create_logical_device();
        this->_create_sync_objects();
        this->_create_command_pool();
        this->_create_swapchain();
    }

    vk::Format _find_supported_format(const std::vector<vk::Format> &candidates,
                                      vk::ImageTiling tiling,
                                      vk::FormatFeatureFlags features) const {
        for (vk::Format format : candidates) {
            auto props = this->_physical_device.getFormatProperties(format);
            if (tiling == vk::ImageTiling::eLinear &&
                (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == vk::ImageTiling::eOptimal &&
                       (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    vk::UniqueImageView
    _create_image_view(vk::Image image, vk::Format format,
                       const vk::ImageAspectFlags &aspect_flags) {
        vk::ImageViewCreateInfo create_info;
        create_info.setImage(image);
        create_info.setViewType(vk::ImageViewType::e2D);
        create_info.setFormat(format);
        // create_info.setComponents({});
        create_info.setSubresourceRange({aspect_flags, 0, 1, 0, 1});
        return this->_device->createImageViewUnique(create_info);
    }

    bool _is_frame_buffer_resized() {
        auto [w, h] = this->_platform_win->get_frame_buffer_size();
        return this->_swapchain.extent.width != w ||
               this->_swapchain.extent.height != h;
    }

    static VkBool32 _debug_msg_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) {
        auto level = vk::to_string(
            vk::DebugUtilsMessageSeverityFlagBitsEXT(messageSeverity));
        auto type =
            vk::to_string(vk::DebugUtilsMessageTypeFlagsEXT(messageTypes));

        if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            GLOG_E("[%s:%s]: %s", level.c_str(), type.c_str(),
                   pCallbackData->pMessage);
        } else {
            GLOG_D("[%s:%s]: %s", level.c_str(), type.c_str(),
                   pCallbackData->pMessage);
        }
        return VK_FALSE;
    }
}; // namespace

} // namespace

namespace my {
std::shared_ptr<VulkanCtx> make_vulkan_ctx(Window *win) {
    return std::make_shared<MyVulkanCtx>(win);
}
} // namespace my
