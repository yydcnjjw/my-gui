#pragma once

#include <memory>
#include <stdint.h>
#include <vector>

#include <glm/glm.hpp>

namespace my {

class FontConfig {};

struct FontGlyph {
    wchar_t charcode;
    float advance_x;
    uint32_t w, h;
    glm::ivec2 bearing;
    glm::vec2 uv0;
    glm::vec2 uv1;
};

class FontFamily {};

class Font {
  public:
    Font() = default;
    virtual ~Font() = default;
    virtual const my::FontGlyph &get_glyph(wchar_t ch) = 0;
    virtual unsigned char *get_tex_as_rgb32(int *out_w = nullptr,
                                            int *out_h = nullptr) = 0;
    virtual unsigned char *get_tex_as_alpha(int *out_w = nullptr,
                                            int *out_h = nullptr) = 0;

    virtual uint32_t font_size() const = 0;
    virtual glm::vec2 white_pixels_uv() = 0;

    typedef std::pair<wchar_t, wchar_t> GlyphRange;
    const std::vector<GlyphRange> &get_glyph_ranges_default() {
        static const std::vector<GlyphRange> ranges = {{0x0020, 0x00ff}};
        return ranges;
    }
};

class FontMgr {
  public:
    virtual ~FontMgr() = default;
    virtual Font *get_font(const std::string &name) = 0;
    virtual Font *add_font(const std::string &path) = 0;
    static std::unique_ptr<FontMgr> create();

  protected:
    FontMgr() = default;
};
} // namespace my
