#include "font_mgr.h"

#include "logger.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#include <glm/glm.hpp>

namespace {

void blit_glyph(const FT_Bitmap *ft_bitmap, uint8_t *dst, uint32_t dst_pitch) {
    assert(dst);
    const auto w = ft_bitmap->width;
    const auto h = ft_bitmap->rows;
    const uint8_t *src = ft_bitmap->buffer;
    const uint32_t src_pitch = ft_bitmap->pitch;
    switch (ft_bitmap->pixel_mode) {
    case FT_PIXEL_MODE_GRAY: {
        for (uint32_t y = 0; y < h; y++, src += src_pitch, dst += dst_pitch) {
            ::memcpy(dst, src, w);
        }
        break;
    }
    case FT_PIXEL_MODE_MONO: {
        uint8_t color0 = 0;
        uint8_t color1 = 255;
        for (uint32_t y = 0; y < h; y++, src += src_pitch, dst += dst_pitch) {
            uint8_t bits = 0;
            const uint8_t *bits_ptr = src;
            for (uint32_t x = 0; x < w; x++, bits <<= 1) {
                if ((x & 7) == 0)
                    bits = *bits_ptr++;
                dst[x] = (bits & 0x80) ? color1 : color0;
            }
        }
        break;
    }
    default:
        break;
    }
}

class MyFont : public my::Font {
  public:
    MyFont(FT_Library ft_lib, const char *path) : _default_font_size(16) {
        FT_Error e = ::FT_New_Face(ft_lib, path, 0, &this->_ft_face);
        if (e) {
            throw std::runtime_error(FT_Error_String(e));
        }

        FT_Set_Pixel_Sizes(this->_ft_face, 0, this->_default_font_size);

    }
    ~MyFont() { FT_Done_Face(this->_ft_face); }

    uint32_t get_default_font_size() const override {
        return this->_default_font_size;
    };
    u_char *get_tex_as_alpha(int *out_w, int *out_h) override {
        if (this->_alpha_tex) {
            *out_w = this->_tex_w;
            *out_h = this->_tex_h;
            return this->_alpha_tex;
        }
        const size_t BITMAP_BUF_CHUNK_SIZE = 256 * 1024;
        std::vector<u_char *> bitmap_buf{new u_char[BITMAP_BUF_CHUNK_SIZE]};
        size_t bitmap_buf_used = 0;
#define FT_CEIL(X) (((X + 63) & -64) / 64)
        struct GlyphData {
            wchar_t charcode;
            uint32_t w;
            uint32_t h;
            int offset_x;
            int offset_y;
            float advance_x;
            uint32_t x;
            u_char *pixels;
            uint32_t y;

            GlyphData(const FT_GlyphSlot &slot, wchar_t charcode)
                : charcode(charcode), w(slot->bitmap.width),
                  h(slot->bitmap.rows), offset_x(slot->bitmap_left),
                  offset_y(slot->bitmap_top),
                  advance_x(FT_CEIL(slot->advance.x)) {}
        };

        std::vector<GlyphData> glyphs;

        for (const auto &range : this->get_glyph_ranges_default()) {
            for (wchar_t ch = range.first; ch <= range.second; ++ch) {
                uint32_t index = FT_Get_Char_Index(_ft_face, ch);
                if (!index) {
                    continue;
                }
                auto &slot = this->_ft_face->glyph;

                FT_Error e =
                    FT_Load_Glyph(this->_ft_face, index, FT_LOAD_DEFAULT);
                if (e) {
                    throw std::runtime_error(FT_Error_String(e));
                }

                e = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
                if (e) {
                    throw std::runtime_error(FT_Error_String(e));
                }

                GlyphData data(slot, ch);

                size_t alloc_size = data.w * data.h;
                if (alloc_size + bitmap_buf_used > BITMAP_BUF_CHUNK_SIZE) {
                    bitmap_buf.push_back(new u_char[BITMAP_BUF_CHUNK_SIZE]);
                }
                data.pixels = bitmap_buf.back() + bitmap_buf_used;
                bitmap_buf_used += alloc_size;

                blit_glyph(&slot->bitmap, data.pixels, data.w);
                glyphs.push_back(data);
            }
        }

        const size_t TEX_HEIGHT_MAX = 1024 * 32;
        const size_t TEX_WIDTH = 512;
        const size_t TEX_GLYPH_PADDING = 1;
        stbrp_context pack_ctx;
        std::vector<stbrp_node> pack_nodes(TEX_WIDTH + TEX_GLYPH_PADDING);

        ::stbrp_init_target(&pack_ctx, TEX_WIDTH, TEX_HEIGHT_MAX,
                            pack_nodes.data(), pack_nodes.size());

        std::vector<stbrp_rect> rects(glyphs.size());
        for (auto i = 0; i < glyphs.size(); i++) {
            rects[i].w = glyphs[i].w;
            rects[i].h = glyphs[i].h;
        }

        ::stbrp_pack_rects(&pack_ctx, rects.data(), rects.size());

        int tex_h = 0;
        for (auto i = 0; i < rects.size(); i++) {
            if (!rects[i].was_packed) {
                continue;
            }
            tex_h = std::max(tex_h, rects[i].y + rects[i].h);
            glyphs[i].x = rects[i].x;
            glyphs[i].y = rects[i].y;
        }

        this->_tex_w = TEX_WIDTH;
        this->_tex_h = tex_h;
        this->_alpha_tex = new u_char[this->_tex_h * this->_tex_w];
        ::bzero(this->_alpha_tex, this->_tex_h * this->_tex_w);

        for (auto i = 0; i < glyphs.size(); i++) {
            if (!rects[i].was_packed) {
                GLOG_W("%#x was not packed", glyphs[i].charcode);
                continue;
            }
            auto &data = glyphs[i];
            // assert(data.w + TEX_GLYPH_PADDING <= rects[i].w);
            // assert(data.h + TEX_GLYPH_PADDING <= rects[i].h);

            // const int tx = data.x + TEX_GLYPH_PADDING;
            // const int ty = data.y + TEX_GLYPH_PADDING;

            auto blit_src_stride = data.w;
            auto blit_dst_stride = this->_tex_w;
            auto blit_src = data.pixels;
            auto blit_dst = this->_alpha_tex + (data.y * this->_tex_w) + data.x;
            for (int y = data.h; y > 0; y--, blit_src += blit_src_stride,
                     blit_dst += blit_dst_stride) {
                memcpy(blit_dst, blit_src, blit_src_stride);
            }

            // TODO:
            float u0 = data.x / (float)this->_tex_w;
            float v0 = data.y / (float)this->_tex_h;
            float u1 = (data.x + data.w) / (float)this->_tex_w;
            float v1 = (data.y + data.h) / (float)this->_tex_h;
            this->_glyph_map[glyphs[i].charcode] = {
                data.charcode,
                data.advance_x,
                data.w,
                data.h,
                {data.offset_x, data.offset_y},
                {u0, v0},
                {u1, v1}};
        }

        if (out_w) {
            *out_w = this->_tex_w;
        }

        if (out_h) {
            *out_h = this->_tex_h;
        }

        // return bitmap.buffer;
        return this->_alpha_tex;
    }

    u_char *get_tex_as_rgb32(int *out_w, int *out_h) override {
        if (this->_rgb32_tex) {
            *out_w = this->_tex_w;
            *out_h = this->_tex_h;
            return this->_rgb32_tex;
        }

        auto pixels = this->get_tex_as_alpha(out_w, out_h);
        assert(pixels);

        auto pixels_rgba32 = new glm::u8vec4[*out_w * *out_h];

        for (auto i = 0; i < *out_w * *out_h; ++i) {
            pixels_rgba32[i] = glm::u8vec4(255, 255, 255, *pixels++);
        }

        this->_rgb32_tex = reinterpret_cast<unsigned char *>(pixels_rgba32);
        return this->_rgb32_tex;
    }

    const my::FontGlyph &get_glyph(wchar_t ch) override {
        return this->_glyph_map[ch];
    }

    FT_Face _ft_face;

  private:
    uint32_t _default_font_size;
    u_char *_rgb32_tex = nullptr;
    u_char *_alpha_tex = nullptr;
    size_t _tex_w;
    size_t _tex_h;
    std::map<wchar_t, my::FontGlyph> _glyph_map;
};

class MyFontMgr : public my::FontMgr {
  public:
    MyFontMgr() {
        FT_Error e = ::FT_Init_FreeType(&this->_ft_lib);
        if (e) {
            throw std::runtime_error(FT_Error_String(e));
        }
    }
    ~MyFontMgr() {
        fonts.clear();
        FT_Done_FreeType(this->_ft_lib);
    }
    my::Font *add_font(const char *path) override {
        auto font = std::make_unique<MyFont>(this->_ft_lib, path);
        auto ptr = font.get();
        fonts.insert(std::move(font));
        return ptr;
    }

  private:
    FT_Library _ft_lib;

    std::set<std::unique_ptr<my::Font>> fonts;
};
} // namespace
namespace my {
FontMgr *get_font_mgr() {
    static auto mgr = MyFontMgr();
    return &mgr;
}
} // namespace my
