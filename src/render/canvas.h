#pragma once

#include <memory>
#include <stack>
#include <vector>

#include <boost/gil.hpp>
#include <glm/glm.hpp>
#include <render/window/window_mgr.h>
#include <storage/font_mgr.h>
#include <storage/resource_mgr.hpp>
#include <util/event_bus.hpp>
#include <util/logger.h>

namespace my {
typedef LLGL::RenderSystem RenderSystem;
typedef glm::u8vec4 ColorRGBAub;
typedef boost::gil::rgba8_image_t RGBAImage;

struct DrawVert {
    glm::vec2 pos;
    glm::vec2 uv;
    ColorRGBAub col{255, 255, 255, 255};
};

class Canvas;
class DrawPath {
  public:
    friend class Canvas;
    DrawPath() : DrawPath({0.0f, 0.0f}) {}
    DrawPath(const glm::vec2 begin);

    DrawPath &line_to(const glm::vec2 &pos);

    /* DrawPath &arc() { return *this; } */

    /* DrawPath &arc_to() { return *this; } */

    // DrawPath &ellipse() { return *this; }

    DrawPath &rect(const glm::vec2 &a, const glm::vec2 &c);

  private:
    std::vector<glm::vec2> _points;
};
#define _CHECK_CURRENT_PATH                                                    \
    if (!this->_current_path) {                                                \
        GLOG_W("current path is reset");                                       \
        return *this;                                                          \
    }

struct DrawState {
    std::shared_ptr<Image> image;
};

struct DrawCmd {
    DrawState state;
    uint32_t elem_count;
    uint32_t idx_offset = 0;
    uint32_t vtx_offset = 0;
};

class Canvas {
  public:
    Canvas(RenderSystem *renderer, Window *win, EventBus *bus,
           ResourceMgr *resource_mgr, FontMgr *font_mgr);
    ~Canvas();

    Canvas &fill_rect(const glm::vec2 &a, const glm::vec2 &c,
                      const ColorRGBAub &col) {
        return this->begin_path(a).rect(a, c).fill(col);
    }

    Canvas &stroke_rect(const glm::vec2 &a, const glm::vec2 &c,
                        const ColorRGBAub &col, float line_width) {
        return this->begin_path(a).rect(a, c).close_path().stroke(col,
                                                                  line_width);
    }

    Canvas &begin_path(const glm::vec2 &pos) {
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);
        this->_current_path.reset(new DrawPath(pos));
        return *this;
    }

    Canvas &line_to(const glm::vec2 &pos) {
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);
        _CHECK_CURRENT_PATH;
        this->_current_path->line_to(pos);
        return *this;
    }

    Canvas &rect(const glm::vec2 &a, const glm::vec2 &c) {
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);
        _CHECK_CURRENT_PATH;
        this->_current_path->rect(a, c);
        return *this;
    }

    Canvas &close_path() {
        glm::vec2 pos;
        {
            std::shared_lock<std::shared_mutex> l_lock(this->_lock);
            _CHECK_CURRENT_PATH;
            pos = this->_current_path->_points.front();
        }

        this->line_to(pos);
        return *this;
    }

    Canvas &fill(const glm::u8vec4 &col) {
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);
        _CHECK_CURRENT_PATH;
        this->_add_convex_poly_fill(*this->_current_path, col);
        this->_current_path.reset();
        return *this;
    }

    Canvas &stroke(const ColorRGBAub &col, float line_width) {
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);
        _CHECK_CURRENT_PATH;
        this->_add_poly_line(*this->_current_path, col, line_width);
        this->_current_path.reset();

        return *this;
    }

    Canvas &draw_image(std::shared_ptr<Image> image, const glm::vec2 &p_min,
                       const glm::vec2 &p_max, const glm::vec2 &uv_min = {0, 0},
                       const glm::vec2 &uv_max = {1, 1}, uint8_t alpha = 255);

    Canvas &fill_text(const std::string &text, const glm::vec2 &pos,
                      my::Font *font = nullptr, float font_size = 16,
                      const ColorRGBAub &color = {255, 255, 255, 255});

    std::shared_ptr<RGBAImage> get_image_data(const PixelPos &offset,
                                              const Size2D &size) {
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);

        auto [canvas_w, canvas_h] = this->_canvas_tex->get_size();
        uint32_t w{size.w}, h{size.h};
        if (offset.x + size.w > canvas_w) {
            w = canvas_w - offset.x;
        }

        if (offset.y + size.h > canvas_h) {
            h = canvas_h - offset.y;
        }

        return this->_canvas_tex->get_image_data(offset.x, offset.y, w, h);
    }

    void put_image_data(std::shared_ptr<RGBAImage> data,
                        const PixelPos &offset) {
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);
        uint32_t src_w = data->width();
        uint32_t src_h = data->height();
        auto [canvas_w, canvas_h] = this->_canvas_tex->get_size();

        uint32_t w{src_w}, h{src_h};
        if (offset.x + src_w > canvas_w) {
            w = canvas_w - offset.x;
        }

        if (offset.y + src_h > canvas_h) {
            h = canvas_h - offset.y;
        }

        RGBAImage::const_view_t image_view{};

        if (w != src_w && h != src_h) {
            image_view = boost::gil::subimage_view(boost::gil::view(*data),
                                                   {0, 0}, {w, h});
        } else {
            image_view = boost::gil::const_view(*data);
        }

        this->_canvas_tex->put_image_data(image_view, offset.x, offset.y);
    }

    void render();

    void clear() {
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);
        this->_vtx_list.clear();
        this->_idx_list.clear();
        this->_cmd_list.clear();
        this->_current_path.reset();
        this->_current_cmd = {};

        for (auto &tex : this->_textures) {
            tex.second.release();
        }
        this->_textures.clear();
    }

  private:
    RenderSystem *_renderer{};
    LLGL::RenderContext *_context{};
    LLGL::ShaderProgram *_shader{};
    std::array<LLGL::PipelineState *, 2> _pipeline{};
    LLGL::PipelineLayout *_pipeline_layout{};
    LLGL::CommandQueue *_queue{};
    LLGL::CommandBuffer *_commands{};
    LLGL::Fence *_fence;

    LLGL::VertexFormat _vertex_format;
    std::vector<DrawVert> _vtx_list;
    std::vector<uint32_t> _idx_list;
    LLGL::Buffer *_vtx_buf{};
    LLGL::Buffer *_idx_buf{};
    LLGL::ResourceHeap *_default_resource{};

    LLGL::Sampler *_sampler{};

    my::Font *_default_font{};
    LLGL::Texture *_font_tex{};
    struct ConstBlock {
        glm::vec2 scale;
        glm::vec2 translate;
    };

    ConstBlock _const_block;
    LLGL::Buffer *_constant{};

    class Texture {
      public:
        LLGL::Texture *texture{};
        LLGL::ResourceHeap *resource{};
        LLGL::Extent2D get_size() {
            auto extent = this->texture->GetDesc().extent;
            return {extent.width, extent.height};
        }

        Texture(LLGL::Texture *texture, LLGL::ResourceHeap *resource,
                RenderSystem *renderer)
            : texture(texture), resource(resource), _renderer(renderer) {}
        void release() {
            this->_renderer->Release(*this->resource);
            this->_renderer->Release(*this->texture);
            this->resource = nullptr;
            this->texture = nullptr;
        }

        std::shared_ptr<RGBAImage> get_image_data(int32_t left, int32_t top,
                                                  uint32_t w, uint32_t h) {
            LLGL::TextureRegion region{{left, top, 0}, {w, h, 1}};

            auto image = std::make_shared<RGBAImage>(w, h);
            LLGL::DstImageDescriptor dst(
                LLGL::ImageFormat::RGBA, LLGL::DataType::UInt8,
                boost::gil::interleaved_view_get_raw_data(
                    boost::gil::view(*image)),
                w * h * sizeof(LLGL::ColorRGBAub));
            this->_renderer->ReadTexture(*this->texture, region, dst);
            return image;
        }

        void put_image_data(RGBAImage::const_view_t data, int32_t left,
                            int32_t top) {
            uint32_t w = data.width();
            uint32_t h = data.height();
            LLGL::TextureRegion region{{left, top, 0}, {w, h, 1}};
            LLGL::SrcImageDescriptor src(
                LLGL::ImageFormat::RGBA, LLGL::DataType::UInt8,
                boost::gil::interleaved_view_get_raw_data(data),
                w * h * sizeof(LLGL::ColorRGBAub));
            this->_renderer->WriteTexture(*this->texture, region, src);
        }

      private:
        RenderSystem *_renderer;
    };
    std::map<std::shared_ptr<Image>, Texture> _textures;

    std::shared_ptr<Texture> _canvas_tex;
    LLGL::Buffer *_canvas_vtx{};
    LLGL::Buffer *_canvas_idx{};

    LLGL::RenderTarget *_render_target;

    // first cmd
    std::vector<DrawCmd> _cmd_list;

    std::shared_ptr<DrawPath> _current_path;
    DrawCmd _current_cmd;
    std::stack<DrawState> _state_stack;

    uint32_t _vtx_offset = 0;

    std::shared_mutex _lock;

    my::Window *_window;
    my::Size2D _size;

    void _make_context_resource();
    void _release_context_resource();
    void _resize_handle();

    DrawState &_get_state() { return this->_current_cmd.state; }
    void _set_state(const DrawState &state) {
        this->_current_cmd.state = state;
    }

    void _add_cmd();
    const std::vector<DrawCmd> &_get_draw_cmd();

    void _save();
    void _restore();

    void _prim_rect(const glm::vec2 &a, const glm::vec2 &c,
                    const ColorRGBAub &col);
    void _prim_rect_uv(const glm::vec2 &a, const glm::vec2 &c,
                       const glm::vec2 &uv_a, const glm::vec2 &uv_c,
                       const ColorRGBAub &col = {0, 0, 0, 0});

    void _add_poly_line(const DrawPath &, const ColorRGBAub &col,
                        float line_width);
    void _add_convex_poly_fill(const DrawPath &, const ColorRGBAub &col);
};
} // namespace my
