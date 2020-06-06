#pragma once

#include <my_render.hpp>
#include <storage/blob.hpp>
#include <storage/resource.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <skia/include/core/SkFontMgr.h>

namespace my {

class Font : public Blob {
  public:
    ~Font() { // ::FT_Done_Face(this->_face);
    }
    virtual size_t used_mem() override { return 0; }

    sk_sp<SkTypeface> get_sk_typeface() {
        assert(this->_sk_typeface);
        return this->_sk_typeface;
    }

    static std::shared_ptr<Font> make(const ResourceStreamInfo &info) {
        return std::make_shared<Font>(info);
    }

    Font(const ResourceStreamInfo &info)
        : Blob(info),
          _sk_typeface(SkTypeface::MakeFromData(
              SkData::MakeWithoutCopy(this->data(), this->size()))) {}

  private:
    sk_sp<SkTypeface> _sk_typeface{};
};

template <> class ResourceProvider<Font> {
  public:
    ~ResourceProvider<Font>() { ::FT_Done_FreeType(this->_ft_lib); }

    static std::shared_ptr<Font> load(const ResourcePathInfo &info) {
        return ResourceProvider<Font>::load(ResourceStreamInfo::make(info));
    }

    static std::shared_ptr<Font> load(const ResourceStreamInfo &info) {
        return Font::make(info);
    }

  private:
    FT_Library _ft_lib;

    ResourceProvider<Font>() {
        auto e = ::FT_Init_FreeType(&this->_ft_lib);
        if (e) {
            throw std::runtime_error(FT_Error_String(e));
        }
    }

    static ResourceProvider<Font> &get() {
        static ResourceProvider<Font> _instance;
        return _instance;
    }

    static unsigned long ft_read(FT_Stream stream, unsigned long offset,
                                 unsigned char *buffer, unsigned long count) {
        auto is = static_cast<std::istream *>(stream->descriptor.pointer);
        return is->readsome(reinterpret_cast<char *>(buffer + offset), count);
    }

    static void ft_close(FT_Stream) {}

    static FT_Face open_face(std::shared_ptr<Font> font, int index) {
        auto is = font->stream();
        FT_StreamDesc desc;
        { desc.pointer = is.get(); }

        auto stream = std::make_shared<FT_StreamRec>();
        {
            stream->base = 0;
            stream->size = font->size();
            stream->descriptor = desc;
            stream->read = &ft_read;
            stream->close = &ft_close;
        }

        FT_Open_Args args;
        {
            args.flags = FT_OPEN_STREAM;
            args.stream = stream.get();
        }
        FT_Face face;

        auto e = ::FT_Open_Face(ResourceProvider<Font>::get()._ft_lib, &args,
                                index, &face);
        if (e) {
            throw std::runtime_error(FT_Error_String(e));
        }

        return face;
    }
};

} // namespace my
