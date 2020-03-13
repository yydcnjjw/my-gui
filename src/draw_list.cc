#include "draw_list.h"

namespace my {

DrawPath::DrawPath(const glm::vec2 begin) { this->_points.push_back(begin); }

DrawPath &DrawPath::line_to(const glm::vec2 &pos) {
    this->_points.push_back(pos);
    return *this;
}
DrawPath &DrawPath::rect(const glm::vec2 &a, const glm::vec2 &b) {
    if (this->_points.size() == 1) {
        this->_points[0] = a;
    } else {
        this->line_to(a);
    }

    this->line_to({b.x, a.y});
    this->line_to(b);
    this->line_to({a.x, b.y});
    return *this;
}

void DrawList::_add_poly_line(const DrawPath &path, const glm::u8vec4 &col,
                              float line_width) {
    const size_t point_count = path._points.size();

    if (point_count < 2) {
        return;
    }

    glm::vec2 uv;

    size_t count = point_count - 1;

    const size_t idx_count = count * 6;
    const size_t vtx_count = count * 4;

    for (auto i = 0; i < count; ++i) {
        const int i2 = i + 1;
        assert(i2 < point_count);
        size_t current_idx = this->_vtx_list.size();

        const glm::vec2 &p1 = path._points[i];
        const glm::vec2 &p2 = path._points[i2];

        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;

        float d2 = dx * dx + dy * dy;
        if (d2 > 0.0f) {
            float inv_len = 1.0f / std::sqrt(d2);
            dx *= inv_len;
            dy *= inv_len;
        }

        dx *= (line_width * 0.5f);
        dy *= (line_width * 0.5f);

        this->_vtx_list.push_back({{p1.x + dy, p1.y - dx}, uv, col});
        this->_vtx_list.push_back({{p2.x + dy, p2.y - dx}, uv, col});
        this->_vtx_list.push_back({{p2.x - dy, p2.y + dx}, uv, col});
        this->_vtx_list.push_back({{p1.x - dy, p1.y + dx}, uv, col});

        this->_idx_list.push_back(current_idx);
        this->_idx_list.push_back(current_idx + 1);
        this->_idx_list.push_back(current_idx + 2);
        this->_idx_list.push_back(current_idx);
        this->_idx_list.push_back(current_idx + 2);
        this->_idx_list.push_back(current_idx + 3);
    }
}
    
void DrawList::_add_convex_poly_fill(const DrawPath &path, const glm::u8vec4 &col) {
    const size_t point_count = path._points.size();

    glm::vec2 uv;

    const size_t idx_count = (point_count - 2) * 3;
    const size_t vtx_count = point_count;

    size_t current_idx = this->_vtx_list.size();

    for (auto i = 0; i < vtx_count; ++i) {
        this->_vtx_list.push_back({path._points[i], uv, col});
    }

    for (auto i = 2; i < point_count; i++) {
        this->_idx_list.push_back(current_idx);
        this->_idx_list.push_back(current_idx + i - 1);
        this->_idx_list.push_back(current_idx + i);
    }
}

} // namespace my
