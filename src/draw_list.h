#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <vector>

#include "logger.h"

namespace my {

struct DrawVert {
    glm::vec2 pos;
    glm::vec2 uv;
    glm::u8vec4 col;
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

    DrawPath &rect(const glm::vec2 &a, const glm::vec2 &b);

  private:
    std::vector<glm::vec2> _points;
};
#define _CHECK_CURRENT_PATH                                                    \
    if (!this->_current_path) {                                                \
        GLOG_W("current path is reset");                                       \
        return *this;                                                          \
    }

class DrawList {
  public:
    DrawList &fill_rect(const glm::vec2 &a, const glm::vec2 &b,
                        const glm::u8vec4 &col) {
        return this->begin_path(a).rect(a, b).fill(col);
    }

    DrawList &stroke_rect(const glm::vec2 &a, const glm::vec2 &b,
                          const glm::u8vec4 &col, float line_width) {
        return this->begin_path(a).rect(a, b).close_path().stroke(col, line_width);
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

    DrawList &rect(const glm::vec2 &a, const glm::vec2 &b) {
        _CHECK_CURRENT_PATH;
        this->_current_path->rect(a, b);
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

    const std::vector<DrawVert> &get_draw_vtx() { return this->_vtx_list; }

    const std::vector<uint32_t> &get_draw_idx() { return this->_idx_list; }

    void clear() {
        this->_vtx_list.clear();
        this->_idx_list.clear();
        this->_current_path.reset();
    }

  private:
    std::vector<DrawVert> _vtx_list;
    std::vector<uint32_t> _idx_list;

    std::shared_ptr<DrawPath> _current_path;

    void _add_poly_line(const DrawPath &, const glm::u8vec4 &col,
                        float line_width);
    void _add_convex_poly_fill(const DrawPath &, const glm::u8vec4 &col);
};
} // namespace my
