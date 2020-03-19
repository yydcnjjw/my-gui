#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "draw_list.h"
#include "vulkan_ctx.h"
namespace my {

class Canvas {
  public:
    Canvas() = default;
    virtual ~Canvas() = default;

    virtual void fill_rect(const glm::vec2 &a, const glm::vec2 &b,
                           const glm::u8vec4 &) = 0;

    virtual void stroke_rect(const glm::vec2 &a, const glm::vec2 &b,
                             const glm::u8vec4 &, float line_width = 1.0f) = 0;

    virtual void draw_image(TextureID, const glm::vec2 &p_min,
                            const glm::vec2 &p_max, uint8_t alpha = 255,
                            const glm::vec2 &uv_min = {0, 0},
                            const glm::vec2 &uv_max = {1, 1}) = 0;

    virtual void fill_text(const char *text, const glm::vec2 &pos,
                           float font_size = 16,
                           const glm::u8vec4 &color = {255, 255, 255, 255},
                           my::Font *font = nullptr) = 0;

    virtual void draw() = 0;
};

std::shared_ptr<Canvas> make_vulkan_canvas(VulkanCtx *ctx,
                                           RenderDevice *device);

} // namespace my
