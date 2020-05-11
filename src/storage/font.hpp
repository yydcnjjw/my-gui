#pragma once

#include <my_render.h>
#include <storage/resource.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <skia/include/core/SkFontMgr.h>

namespace my {

class Font : public Resource {
  public:
    ~Font() {
        ::FT_Done_Face(this->_face);
    }
    virtual size_t used_mem() override { return 0; }

    static std::shared_ptr<Font> make(FT_Face face) {
        return std::make_shared<Font>(face);
    }

  private:
    FT_Face _face;
    Font(FT_Face face) : _face(face) {
        SkFontMgr::RefDefault()->makeFromData(nullptr);
    }
};

template <> class ResourceProvider<Font> {
  public:
    static std::shared_ptr<Font> load(const fs::path &path) {
        return ResourceProvider<Font>::get()._load(path);
    }

    static std::shared_ptr<Font> load(const ResourceStreamInfo &info) {
        return ResourceProvider<Font>::get()._load(info);
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

    std::shared_ptr<Font> _load(const fs::path &path) {
        FT_Face face;
        auto e = ::FT_New_Face(this->_ft_lib, path.c_str(), 0, &face);
        if (e) {
            throw std::runtime_error(FT_Error_String(e));
        }
        return Font::make(face);
    }

    std::shared_ptr<Font> _load(const ResourceStreamInfo &info) {
        FT_StreamDesc desc;
        { desc.pointer = info.is.get(); }

        auto stream = std::make_shared<FT_StreamRec>();
        {
            stream->base = 0;
            stream->size = info.size;
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
        auto e = ::FT_Open_Face(this->_ft_lib, &args, 0, &face);
        if (e) {
            throw std::runtime_error(FT_Error_String(e));
        }

        return Font::make(face);
    }
};

} // namespace my
