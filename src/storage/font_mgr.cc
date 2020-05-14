#include "font_mgr.h"

#include <my_gui.hpp>
// #include <util/codecvt.h>
#include <util/logger.h>
#define STB_RECT_PACK_IMPLEMENTATION
#include <util/stb_rect_pack.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/glm.hpp>
#include <msdfgen.h>

#include <boost/format.hpp>

#define FT_CEIL(X) (((X + 63) & -64) / 64)

namespace {
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
        : charcode(charcode), w(slot->bitmap.width), h(slot->bitmap.rows),
          offset_x(slot->bitmap_left), offset_y(slot->bitmap_top),
          advance_x(FT_CEIL(slot->advance.x)) {}
};

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
    MyFont(FT_Library ft_lib, const std::string &path) : _font_size(24) {
        FT_Error e = ::FT_New_Face(ft_lib, path.c_str(), 0, &this->_ft_face);
        FT_Open_Face(FT_Library library, const FT_Open_Args *args, FT_Long face_index, FT_Face *aface)
        if (e) {
            throw std::runtime_error(FT_Error_String(e));
        }

        ::FT_Set_Pixel_Sizes(this->_ft_face, 0, this->_font_size);
    }
    ~MyFont() { FT_Done_Face(this->_ft_face); }

    uint32_t font_size() const override { return this->_font_size; };

    glm::vec2 white_pixels_uv() override { return this->_white_pixels_uv; }

    u_char *get_tex_as_alpha(int *out_w, int *out_h) override {
        if (this->_alpha_tex) {
            *out_w = this->_tex_w;
            *out_h = this->_tex_h;
            return this->_alpha_tex;
        }
        const size_t BITMAP_BUF_CHUNK_SIZE = 256 * 1024;
        std::vector<std::shared_ptr<u_char[]>> bitmap_buf{
            std::shared_ptr<u_char[]>(new u_char[BITMAP_BUF_CHUNK_SIZE])};
        size_t bitmap_buf_used = 0;

        std::vector<GlyphData> glyphs;

        FT_UInt index;
        auto char_code = FT_Get_First_Char(this->_ft_face, &index);
        while (index != 0) {
            auto &slot = this->_ft_face->glyph;

            FT_Error e = FT_Load_Glyph(this->_ft_face, index, FT_LOAD_DEFAULT);
            if (e) {
                throw std::runtime_error(FT_Error_String(e));
            }

            e = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
            if (e) {
                throw std::runtime_error(FT_Error_String(e));
            }
            GlyphData data(slot, char_code);

            size_t alloc_size = data.w * data.h;
            if (alloc_size + bitmap_buf_used > BITMAP_BUF_CHUNK_SIZE) {
                bitmap_buf.push_back(std::shared_ptr<u_char[]>(
                    new u_char[BITMAP_BUF_CHUNK_SIZE]));
                bitmap_buf_used = 0;
            }

            data.pixels = bitmap_buf.back().get() + bitmap_buf_used;
            bitmap_buf_used += alloc_size;

            blit_glyph(&slot->bitmap, data.pixels, data.w);
            glyphs.push_back(data);

            char_code = FT_Get_Next_Char(this->_ft_face, char_code, &index);
        }

        // for (const auto &range : this->get_glyph_ranges_default()) {
        //     for (wchar_t ch = range.first; ch <= range.second; ++ch) {
        //         uint32_t index = FT_Get_Char_Index(this->_ft_face, ch);
        //         if (!index) {
        //             continue;
        //         }
        //         auto &slot = this->_ft_face->glyph;

        //         FT_Error e =
        //             FT_Load_Glyph(this->_ft_face, index, FT_LOAD_DEFAULT);
        //         if (e) {
        //             throw std::runtime_error(FT_Error_String(e));
        //         }

        //         e = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
        //         if (e) {
        //             throw std::runtime_error(FT_Error_String(e));
        //         }

        //         GlyphData data(slot, ch);

        //         size_t alloc_size = data.w * data.h;
        //         if (alloc_size + bitmap_buf_used > BITMAP_BUF_CHUNK_SIZE) {
        //             bitmap_buf.push_back(std::shared_ptr<u_char[]>(
        //                 new u_char[BITMAP_BUF_CHUNK_SIZE]));
        //         }
        //         data.pixels = bitmap_buf.back().get() + bitmap_buf_used;
        //         bitmap_buf_used += alloc_size;

        //         blit_glyph(&slot->bitmap, data.pixels, data.w);
        //         glyphs.push_back(data);
        //     }
        // }

        const size_t TEX_HEIGHT_MAX = 1024 * 32;
        const size_t TEX_WIDTH = 1024;
        stbrp_context pack_ctx;
        std::vector<stbrp_node> pack_nodes(TEX_WIDTH);

        ::stbrp_init_target(&pack_ctx, TEX_WIDTH, TEX_HEIGHT_MAX,
                            pack_nodes.data(), pack_nodes.size());

        std::vector<stbrp_rect> rects(glyphs.size() + 1);
        for (size_t i = 0; i < glyphs.size(); i++) {
            rects[i].w = glyphs[i].w;
            rects[i].h = glyphs[i].h;
        }

        auto white_pixels = rects.back();
        white_pixels.w = this->_font_size;
        white_pixels.h = this->_font_size;

        ::stbrp_pack_rects(&pack_ctx, rects.data(), rects.size());

        int tex_h = 0;
        for (size_t i = 0; i < rects.size() - 1; i++) {
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
        GLOG_D("char number %d font texture w = %d h = %d", glyphs.size(),
               this->_tex_w, this->_tex_h);

        for (size_t i = 0; i < glyphs.size(); i++) {
            if (!rects[i].was_packed) {
                GLOG_W("%#x was not packed", glyphs[i].charcode);
                continue;
            }
            auto &data = glyphs[i];

            auto blit_src_stride = data.w;
            auto blit_dst_stride = this->_tex_w;
            auto blit_src = data.pixels;
            auto blit_dst = this->_alpha_tex + (data.y * this->_tex_w) + data.x;
            for (int y = data.h; y > 0; y--, blit_src += blit_src_stride,
                     blit_dst += blit_dst_stride) {
                memcpy(blit_dst, blit_src, blit_src_stride);
            }

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

        // set white pixels
        auto blit_src_stride = white_pixels.w;
        auto blit_dst_stride = this->_tex_w;
        auto blit_dst =
            this->_alpha_tex + (white_pixels.y * this->_tex_w) + white_pixels.x;
        for (int y = white_pixels.h; y > 0; y--, blit_dst += blit_dst_stride) {
            memset(blit_dst, 255, blit_src_stride);
        }
        this->_white_pixels_uv = {
            white_pixels.x + white_pixels.w / 2 / (float)this->_tex_w,
            white_pixels.y + white_pixels.h / 2 / (float)this->_tex_h};
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
        if (this->_glyph_map.find(ch) == this->_glyph_map.end()) {
            throw std::runtime_error(
                (boost::format("unknown glyph %1%") % ch).str());
        }
        return this->_glyph_map.at(ch);
    }

    FT_Face _ft_face;

  private:
    uint32_t _font_size;
    u_char *_rgb32_tex = nullptr;
    u_char *_alpha_tex = nullptr;
    size_t _tex_w;
    size_t _tex_h;
    glm::vec2 _white_pixels_uv;
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
        _fonts.clear();
        FT_Done_FreeType(this->_ft_lib);
    }

    my::Font *get_font(const std::string &name) override {
        auto it =
            std::find_if(this->_fonts.cbegin(), this->_fonts.cend(),
                         [&name](const font_map::value_type &v) {
                             return !my::fs::path(v.first).stem().compare(name);
                         });
        if (it == this->_fonts.end()) {
            throw std::runtime_error("font is not exist");
        } else {
            return it->second.get();
        }
    };

    my::Font *add_font(const std::string &path) override {
        auto it = this->_fonts.find(path);
        if (it != this->_fonts.end()) {
            return it->second.get();
        }
        auto font = std::make_unique<MyFont>(this->_ft_lib, path);
        auto ptr = font.get();
        this->_fonts.insert({path, std::move(font)});
        return ptr;
    }

  private:
    FT_Library _ft_lib;
    typedef std::map<std::string, std::unique_ptr<my::Font>> font_map;
    font_map _fonts;
};
} // namespace
namespace my {
std::unique_ptr<FontMgr> FontMgr::create() {
    return std::make_unique<MyFontMgr>();
}
} // namespace my
