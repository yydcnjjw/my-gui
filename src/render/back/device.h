#pragma once

namespace my {
namespace render {
class Device {
  public:
    virtual ~Device() = default;
    void make_context();
    void make_texture();
    void make_buffer();
    void make_frame_buffer();

    void make_swapchain();
    void make_render_pass();
    void make_pipeline_layout();
    void make_pipeline();
};

} // namespace render
} // namespace my
