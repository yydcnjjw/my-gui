#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <boost/move/move.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <utility>

#include "vulkan_ctx.h"

#define VERTEX_BINDING_ID 0

namespace my {

class Vertex {
  public:
    glm::vec3 pos;
    glm::vec2 tex_coord;

    static vk::VertexInputBindingDescription get_binding_description() {
        return vk::VertexInputBindingDescription(
            VERTEX_BINDING_ID, sizeof(Vertex), vk::VertexInputRate::eVertex);
    }

    static std::vector<vk::VertexInputAttributeDescription>
    get_attribute_descriptions() {
        vk::VertexInputAttributeDescription pos_desc(
            0, VERTEX_BINDING_ID, vk::Format::eR32G32B32Sfloat,
            offsetof(Vertex, pos));
        vk::VertexInputAttributeDescription tex_coord_desc(
            1, VERTEX_BINDING_ID, vk::Format::eR32G32Sfloat,
            offsetof(Vertex, tex_coord));
        return {pos_desc, tex_coord_desc};
    }
};

enum TextureType {};

class Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

template <class T> struct BufferBinding {
    BufferBinding(T &&buffer, vk::UniqueDeviceMemory &&memory)
        : buffer(std::move(buffer)), memory(std::move(memory)) {}

    T buffer;
    vk::UniqueDeviceMemory memory;

  private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(BufferBinding)
};

class VulkanShader {
  public:
    VulkanShader(const vk::Device &device, const std::string &code,
                 const vk::ShaderStageFlagBits &stage, std::string name)
        : _shader(my::VulkanShader::_create_shader_module(device, code)),
          _stage(stage), _name(std::move(name)),
          _create_info({}, this->_stage, this->_shader.get(),
                       this->_name.c_str()) {}

    [[nodiscard]] const vk::PipelineShaderStageCreateInfo &get() const {
        return this->_create_info;
    }

  private:
    vk::UniqueShaderModule _shader;
    vk::ShaderStageFlagBits _stage;
    std::string _name;
    vk::PipelineShaderStageCreateInfo _create_info;

    static vk::UniqueShaderModule
    _create_shader_module(const vk::Device &device, const std::string &code) {
        vk::ShaderModuleCreateInfo create_info(
            {}, code.size(), reinterpret_cast<const uint32_t *>(code.data()));

        return device.createShaderModuleUnique(create_info);
    }
};

struct VertexDesciption {
    std::vector<vk::VertexInputBindingDescription> bindings;
    std::vector<vk::VertexInputAttributeDescription> attributes;
};

struct VulkanImageBuffer {
  public:
    VulkanImageBuffer() = default;
    VulkanImageBuffer(std::shared_ptr<BufferBinding<vk::UniqueImage>> &&binding,
                      vk::UniqueImageView &&image_view,
                      vk::UniqueDescriptorSet &&desc_set = {},
                      vk::UniqueSampler &&sampler = {})
        : binding(std::move(binding)), image_view(std::move(image_view)),
          desc_set(std::move(desc_set)), tex_sampler(std::move(sampler)) {}

    std::shared_ptr<BufferBinding<vk::UniqueImage>> binding;
    vk::UniqueImageView image_view;
    vk::UniqueDescriptorSet desc_set;
    vk::UniqueSampler tex_sampler;

    VulkanImageBuffer &operator=(VulkanImageBuffer &&buffer) {
        this->binding = std::move(buffer.binding);
        this->image_view = std::move(buffer.image_view);
        this->desc_set = std::move(buffer.desc_set);
        this->tex_sampler = std::move(buffer.tex_sampler);
        return *this;
    }

  private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(VulkanImageBuffer);
};
struct VulkanUniformBuffer {
    std::shared_ptr<BufferBinding<vk::UniqueBuffer>> share_binding;
    std::shared_ptr<BufferBinding<vk::UniqueBuffer>> dynamic_binding;
    vk::UniqueDescriptorSet desc_set;
    vk::DeviceSize share_size;
    vk::DeviceSize dynamic_size;
    vk::DeviceSize dynamic_align;

  private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(VulkanUniformBuffer);
};

struct VulkanPipeline {
    vk::UniquePipeline handle;
    vk::UniquePipelineLayout layout;
    VulkanPipeline(vk::UniquePipeline &&handle,
                   vk::UniquePipelineLayout &&layout)
        : handle(std::move(handle)), layout(std::move(layout)) {}
};

struct VulkanDrawCtx;
typedef std::function<void(const VulkanDrawCtx &, const VulkanCtx &,
                           my::RenderDevice &)>
    VulkanDrawCallback;

struct VulkanDrawCtx {
  public:
    VulkanDrawCtx() = default;
    vk::CommandBuffer command_buffer;
    size_t swapchain_img_index = 0;
    vk::RenderPass render_pass;
    vk::Framebuffer frame_buffer;

  private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(VulkanDrawCtx);
};

class RenderDevice {
  public:
    RenderDevice() = default;

    virtual ~RenderDevice() = default;

    virtual std::shared_ptr<VulkanShader>
    create_shader(const std::string &code, const vk::ShaderStageFlagBits &stage,
                  const std::string &name) = 0;

    virtual std::shared_ptr<VulkanPipeline>
    create_pipeline(const std::vector<std::shared_ptr<VulkanShader>> &shaders,
                    const VertexDesciption &vertex_desc,
                    const bool enable_depth_test = true,
                    const std::vector<vk::PushConstantRange>
                        &push_constant_ranges = {}) = 0;

    virtual std::shared_ptr<BufferBinding<vk::UniqueBuffer>>
    create_buffer(const void *data, const vk::DeviceSize &buffer_size,
                  const vk::BufferUsageFlags &usage) = 0;

    virtual std::shared_ptr<BufferBinding<vk::UniqueBuffer>>
    create_vertex_buffer(const std::vector<Vertex> &vertices) = 0;

    virtual std::shared_ptr<my::BufferBinding<vk::UniqueBuffer>>
    create_index_buffer(const std::vector<uint32_t> &indices) = 0;

    virtual VulkanUniformBuffer
    create_uniform_buffer(const uint64_t &share_size,
                          const uint64_t &per_obj_size,
                          const uint64_t &num) = 0;

    virtual VulkanImageBuffer
    create_texture_buffer(void *pixels, size_t w, size_t h,
                          bool enable_mipmaps = false) = 0;

    virtual void bind_pipeline(my::VulkanPipeline *pipeline) = 0;

    virtual void bind_vertex_buffer(
        const std::shared_ptr<BufferBinding<vk::UniqueBuffer>> &) = 0;

    virtual void bind_index_buffer(
        const std::shared_ptr<BufferBinding<vk::UniqueBuffer>> &) = 0;

    virtual void bind_uniform_buffer(const VulkanUniformBuffer &,
                                     const uint64_t &object_index,
                                     const my::VulkanPipeline &pipeline) = 0;

    virtual void bind_texture_buffer(const VulkanImageBuffer &,
                                     const VulkanPipeline &) = 0;

    virtual void push_constants(vk::PipelineLayout layout,
                                const vk::ShaderStageFlags &stage,
                                uint32_t size, void *data) = 0;

    virtual void
    copy_to_buffer(void *src, size_t size,
                   const my::BufferBinding<vk::UniqueBuffer> &bind) = 0;

    virtual void map_memory(const my::BufferBinding<vk::UniqueBuffer> &bind,
                            uint64_t size,
                            const std::function<void(void *map)>&) = 0;

    virtual void wait_idle() = 0;
    virtual void with_draw(const vk::RenderPass &,
                           const VulkanDrawCallback &) = 0;

    virtual void draw(uint32_t index_count, uint32_t instance_count = 1,
                      uint32_t first_index = 0, int32_t vertex_offset = 0,
                      uint32_t first_instance = 0) = 0;

    virtual void draw_begin() = 0;
    virtual void draw_end() = 0;

    virtual VulkanImageBuffer *get_depth_buffer() = 0;

    bool enable_depth_test = true;
    bool enable_sample = true;
};

std::shared_ptr<RenderDevice> make_vulkan_render_device(VulkanCtx *ctx);

} // namespace my
