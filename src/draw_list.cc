#include "draw_list.h"

#include <cstring>

namespace my {

DrawPath::DrawPath(const glm::vec2 begin) { this->_points.push_back(begin); }

DrawPath &DrawPath::line_to(const glm::vec2 &pos) {
    this->_points.push_back(pos);
    return *this;
}
DrawPath &DrawPath::rect(const glm::vec2 &a, const glm::vec2 &c) {
    if (this->_points.size() == 1) {
        this->_points[0] = a;
    } else {
        this->line_to(a);
    }

    this->line_to({c.x, a.y});
    this->line_to(c);
    this->line_to({a.x, c.y});
    return *this;
}

DrawList &DrawList::draw_image(TextureID id, const glm::vec2 &p_min,
                               const glm::vec2 &p_max, const glm::vec2 &uv_min,
                               const glm::vec2 &uv_max, uint8_t alpha) {
    if (alpha == 0) {
        return *this;
    }

    this->_save();
    this->_get_state().texture_id = id;

    this->_prim_rect_uv(p_min, p_max, uv_min, uv_max, {255, 255, 255, alpha});

    this->_restore();
    return *this;
}

DrawList &DrawList::fill_text(const char *text, const glm::vec2 &p,
                              my::Font *font, float font_size,
                              const glm::u8vec4 &color) {
    auto text_len = std::strlen(text);
    auto wtext_len = std::mbstowcs(nullptr, text, text_len);
    std::wstring wtext(wtext_len, 0);
    std::mbstowcs(wtext.data(), text, text_len);
    this->_save();
    this->_get_state().is_font = true;

    float scale = font_size / font->get_default_font_size();

    glm::vec2 pos = p;
    pos.y += font_size;
    glm::vec2 p_min;
    glm::vec2 p_max;
    for (auto ch : wtext) {
        const auto &glyph = font->get_glyph(ch);
        const auto w = glyph.w * scale;
        const auto h = glyph.h * scale;
        p_min.x = pos.x + glyph.bearing.x;
        p_min.y = pos.y - (h - glyph.h * scale) - h;
        p_max.x = p_min.x + w;
        p_max.y = p_min.y + h;
        this->_prim_rect_uv(p_min, p_max, glyph.uv0, glyph.uv1, color);
        pos.x += glyph.advance_x * scale;
    }
    this->_restore();
    return *this;
}

const std::vector<DrawCmd> &DrawList::get_draw_cmd() {
    this->_add_cmd();
    return this->_cmd_list;
}

void DrawList::_add_cmd() {
    auto idx_count = this->_idx_list.size();
    uint32_t elem_count;
    if (this->_cmd_list.empty()) {
        elem_count = idx_count;
    } else {
        auto &prev_cmd = this->_cmd_list.back();
        auto prev_idx_offset = prev_cmd.idx_offset;
        auto prev_elem_count = prev_cmd.elem_count;
        elem_count = idx_count - (prev_idx_offset + prev_elem_count);
    }

    if (elem_count == 0) {
        return;
    }

    this->_current_cmd.elem_count = elem_count;
    this->_cmd_list.push_back(this->_current_cmd);
    this->_current_cmd.idx_offset = idx_count;
    this->_current_cmd.vtx_offset = this->_vtx_list.size();
}

void DrawList::_save() {
    this->_state_stack.push(this->_get_state());
    this->_add_cmd();
}
void DrawList::_restore() {
    this->_add_cmd();

    this->_set_state(this->_state_stack.top());
    this->_state_stack.pop();
}

void DrawList::_prim_rect(const glm::vec2 &a, const glm::vec2 &c,
                          const glm::u8vec4 &col) {
    this->_prim_rect_uv(a, c, {0, 0}, {0, 0}, col);
}

void DrawList::_prim_rect_uv(const glm::vec2 &a, const glm::vec2 &c,
                             const glm::vec2 &uv_a, const glm::vec2 &uv_c,
                             const glm::u8vec4 &col) {
    glm::vec2 b(c.x, a.y), d(a.x, c.y), uv_b(uv_c.x, uv_a.y),
        uv_d(uv_a.x, uv_c.y);
    auto idx = this->_vtx_list.size();
    this->_vtx_list.push_back({a, uv_a, col});
    this->_vtx_list.push_back({b, uv_b, col});
    this->_vtx_list.push_back({c, uv_c, col});
    this->_vtx_list.push_back({d, uv_d, col});
    this->_idx_list.push_back(idx);
    this->_idx_list.push_back(idx + 1);
    this->_idx_list.push_back(idx + 2);
    this->_idx_list.push_back(idx);
    this->_idx_list.push_back(idx + 2);
    this->_idx_list.push_back(idx + 3);
}

void DrawList::_add_poly_line(const DrawPath &path, const glm::u8vec4 &col,
                              float line_width) {
    const size_t point_count = path._points.size();

    if (point_count < 2) {
        return;
    }

    glm::vec2 uv;

    size_t count = point_count - 1;

    for (size_t i = 0; i < count; ++i) {
        const auto i2 = i + 1;
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

void DrawList::_add_convex_poly_fill(const DrawPath &path,
                                     const glm::u8vec4 &col) {
    const size_t point_count = path._points.size();

    glm::vec2 uv;

    // const size_t idx_count = (point_count - 2) * 3;
    const size_t vtx_count = point_count;

    size_t current_idx = this->_vtx_list.size();

    for (size_t i = 0; i < vtx_count; ++i) {
        this->_vtx_list.push_back({path._points[i], uv, col});
    }

    for (size_t i = 2; i < point_count; i++) {
        this->_idx_list.push_back(current_idx);
        this->_idx_list.push_back(current_idx + i - 1);
        this->_idx_list.push_back(current_idx + i);
    }
}

} // namespace my
