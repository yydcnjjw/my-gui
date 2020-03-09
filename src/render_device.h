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

class Texture {
    TextureType type;
    union {
        glm::vec3 color;
    };
};

class Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Texture> textures;
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
    VulkanShader(const vk::Device &device, const std::vector<char> &code,
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
    _create_shader_module(const vk::Device &device,
                          const std::vector<char> &code) {
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
                      vk::UniqueSampler &&sampler = {})
        : binding(std::move(binding)), image_view(std::move(image_view)),
          tex_sampler(std::move(sampler)) {}

    std::shared_ptr<BufferBinding<vk::UniqueImage>> binding;
    vk::UniqueImageView image_view;
    vk::UniqueSampler tex_sampler;

    VulkanImageBuffer &operator=(VulkanImageBuffer &&buffer) {
        this->binding = std::move(buffer.binding);
        this->image_view = std::move(buffer.image_view);
        this->tex_sampler = std::move(buffer.tex_sampler);
        return *this;
    }

  private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(VulkanImageBuffer);
};
struct VulkanUniformBuffer {
    // public:
    //     VulkanUniformBuffer
    std::shared_ptr<BufferBinding<vk::UniqueBuffer>> binding;
    vk::DeviceSize size;

  private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(VulkanUniformBuffer);
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

    virtual vk::UniqueRenderPass create_render_pass() = 0;

    virtual std::shared_ptr<VulkanShader>
    create_shader(const std::vector<char> &code,
                  const vk::ShaderStageFlagBits &stage,
                  const std::string &name) = 0;

    virtual vk::UniquePipeline
    create_pipeline(const std::vector<std::shared_ptr<VulkanShader>> &shaders,
                    const VertexDesciption &vertex_desc,
                    const vk::RenderPass &render_pass) = 0;

    // virtual std::vector<vk::UniqueFramebuffer>
    // create_frame_buffers(const vk::RenderPass &render_pass) = 0;

    // virtual std::vector<vk::UniqueCommandBuffer> create_command_buffers(
    //     const std::vector<vk::UniqueFramebuffer> &frame_buffers,
    //     const vk::RenderPass &render_pass,
    //     std::function<void(vk::CommandBuffer &)> callback) = 0;

    virtual std::shared_ptr<BufferBinding<vk::UniqueBuffer>>
    create_vertex_buffer(const std::vector<Vertex> &vertices) = 0;

    virtual std::shared_ptr<my::BufferBinding<vk::UniqueBuffer>>
    create_index_buffer(const std::vector<uint32_t> &indices) = 0;

    virtual VulkanUniformBuffer
    create_uniform_buffer(const vk::DeviceSize &size) = 0;

    virtual VulkanImageBuffer create_texture_buffer(void *pixels, size_t w,
                                                    size_t h) = 0;

    virtual void bind_pipeline(const vk::Pipeline &) = 0;

    virtual void bind_vertex_buffer(
        const std::shared_ptr<BufferBinding<vk::UniqueBuffer>> &) = 0;

    virtual void bind_index_buffer(
        const std::shared_ptr<BufferBinding<vk::UniqueBuffer>> &) = 0;

    virtual void
    copy_to_buffer(void *src, size_t size,
                   const my::BufferBinding<vk::UniqueBuffer> &bind) = 0;

    virtual void wait_idle() = 0;
    virtual void with_draw(const vk::RenderPass &,
                           const VulkanDrawCallback &) = 0;

    virtual void draw(size_t index_count) = 0;

    virtual void draw_begin(const vk::RenderPass &render_pass) = 0;
    virtual void draw_end() = 0;
};

std::shared_ptr<RenderDevice>
make_vulkan_render_device(std::shared_ptr<VulkanCtx> ctx);

} // namespace my
