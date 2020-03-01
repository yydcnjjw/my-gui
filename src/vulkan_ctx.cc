#include "vulkan_ctx.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// TOOD: include
#include "camera.hpp"
#include "env.h"
#include "logger.h"
#include "window_mgr.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

constexpr int MESH_BINDING_ID = 0;
constexpr int INSTANCE_BINDING_ID = 1;
constexpr int INSTANCE_COUNT = 2;

struct Vertex {
    struct Mesh {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;
        bool operator==(const Mesh &other) const {
            return pos == other.pos && color == other.color &&
                   texCoord == other.texCoord;
        }
    };

    struct Instance {
        glm::vec3 pos;
    };

    static std::vector<vk::VertexInputBindingDescription>
    getBindingDescription() {
        return {vk::VertexInputBindingDescription(MESH_BINDING_ID, sizeof(Mesh),
                                                  vk::VertexInputRate::eVertex),
                vk::VertexInputBindingDescription(
                    INSTANCE_BINDING_ID, sizeof(Instance),
                    vk::VertexInputRate::eInstance)};
    }

    static std::array<vk::VertexInputAttributeDescription, 4>
    getAttributeDescriptions() {
        std::array<vk::VertexInputAttributeDescription, 4>
            attributeDescriptions = {};

        attributeDescriptions[0].binding = MESH_BINDING_ID;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[0].offset = offsetof(Vertex::Mesh, pos);

        attributeDescriptions[1].binding = MESH_BINDING_ID;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(Vertex::Mesh, color);

        attributeDescriptions[2].binding = MESH_BINDING_ID;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[2].offset = offsetof(Vertex::Mesh, texCoord);

        attributeDescriptions[3].binding = INSTANCE_BINDING_ID;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[3].offset = offsetof(Vertex::Instance, pos);

        return attributeDescriptions;
    }
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

namespace std {
template <> struct hash<Vertex::Mesh> {
    size_t operator()(Vertex::Mesh const &mesh) const {
        return ((hash<glm::vec3>()(mesh.pos) ^
                 (hash<glm::vec3>()(mesh.color) << 1)) >>
                1) ^
               (hash<glm::vec2>()(mesh.texCoord) << 1);
    }
};
} // namespace std

namespace {
constexpr int MAX_FRAMES_IN_FLIGHT = 2;
const std::string MODEL_PATH = "models/chalet.obj";
const std::string TEXTURE_PATH = "textures/chalet.jpg";

class MyVulkanCtx : public my::VulkanCtx {

  public:
    explicit MyVulkanCtx(const std::shared_ptr<my::Window> &win)
        : _platform_win(win), _camera(my::get_window_mgr()->get_observable()) {
        this->_init();
    }

    ~MyVulkanCtx() { this->_device->waitIdle(); };

    void draw() override {
        auto in_flight = this->_in_flight[this->_current_frame].get();
        this->_device->waitForFences({in_flight}, VK_TRUE,
                                     std::numeric_limits<uint64_t>::max());

        vk::Semaphore wait_semaphores[] = {
            this->_img_available[this->_current_frame].get()};

        vk::PipelineStageFlags wait_stages[] = {
            vk::PipelineStageFlagBits::eColorAttachmentOutput};

        vk::Semaphore signal_semaphores[] = {
            this->_render_finished[this->_current_frame].get()};

        uint32_t img_index;
        {
            try {
                auto result = this->_device->acquireNextImageKHR(
                    this->_swapchain.get(),
                    std::numeric_limits<uint64_t>::max(),
                    this->_img_available[this->_current_frame].get(), {});
                img_index = result.value;
            } catch (vk::OutOfDateKHRError err) {
                this->_recreate_swapchain();
                return;
            } catch (...) {
                throw std::runtime_error("failed to acquire swap chain image!");
            }
        }

        this->_update_uniform_buffer(
            img_index,
            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 1.0f)));

        vk::SubmitInfo submit_info(1, wait_semaphores, wait_stages, 1,
                                   &this->_command_buffers[img_index].get(), 1,
                                   signal_semaphores);

        this->_device->resetFences(1, &in_flight);

        this->_queues[VK_QUEUE_GRAPHICS].queue.submit({submit_info}, in_flight);

        {
            vk::SwapchainKHR swapchains[] = {this->_swapchain.get()};
            vk::PresentInfoKHR present_info(1, signal_semaphores, 1, swapchains,
                                            &img_index);
            vk::Result result;
            try {
                result = this->_queues[VK_QUEUE_PRESENT].queue.presentKHR(
                    present_info);
            } catch (vk::OutOfDateKHRError) {
                result = vk::Result::eErrorOutOfDateKHR;
            } catch (...) {
                throw std::runtime_error("failed to present swap chain image!");
            }

            if (this->_is_frame_buffer_resized()) {
                this->_recreate_swapchain();
                return;
            }

            switch (result) {
            case vk::Result::eSuccess:
                break;
            case vk::Result::eErrorOutOfDateKHR:
            case vk::Result::eSuboptimalKHR:
                this->_recreate_swapchain();
                return;
            default:
                throw std::runtime_error("failed to present swap chain image!");
            }
        }

        this->_current_frame =
            (this->_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
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

    std::shared_ptr<my::Window> _platform_win;
    vk::UniqueSurfaceKHR _surface;

    vk::PhysicalDevice _physical_device;
    vk::UniqueDevice _device;

    vk::UniqueSwapchainKHR _swapchain;
    std::vector<vk::Image> _swapchain_images;
    std::vector<vk::UniqueImageView> _swapchain_image_views;
    vk::Format _swapchain_format;
    vk::Extent2D _swapchain_extent;

    vk::UniqueRenderPass _render_pass;
    vk::UniqueDescriptorSetLayout _descriptor_set_layout;
    vk::UniquePipelineLayout _pipeline_layout;
    vk::UniquePipeline _pipeline;
    std::vector<vk::UniqueFramebuffer> _swapchain_frame_buffers;

    vk::UniqueCommandPool _common_command_pool;
    vk::UniqueCommandPool _mem_command_pool;

    std::vector<vk::UniqueCommandBuffer> _command_buffers;

    std::vector<Vertex::Mesh> _vertices;
    std::vector<uint32_t> _indices;
    std::vector<Vertex::Instance> _instance_data;

    struct DrawData {
        std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>
            vertex_buffer_binding;
        std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>
            index_buffer_binding;
        std::vector<Vertex::Mesh> vertices;
        std::vector<uint32_t> indices;
    };

    std::vector<DrawData> _draw_datas;

    std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory> _vertex_buffer_binding;
    std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory> _index_buffer_binding;
    std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>
        _instance_data_buffer_binding;
    std::vector<std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>>
        _uniform_buffer_bindings;
    std::pair<vk::UniqueImage, vk::UniqueDeviceMemory> _tex_buffer_binding;
    vk::UniqueImageView _tex_image_view;
    vk::UniqueSampler _tex_sampler;

    std::pair<vk::UniqueImage, vk::UniqueDeviceMemory>
        _depth_img_buffer_binding;
    vk::UniqueImageView _depth_image_view;

    vk::UniqueDescriptorPool _descriptor_pool;
    std::vector<vk::UniqueDescriptorSet> _descriptor_sets;

    std::vector<vk::UniqueSemaphore> _img_available;
    std::vector<vk::UniqueSemaphore> _render_finished;
    std::vector<vk::UniqueFence> _in_flight;

    size_t _current_frame = 0;

    my::Camera _camera;

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

    enum VulkanQueueType {
        VK_QUEUE_GRAPHICS,
        VK_QUEUE_COMPUTE,
        VK_QUEUE_TRANSFER,
        VK_QUEUE_SPARSE_BINDING,
        VK_QUEUE_PROTECTED,
        VK_QUEUE_PRESENT
    };

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

    struct SurfaceInfo {
        vk::Format format;
        vk::Extent2D extent;
    };

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
        auto extent = detail.choose_extent(this->_platform_win.get());

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
        create_info.setOldSwapchain({});

        this->_swapchain = this->_device->createSwapchainKHRUnique(create_info);
        this->_swapchain_images = std::move(
            this->_device->getSwapchainImagesKHR(this->_swapchain.get()));

        this->_swapchain_format = surface_format.format;
        this->_swapchain_extent = extent;

        this->_swapchain_image_views.clear();
        std::transform(this->_swapchain_images.begin(),
                       this->_swapchain_images.end(),
                       std::inserter(this->_swapchain_image_views,
                                     this->_swapchain_image_views.end()),
                       [this](const vk::Image &img) {
                           return this->_create_image_view(
                               img, this->_swapchain_format,
                               vk::ImageAspectFlagBits::eColor);
                       });
    }

    void _create_render_pass() {
        vk::AttachmentDescription color_attachment;
        color_attachment.setFormat(this->_swapchain_format);
        color_attachment.setSamples(vk::SampleCountFlagBits::e1);
        color_attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        color_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
        color_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
        color_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
        color_attachment.setInitialLayout(vk::ImageLayout::eUndefined);
        color_attachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

        vk::AttachmentDescription depth_attachment;
        depth_attachment.setFormat(this->_find_depth_format());
        depth_attachment.setSamples(vk::SampleCountFlagBits::e1);
        depth_attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        depth_attachment.setStoreOp(vk::AttachmentStoreOp::eDontCare);
        depth_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
        depth_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
        depth_attachment.setInitialLayout(vk::ImageLayout::eUndefined);
        depth_attachment.setFinalLayout(
            vk::ImageLayout::eDepthStencilAttachmentOptimal);

        vk::AttachmentReference color_attachment_ref(
            0, vk::ImageLayout::eColorAttachmentOptimal);

        vk::AttachmentReference depth_attachment_ref(
            1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

        vk::SubpassDescription subpass;
        subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
        subpass.setColorAttachmentCount(1);
        subpass.setPColorAttachments(&color_attachment_ref);
        subpass.setPDepthStencilAttachment(&depth_attachment_ref);

        vk::SubpassDependency dependency;
        dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
        dependency.setDstSubpass(0);
        dependency.setSrcStageMask(
            vk::PipelineStageFlagBits::eColorAttachmentOutput);
        dependency.setDstStageMask(
            vk::PipelineStageFlagBits::eColorAttachmentOutput);
        dependency.setSrcAccessMask({});
        dependency.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead |
                                    vk::AccessFlagBits::eColorAttachmentWrite);

        std::array<vk::AttachmentDescription, 2> attachments = {
            color_attachment, depth_attachment};

        vk::RenderPassCreateInfo create_info;
        create_info.setAttachmentCount(attachments.size());
        create_info.setPAttachments(attachments.data());
        create_info.setSubpassCount(1);
        create_info.setPSubpasses(&subpass);
        create_info.setDependencyCount(1);
        create_info.setPDependencies(&dependency);

        this->_render_pass = this->_device->createRenderPassUnique(create_info);
    }

    void _create_descriptor_set_layout() {
        vk::DescriptorSetLayoutBinding ubo_layout_binding(
            0, vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex, {});
        vk::DescriptorSetLayoutBinding sampler_layout_binding(
            1, vk::DescriptorType::eCombinedImageSampler, 1,
            vk::ShaderStageFlagBits::eFragment, {});

        std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
            ubo_layout_binding, sampler_layout_binding};

        vk::DescriptorSetLayoutCreateInfo create_info({}, bindings.size(),
                                                      bindings.data());

        this->_descriptor_set_layout =
            this->_device->createDescriptorSetLayoutUnique(create_info);
    }

    void _create_uniform_buffers() {
        vk::DeviceSize buffer_size = sizeof(UniformBufferObject);
        auto size = this->_swapchain_images.size();
        this->_uniform_buffer_bindings.resize(size);

        for (auto i = 0; i < size; ++i) {
            this->_uniform_buffer_bindings[i] = this->_create_buffer(
                buffer_size, vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent);
        }
    }

    void _create_descriptor_pool() {
        std::array<vk::DescriptorPoolSize, 2> pool_sizes;
        pool_sizes[0] = {vk::DescriptorType::eUniformBuffer,
                         static_cast<uint32_t>(this->_swapchain_images.size())};
        pool_sizes[1] = {vk::DescriptorType::eCombinedImageSampler,
                         static_cast<uint32_t>(this->_swapchain_images.size())};

        vk::DescriptorPoolCreateInfo create_info(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            this->_swapchain_images.size(), pool_sizes.size(),
            pool_sizes.data());

        this->_descriptor_pool =
            this->_device->createDescriptorPoolUnique(create_info);
    }

    void _create_descriptor_sets() {
        std::vector<vk::DescriptorSetLayout> layouts(
            this->_swapchain_images.size(), this->_descriptor_set_layout.get());

        vk::DescriptorSetAllocateInfo alloc_info(
            this->_descriptor_pool.get(), layouts.size(), layouts.data());

        this->_descriptor_sets =
            this->_device->allocateDescriptorSetsUnique(alloc_info);

        for (auto i = 0; i < this->_swapchain_images.size(); ++i) {
            vk::DescriptorBufferInfo buffer_info(
                this->_uniform_buffer_bindings[i].first.get(), 0,
                sizeof(UniformBufferObject));

            vk::DescriptorImageInfo img_info(
                this->_tex_sampler.get(), this->_tex_image_view.get(),
                vk::ImageLayout::eShaderReadOnlyOptimal);

            std::array<vk::WriteDescriptorSet, 2> desc_wirtes;
            desc_wirtes[0] = vk::WriteDescriptorSet(
                this->_descriptor_sets[i].get(), 0, 0, 1,
                vk::DescriptorType::eUniformBuffer, {}, &buffer_info);
            desc_wirtes[1] = vk::WriteDescriptorSet(
                this->_descriptor_sets[i].get(), 1, 0, 1,
                vk::DescriptorType::eCombinedImageSampler, &img_info);

            this->_device->updateDescriptorSets(desc_wirtes, {});
        }
    }

    static std::vector<char> _read_file(const std::string &filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    vk::UniqueShaderModule
    create_shader_module(const std::vector<char> &&code) {
        vk::ShaderModuleCreateInfo create_info(
            {}, code.size(), reinterpret_cast<const uint32_t *>(code.data()));

        return this->_device->createShaderModuleUnique(create_info);
    }

    void _create_graphics_pipeline() {
        auto vert_shader =
            create_shader_module(_read_file("shaders/shader.vert.spv"));
        auto frag_shader =
            create_shader_module(_read_file("shaders/shader.frag.spv"));

        vk::PipelineShaderStageCreateInfo vert_shader_stage(
            {}, vk::ShaderStageFlagBits::eVertex, vert_shader.get(), "main");
        vk::PipelineShaderStageCreateInfo frag_shader_stage(
            {}, vk::ShaderStageFlagBits::eFragment, frag_shader.get(), "main");

        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {
            vert_shader_stage, frag_shader_stage};

        auto binding_desc = Vertex::getBindingDescription();
        auto attribute_desc = Vertex::getAttributeDescriptions();
        vk::PipelineVertexInputStateCreateInfo vertex_input_state(
            {}, binding_desc.size(), binding_desc.data(), attribute_desc.size(),
            attribute_desc.data());

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state(
            {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE);

        vk::Viewport viewport(0.0f, 0.0f, this->_swapchain_extent.width,
                              this->_swapchain_extent.height, 0.0f, 1.0f);
        vk::Rect2D scissor({0, 0}, this->_swapchain_extent);
        vk::PipelineViewportStateCreateInfo viewport_state({}, 1, &viewport, 1,
                                                           &scissor);

        vk::PipelineRasterizationStateCreateInfo rasterization_state(
            {}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise,
            VK_FALSE, {}, {}, {}, 1.0f);

        vk::PipelineMultisampleStateCreateInfo multisample_state(
            {}, vk::SampleCountFlagBits::e1, VK_FALSE);

        vk::PipelineDepthStencilStateCreateInfo depth_stencil_state(
            {}, VK_TRUE, VK_TRUE, vk::CompareOp::eLess, VK_FALSE, VK_FALSE);

        vk::PipelineColorBlendAttachmentState color_blend_attachment_state(
            VK_FALSE);
        color_blend_attachment_state.setColorWriteMask(
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

        vk::PipelineColorBlendStateCreateInfo color_blend_state(
            {}, VK_FALSE, vk::LogicOp::eCopy, 1, &color_blend_attachment_state,
            {0.0f, 0.0f, 0.0f, 0.0f});

        this->_pipeline_layout = this->_device->createPipelineLayoutUnique(
            vk::PipelineLayoutCreateInfo({}, 1,
                                         &this->_descriptor_set_layout.get()));

        vk::GraphicsPipelineCreateInfo create_info;
        create_info.setStageCount(2);
        create_info.setPStages(shader_stages.data());
        create_info.setPVertexInputState(&vertex_input_state);
        create_info.setPInputAssemblyState(&input_assembly_state);
        create_info.setPViewportState(&viewport_state);
        create_info.setPRasterizationState(&rasterization_state);
        create_info.setPMultisampleState(&multisample_state);
        create_info.setPDepthStencilState(&depth_stencil_state);
        create_info.setPColorBlendState(&color_blend_state);
        create_info.setLayout(this->_pipeline_layout.get());
        create_info.setRenderPass(this->_render_pass.get());
        create_info.setSubpass(0);
        create_info.setBasePipelineHandle({});
        this->_pipeline =
            this->_device->createGraphicsPipelineUnique({}, create_info);
    }

    void _create_frame_buffers() {
        this->_swapchain_frame_buffers.clear();
        std::transform(
            this->_swapchain_image_views.begin(),
            this->_swapchain_image_views.end(),
            std::inserter(this->_swapchain_frame_buffers,
                          this->_swapchain_frame_buffers.end()),
            [this](const vk::UniqueImageView &img_view) {
                std::array<vk::ImageView, 2> attachments = {
                    img_view.get(), this->_depth_image_view.get()};

                vk::FramebufferCreateInfo create_info(
                    {}, this->_render_pass.get(), attachments.size(),
                    attachments.data(), this->_swapchain_extent.width,
                    this->_swapchain_extent.height, 1);
                return this->_device->createFramebufferUnique(create_info);
            });
    }

    void _create_command_pool() {
        this->_common_command_pool =
            this->_device->createCommandPoolUnique(vk::CommandPoolCreateInfo(
                {},
                this->_queues[VK_QUEUE_GRAPHICS].queue_family_index.value()));

        this->_mem_command_pool =
            this->_device->createCommandPoolUnique(vk::CommandPoolCreateInfo(
                {},
                this->_queues[VK_QUEUE_TRANSFER].queue_family_index.value()));
    }

    void _create_command_buffers() {
        vk::CommandBufferAllocateInfo allocate_info(
            this->_common_command_pool.get(), vk::CommandBufferLevel::ePrimary,
            this->_swapchain_frame_buffers.size());

        this->_command_buffers =
            this->_device->allocateCommandBuffersUnique(allocate_info);

        for (int i = 0; i < this->_command_buffers.size(); ++i) {
            vk::CommandBufferBeginInfo begin_info(
                vk::CommandBufferUsageFlagBits::eSimultaneousUse);
            this->_command_buffers[i]->begin(begin_info);

            std::array<vk::ClearValue, 2> clear_values = {};
            clear_values[0].color =
                std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
            clear_values[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
            vk::RenderPassBeginInfo render_pass_info(
                this->_render_pass.get(),
                this->_swapchain_frame_buffers[i].get(),
                {{0, 0}, this->_swapchain_extent}, clear_values.size(),
                clear_values.data());

            this->_command_buffers[i]->beginRenderPass(
                render_pass_info, vk::SubpassContents::eInline);

            this->_command_buffers[i]->bindPipeline(
                vk::PipelineBindPoint::eGraphics, this->_pipeline.get());

            std::vector<vk::DescriptorSet> descriptor_set = {
                this->_descriptor_sets[i].get()};
            this->_command_buffers[i]->bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics, this->_pipeline_layout.get(),
                0, descriptor_set, {});

            vk::DeviceSize offsets[] = {0};
            this->_command_buffers[i]->bindVertexBuffers(
                MESH_BINDING_ID, 1,
                &this->_vertex_buffer_binding.first.get(), offsets);
            this->_command_buffers[i]->bindVertexBuffers(
                INSTANCE_BINDING_ID, 1,
                &this->_instance_data_buffer_binding.first.get(), offsets);

            this->_command_buffers[i]->bindIndexBuffer(
                this->_index_buffer_binding.first.get(), 0,
                vk::IndexType::eUint32);
            this->_command_buffers[i]->drawIndexed(this->_indices.size(),
                                                   INSTANCE_COUNT, 0, 0, 0);

            // for (const auto &data : this->_draw_datas) {
            //     this->_command_buffers[i]->bindVertexBuffers(
            //         MESH_BINDING_ID, 1, &data.vertex_buffer_binding.first.get(),
            //         offsets);
            //     this->_command_buffers[i]->bindIndexBuffer(
            //         data.index_buffer_binding.first.get(), 0,
            //         vk::IndexType::eUint32);
            //     this->_command_buffers[i]->drawIndexed(data.indices.size(), 1,
            //                                            0, 0, 0);
            // }

            this->_command_buffers[i]->endRenderPass();

            this->_command_buffers[i]->end();
        }
    }

    void _create_sync_objects() {
        this->_img_available.resize(MAX_FRAMES_IN_FLIGHT);
        this->_render_finished.resize(MAX_FRAMES_IN_FLIGHT);
        this->_in_flight.resize(MAX_FRAMES_IN_FLIGHT);

        for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            this->_img_available[i] = this->_device->createSemaphoreUnique({});
            this->_render_finished[i] =
                this->_device->createSemaphoreUnique({});
            this->_in_flight[i] = this->_device->createFenceUnique(
                {vk::FenceCreateFlagBits::eSignaled});
        }
    }

    void _recreate_swapchain() {

        this->_device->waitIdle();

        this->_create_swapchain();
        this->_create_render_pass();
        this->_create_graphics_pipeline();
        this->_create_depth_resources();
        this->_create_frame_buffers();
        this->_create_command_buffers();
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

        this->_load_model();

        this->_create_instance();
        this->_setup_debug_messenger();
        this->_create_surface();
        this->_pick_physical_device();
        this->_create_logical_device();
        this->_create_sync_objects();

        this->_create_command_pool();

        this->_vertex_buffer_binding =
            this->_create_vertex_buffer(this->_vertices);
        this->_index_buffer_binding =
            this->_create_index_buffer(this->_indices);

        const std::vector<Vertex::Mesh> vertices = {
            {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
            {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
            {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}};
        const std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};

        std::vector<std::pair<std::vector<Vertex::Mesh>, std::vector<uint32_t>>>
            items = {
                {vertices, indices},
            };

        for (const auto &data : items) {
            this->_draw_datas.push_back(
                {this->_create_vertex_buffer(data.first),
                 this->_create_index_buffer(data.second), data.first,
                 data.second});
        }
        GLOG_D("draw size %d", this->_draw_datas.size());

        this->_create_instance_buffer();
        this->_create_texture_image();
        this->_create_texture_sampler();

        this->_create_swapchain();

        this->_create_uniform_buffers();
        this->_create_descriptor_pool();
        this->_create_descriptor_set_layout();
        this->_create_descriptor_sets();

        this->_create_render_pass();
        this->_create_graphics_pipeline();
        this->_create_depth_resources();
        this->_create_frame_buffers();

        this->_create_command_buffers();
    }

    void _update_uniform_buffer(uint32_t current_image,
                                const glm::mat4 &model) {
        static auto start_time = std::chrono::high_resolution_clock::now();

        auto current_time = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(
                         current_time - start_time)
                         .count();

        UniformBufferObject ubo = {};
        // ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
        //                         glm::vec3(0.0f, 0.0f, 1.0f));
        // ubo.model = model;
        ubo.model = glm::mat4(1.0f);

        ubo.view = this->_camera.view;

        // ubo.view = glm::lookAt(glm::vec3(5.0f, 5.0f, 5.0f),
        //                        glm::vec3(0.0f, 0.0f, 0.0f),
        //                        glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f),
                                    this->_swapchain_extent.width /
                                        (float)this->_swapchain_extent.height,
                                    0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        void *data = this->_device->mapMemory(
            this->_uniform_buffer_bindings[current_image].second.get(), 0,
            sizeof(ubo));
        ::memcpy(data, &ubo, sizeof(ubo));
        this->_device->unmapMemory(
            this->_uniform_buffer_bindings[current_image].second.get());
    }

    void _load_model() {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                              MODEL_PATH.c_str())) {
            throw std::runtime_error(warn + err);
        }
        std::unordered_map<Vertex::Mesh, uint32_t> unique_vertices;

        for (const auto &shape : shapes) {
            for (const auto &index : shape.mesh.indices) {
                Vertex::Mesh vertex = {};

                vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                              attrib.vertices[3 * index.vertex_index + 1],
                              attrib.vertices[3 * index.vertex_index + 2]};

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (unique_vertices.count(vertex) == 0) {
                    unique_vertices[vertex] =
                        static_cast<uint32_t>(this->_vertices.size());
                    this->_vertices.push_back(vertex);
                }

                this->_indices.push_back(unique_vertices[vertex]);
            }
        }
    }

    uint32_t _find_memory_type(uint32_t type_filter,
                               const vk::MemoryPropertyFlags &properties) {
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

    std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>
    _create_buffer(vk::DeviceSize size, const vk::BufferUsageFlags &usage,
                   const vk::MemoryPropertyFlags &properties) {
        vk::BufferCreateInfo create_info({}, size, usage,
                                         vk::SharingMode::eExclusive);
        auto buffer = this->_device->createBufferUnique(create_info);

        auto mem_require =
            this->_device->getBufferMemoryRequirements(buffer.get());
        vk::MemoryAllocateInfo alloc_info(
            mem_require.size,
            this->_find_memory_type(mem_require.memoryTypeBits, properties));
        auto buffer_memory = this->_device->allocateMemoryUnique(alloc_info);
        this->_device->bindBufferMemory(buffer.get(), buffer_memory.get(), 0);

        return std::make_pair(std::move(buffer), std::move(buffer_memory));
    }

    std::pair<vk::UniqueImage, vk::UniqueDeviceMemory>
    _create_image(uint32_t width, uint32_t height, const vk::Format &format,
                  const vk::ImageTiling &tiling,
                  const vk::ImageUsageFlags &usage,
                  const vk::MemoryPropertyFlags &properties) {
        vk::ImageCreateInfo create_info({}, vk::ImageType::e2D, format,
                                        {width, height, 1}, 1, 1,
                                        vk::SampleCountFlagBits::e1, tiling,
                                        usage, vk::SharingMode::eExclusive);

        auto image = this->_device->createImageUnique(create_info);

        auto mem_require =
            this->_device->getImageMemoryRequirements(image.get());
        vk::MemoryAllocateInfo alloc_info(
            mem_require.size,
            this->_find_memory_type(mem_require.memoryTypeBits, properties));
        auto buffer_memory = this->_device->allocateMemoryUnique(alloc_info);
        this->_device->bindImageMemory(image.get(), buffer_memory.get(), 0);

        return std::make_pair(std::move(image), std::move(buffer_memory));
    }

    void _create_texture_image() {
        int tex_width, tex_height, tex_channels;
        stbi_uc *pixels = stbi_load(TEXTURE_PATH.c_str(), &tex_width,
                                    &tex_height, &tex_channels, STBI_rgb_alpha);

        vk::DeviceSize img_size = tex_width * tex_height * 4;
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        auto staging_buffer_binding = this->_create_buffer(
            img_size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);

        auto data = this->_device->mapMemory(
            staging_buffer_binding.second.get(), 0, img_size);
        ::memcpy(data, pixels, img_size);
        this->_device->unmapMemory(staging_buffer_binding.second.get());
        stbi_image_free(pixels);

        this->_tex_buffer_binding = this->_create_image(
            tex_width, tex_height, vk::Format::eR8G8B8A8Unorm,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferDst |
                vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal);

        this->_transition_image_layout(
            this->_tex_buffer_binding.first.get(), vk::Format::eR8G8B8A8Unorm,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

        this->_copy_buffer_to_image(staging_buffer_binding.first.get(),
                                    this->_tex_buffer_binding.first.get(),
                                    tex_width, tex_height);

        this->_transition_image_layout(this->_tex_buffer_binding.first.get(),
                                       vk::Format::eR8G8B8A8Unorm,
                                       vk::ImageLayout::eTransferDstOptimal,
                                       vk::ImageLayout::eShaderReadOnlyOptimal);

        this->_tex_image_view = this->_create_image_view(
            this->_tex_buffer_binding.first.get(), vk::Format::eR8G8B8A8Unorm,
            vk::ImageAspectFlagBits::eColor);
    }

    vk::Format _find_supported_format(const std::vector<vk::Format> &candidates,
                                      vk::ImageTiling tiling,
                                      vk::FormatFeatureFlags features) {
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

    vk::Format _find_depth_format() {
        return this->_find_supported_format(
            {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,
             vk::Format::eD24UnormS8Uint},
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    }

    void _create_depth_resources() {
        vk::Format depth_format = this->_find_depth_format();
        this->_depth_img_buffer_binding = this->_create_image(
            this->_swapchain_extent.width, this->_swapchain_extent.height,
            depth_format, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::MemoryPropertyFlagBits::eDeviceLocal);

        this->_depth_image_view = this->_create_image_view(
            this->_depth_img_buffer_binding.first.get(), depth_format,
            vk::ImageAspectFlagBits::eDepth);
    }

    void _create_instance_buffer() {

        this->_instance_data.resize(INSTANCE_COUNT);

        // for (auto i = 0; i < INSTANCE_COUNT; ++i) {
        //     Vertex::Instance data;
        //     data.pos = {0.0f, 0.0f, 0.0f};
        //     this->_instance_data[i] = data;
        // }
        this->_instance_data[0] = {{1.0f, 0.0f, 0.0f}};
        this->_instance_data[1] = {{0.0f, 1.0f, 0.0f}};

        vk::DeviceSize buffer_size =
            sizeof(this->_instance_data[0]) * this->_instance_data.size();

        auto staging_buffer_binding = this->_create_buffer(
            buffer_size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);

        auto data = this->_device->mapMemory(
            staging_buffer_binding.second.get(), 0, buffer_size);
        ::memcpy(data, this->_instance_data.data(), buffer_size);
        this->_device->unmapMemory(staging_buffer_binding.second.get());

        this->_instance_data_buffer_binding =
            this->_create_buffer(buffer_size,
                                 vk::BufferUsageFlagBits::eTransferDst |
                                     vk::BufferUsageFlagBits::eVertexBuffer,
                                 vk::MemoryPropertyFlagBits::eDeviceLocal);

        this->_copy_buffer(staging_buffer_binding.first.get(),
                           this->_instance_data_buffer_binding.first.get(),
                           buffer_size);
    }

    std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>
    _create_index_buffer(const std::vector<uint32_t> &indices) {
        vk::DeviceSize buffer_size = sizeof(indices[0]) * indices.size();

        auto staging_buffer_binding = this->_create_buffer(
            buffer_size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);

        auto data = this->_device->mapMemory(
            staging_buffer_binding.second.get(), 0, buffer_size);
        ::memcpy(data, indices.data(), buffer_size);
        this->_device->unmapMemory(staging_buffer_binding.second.get());

        auto index_buffer_binding =
            this->_create_buffer(buffer_size,
                                 vk::BufferUsageFlagBits::eTransferDst |
                                     vk::BufferUsageFlagBits::eIndexBuffer,
                                 vk::MemoryPropertyFlagBits::eDeviceLocal);

        this->_copy_buffer(staging_buffer_binding.first.get(),
                           index_buffer_binding.first.get(), buffer_size);
        return index_buffer_binding;
    }

    std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>
    _create_vertex_buffer(const std::vector<Vertex::Mesh> &vertices) {
        vk::DeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

        auto staging_buffer_binding = this->_create_buffer(
            buffer_size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);

        auto data = this->_device->mapMemory(
            staging_buffer_binding.second.get(), 0, buffer_size);
        ::memcpy(data, vertices.data(), buffer_size);
        this->_device->unmapMemory(staging_buffer_binding.second.get());

        auto vertex_buffer_binding =
            this->_create_buffer(buffer_size,
                                 vk::BufferUsageFlagBits::eTransferDst |
                                     vk::BufferUsageFlagBits::eVertexBuffer,
                                 vk::MemoryPropertyFlagBits::eDeviceLocal);

        this->_copy_buffer(staging_buffer_binding.first.get(),
                           vertex_buffer_binding.first.get(), buffer_size);
        return vertex_buffer_binding;
    }

    void _single_time_commands(
        vk::CommandPool &command_pool, VulkanQueueType type,
        const std::function<void(vk::CommandBuffer &)> &commands) {
        vk::CommandBufferAllocateInfo alloc_info(
            command_pool, vk::CommandBufferLevel::ePrimary, 1);

        auto command_buffer = std::move(
            this->_device->allocateCommandBuffersUnique(alloc_info).front());

        command_buffer->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        commands(command_buffer.get());
        command_buffer->end();

        vk::SubmitInfo submit_info;
        submit_info.setCommandBufferCount(1);
        submit_info.setPCommandBuffers(&command_buffer.get());

        this->_queues[type].queue.submit({submit_info}, {});
        this->_queues[type].queue.waitIdle();
    }

    void _copy_buffer(vk::Buffer &src_buffer, vk::Buffer &dst_buffer,
                      vk::DeviceSize &size) {
        this->_single_time_commands(
            this->_mem_command_pool.get(), VK_QUEUE_TRANSFER,
            [&](vk::CommandBuffer &command_buffer) {
                vk::BufferCopy copy_region({}, {}, size);
                command_buffer.copyBuffer(src_buffer, dst_buffer, 1,
                                          &copy_region);
            });
    }

    void _copy_buffer_to_image(vk::Buffer &buffer, vk::Image &image,
                               uint32_t width, uint32_t height) {
        this->_single_time_commands(
            this->_mem_command_pool.get(), VK_QUEUE_TRANSFER,
            [&](vk::CommandBuffer &command_buffer) {
                vk::BufferImageCopy region(
                    0, 0, 0, {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                    {0, 0, 0}, {width, height, 1});
                command_buffer.copyBufferToImage(
                    buffer, image, vk::ImageLayout::eTransferDstOptimal, 1,
                    &region);
            });
    }

    vk::UniqueImageView _create_image_view(vk::Image image, vk::Format format,
                                           vk::ImageAspectFlags aspect_flags) {
        vk::ImageViewCreateInfo create_info;
        create_info.setImage(image);
        create_info.setViewType(vk::ImageViewType::e2D);
        create_info.setFormat(format);
        // create_info.setComponents({});
        create_info.setSubresourceRange({aspect_flags, 0, 1, 0, 1});
        return this->_device->createImageViewUnique(create_info);
    }

    void _create_texture_sampler() {
        vk::SamplerCreateInfo create_info(
            {}, vk::Filter::eLinear, vk::Filter::eLinear,
            vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat,
            vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
            0.0f, VK_FALSE, 1, VK_FALSE, vk::CompareOp::eAlways, 0.0f, 0.0f,
            vk::BorderColor::eIntOpaqueBlack, VK_FALSE);
        this->_tex_sampler = this->_device->createSamplerUnique(create_info);
    }

    void _transition_image_layout(vk::Image &image, const vk::Format &format,
                                  const vk::ImageLayout &old_layout,
                                  const vk::ImageLayout &new_layout) {
        this->_single_time_commands(
            this->_mem_command_pool.get(), VK_QUEUE_TRANSFER,
            [&](vk::CommandBuffer &command_buffer) {
                vk::PipelineStageFlags source_stage;
                vk::PipelineStageFlags destination_stage;

                vk::ImageMemoryBarrier barrier(
                    {}, {}, old_layout, new_layout, VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED, image,
                    {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

                if (old_layout == vk::ImageLayout::eUndefined &&
                    new_layout == vk::ImageLayout::eTransferDstOptimal) {
                    barrier.setSrcAccessMask({});
                    barrier.setDstAccessMask(
                        vk::AccessFlagBits::eTransferWrite);
                    source_stage = vk::PipelineStageFlagBits::eTopOfPipe;
                    destination_stage = vk::PipelineStageFlagBits::eTransfer;
                } else if (old_layout == vk::ImageLayout::eTransferDstOptimal &&
                           new_layout ==
                               vk::ImageLayout::eShaderReadOnlyOptimal) {
                    barrier.setSrcAccessMask(
                        vk::AccessFlagBits::eTransferWrite);
                    barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
                    source_stage = vk::PipelineStageFlagBits::eTransfer;
                    destination_stage =
                        vk::PipelineStageFlagBits::eFragmentShader;
                } else {
                    throw std::invalid_argument(
                        "unsupported layout transition!");
                }

                command_buffer.pipelineBarrier(source_stage, destination_stage,
                                               {}, 0, nullptr, 0, nullptr, 1,
                                               &barrier);
            });
    }

    bool _is_frame_buffer_resized() {
        auto [w, h] = this->_platform_win->get_frame_buffer_size();
        return this->_swapchain_extent.width != w ||
               this->_swapchain_extent.height != h;
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
        GLOG_D("[%s:%s]: %s", level.c_str(), type.c_str(),
               pCallbackData->pMessage);
        return VK_FALSE;
    }
};

} // namespace

namespace my {
std::shared_ptr<VulkanCtx> make_vulkan_ctx(std::shared_ptr<Window> win) {
    return std::make_shared<MyVulkanCtx>(win);
    ;
}
} // namespace my
