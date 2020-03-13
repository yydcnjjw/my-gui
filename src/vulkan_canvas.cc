#include "canvas.h"

#include "draw_list.h"
#include "render_device.h"
#include "util.hpp"

namespace {
class VulkanCanvas : public my::Canvas {
  public:
    VulkanCanvas(my::VulkanCtx *ctx, my::RenderDevice *device)
        : _vk_ctx(ctx), _render_device(device) {

        auto vert_shader = this->_render_device->create_shader(
            Util::read_file("shaders/canvas_shader.vert.spv"),
            vk::ShaderStageFlagBits ::eVertex, "main");
        auto frag_shader = this->_render_device->create_shader(
            Util::read_file("shaders/canvas_shader.frag.spv"),
            vk::ShaderStageFlagBits ::eFragment, "main");

        my::VertexDesciption vertex_desc = {
            {VulkanCanvas::get_binding_description()},
            VulkanCanvas::get_attribute_descriptions()};

        vk::PushConstantRange push_constant_range(
            vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstBlock));

        this->_pipeline = this->_render_device->create_pipeline(
            {vert_shader, frag_shader}, vertex_desc, {push_constant_range});
    }

    ~VulkanCanvas() override { this->_render_device->wait_idle(); }

    void fill_rect(const glm::vec2 &a, const glm::vec2 &b,
                   const glm::u8vec4 &col) override {
        this->_draw_list.fill_rect(a, b, col);
    }

    void stroke_rect(const glm::vec2 &a, const glm::vec2 &b,
                     const glm::u8vec4 &col, float line_width) override {
        this->_draw_list.stroke_rect(a, b, col, line_width);
    }

    void draw() override {
        auto &vtx_list = this->_draw_list.get_draw_vtx();
        auto &idx_list = this->_draw_list.get_draw_idx();

        this->_render_device->bind_pipeline(this->_pipeline.get());
        auto &extent = this->_vk_ctx->get_swapchain().extent;
        this->_push_const_block.scale =
            glm::vec2(2.0f / extent.width, 2.0f / extent.height);
        this->_push_const_block.translate = glm::vec2(-1.0f);

        

        if (vtx_list.size() > 0 && idx_list.size() > 0) {
            this->_vtx_buffer = this->_render_device->create_buffer(
                vtx_list.data(), sizeof(my::DrawVert) * vtx_list.size(),
                vk::BufferUsageFlagBits::eVertexBuffer);
            this->_idx_buffer = this->_render_device->create_buffer(
                idx_list.data(), sizeof(uint32_t) * idx_list.size(),
                vk::BufferUsageFlagBits::eIndexBuffer);

            this->_render_device->bind_vertex_buffer(this->_vtx_buffer);
            this->_render_device->bind_index_buffer(this->_idx_buffer);
            this->_render_device->draw(idx_list.size());
        }

        this->_draw_list.clear();
    }

  private:
    my::VulkanCtx *_vk_ctx;
    my::RenderDevice *_render_device;

    my::DrawList _draw_list;

    vk::UniquePipeline _pipeline;

    std::shared_ptr<my::BufferBinding<vk::UniqueBuffer>> _vtx_buffer;
    std::shared_ptr<my::BufferBinding<vk::UniqueBuffer>> _idx_buffer;

    struct PushConstBlock {
        glm::vec2 scale;
        glm::vec2 translate;
    } _push_const_block;

    static vk::VertexInputBindingDescription get_binding_description() {
        return vk::VertexInputBindingDescription(VERTEX_BINDING_ID,
                                                 sizeof(my::DrawVert),
                                                 vk::VertexInputRate::eVertex);
    }

    static std::vector<vk::VertexInputAttributeDescription>
    get_attribute_descriptions() {
        vk::VertexInputAttributeDescription pos_desc(
            0, VERTEX_BINDING_ID, vk::Format::eR32G32Sfloat,
            offsetof(my::DrawVert, pos));
        vk::VertexInputAttributeDescription uv_desc(1, VERTEX_BINDING_ID,
                                                    vk::Format::eR32G32Sfloat,
                                                    offsetof(my::DrawVert, uv));
        vk::VertexInputAttributeDescription color_desc(
            2, VERTEX_BINDING_ID, vk::Format::eR8G8B8A8Unorm,
            offsetof(my::DrawVert, col));
        return {pos_desc, uv_desc, color_desc};
    }
};
} // namespace

namespace my {

std::shared_ptr<Canvas> make_vulkan_canvas(my::VulkanCtx *ctx,
                                           my::RenderDevice *device) {
    return std::make_shared<VulkanCanvas>(ctx, device);
}
} // namespace my
