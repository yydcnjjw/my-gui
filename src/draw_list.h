#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <stack>
#include <vector>

#include "logger.h"
#include "font_mgr.h"

namespace my {

struct DrawVert {
    glm::vec2 pos;
    glm::vec2 uv;
    glm::u8vec4 col = {255, 255, 255, 255};
};

class DrawList;
class DrawPath {
  public:
    friend class DrawList;
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

typedef void *TextureID;

struct DrawState {
    TextureID texture_id = nullptr;
    bool is_font = false;
};
struct DrawCmd {
    DrawState state;
    uint32_t elem_count;
    uint32_t idx_offset = 0;
    uint32_t vtx_offset = 0;
};

class DrawList {
  public:
    DrawList &fill_rect(const glm::vec2 &a, const glm::vec2 &c,
                        const glm::u8vec4 &col) {
        return this->begin_path(a).rect(a, c).fill(col);
    }

    DrawList &stroke_rect(const glm::vec2 &a, const glm::vec2 &c,
                          const glm::u8vec4 &col, float line_width) {
        return this->begin_path(a).rect(a, c).close_path().stroke(col,
                                                                  line_width);
    }

    DrawList &begin_path(const glm::vec2 &pos) {
        this->_current_path.reset(new DrawPath(pos));
        return *this;
    }

    DrawList &line_to(const glm::vec2 &pos) {
        _CHECK_CURRENT_PATH;
        this->_current_path->line_to(pos);
        return *this;
    }

    DrawList &rect(const glm::vec2 &a, const glm::vec2 &c) {
        _CHECK_CURRENT_PATH;
        this->_current_path->rect(a, c);
        return *this;
    }

    DrawList &close_path() {
        _CHECK_CURRENT_PATH;
        this->line_to(this->_current_path->_points.front());
        return *this;
    }

    DrawList &fill(const glm::u8vec4 &col) {
        _CHECK_CURRENT_PATH;

        this->_add_convex_poly_fill(*this->_current_path, col);
        this->_current_path.reset();
        return *this;
    }

    DrawList &stroke(const glm::u8vec4 &col, float line_width) {
        _CHECK_CURRENT_PATH;

        this->_add_poly_line(*this->_current_path, col, line_width);
        this->_current_path.reset();
        return *this;
    }
    DrawList &draw_image(TextureID id, const glm::vec2 &p_min,
                         const glm::vec2 &p_max,
                         const glm::vec2 &uv_min = {0, 0},
                         const glm::vec2 &uv_max = {1, 1}, uint8_t alpha = 255);

    DrawList &fill_text(const char *text, const glm::vec2 &pos,
                        my::Font *font, float font_size,
                        const glm::u8vec4 &color);

    const std::vector<DrawVert> &get_draw_vtx() { return this->_vtx_list; }

    const std::vector<uint32_t> &get_draw_idx() { return this->_idx_list; }
    const std::vector<DrawCmd> &get_draw_cmd();

    void clear() {
        this->_vtx_list.clear();
        this->_idx_list.clear();
        this->_cmd_list.clear();
        this->_current_path.reset();
        this->_current_cmd = {};
    }

  private:
    std::vector<DrawVert> _vtx_list;
    std::vector<uint32_t> _idx_list;

    // first cmd
    std::vector<DrawCmd> _cmd_list;

    std::shared_ptr<DrawPath> _current_path;
    DrawCmd _current_cmd;
    std::stack<DrawState> _state_stack;

    uint32_t _vtx_offset = 0;

    DrawState &_get_state() { return this->_current_cmd.state; }
    void _set_state(const DrawState &state) {
        this->_current_cmd.state = state;
    }

    void _add_cmd();

    void _save();
    void _restore();

    void _prim_rect(const glm::vec2 &a, const glm::vec2 &c,
                    const glm::u8vec4 &col);
    void _prim_rect_uv(const glm::vec2 &a, const glm::vec2 &c,
                       const glm::vec2 &uv_a, const glm::vec2 &uv_c,
                       const glm::u8vec4 &col = {0, 0, 0, 0});

    void _add_poly_line(const DrawPath &, const glm::u8vec4 &col,
                        float line_width);
    void _add_convex_poly_fill(const DrawPath &, const glm::u8vec4 &col);
};
} // namespace my
