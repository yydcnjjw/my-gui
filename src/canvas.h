#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "vulkan_ctx.h"

namespace my {

class Canvas {
  public:
    Canvas() = default;
    virtual ~Canvas() = default;

    virtual void fill_rect(const glm::vec2 &a, const glm::vec2 &b,
                           const glm::u8vec4 &) = 0;

    virtual void stroke_rect(const glm::vec2 &a, const glm::vec2 &b,
                             const glm::u8vec4 &, float line_width) = 0;
    virtual void draw() = 0;
};

std::shared_ptr<Canvas>
make_vulkan_canvas(VulkanCtx *ctx, RenderDevice *device);

} // namespace my
