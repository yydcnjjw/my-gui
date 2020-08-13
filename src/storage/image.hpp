#pragma once

#include <boost/format.hpp>
#include <boost/gil/extension/io/bmp.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/io/read_image.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

#include <my_render.hpp>
#include <storage/blob.hpp>
#include <storage/resource.hpp>

#include <OpenImageIO/filesystem.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>

namespace my {

class Image : public Resource {
    using image_type = boost::gil::rgba8_image_t;
    using image_view_type = boost::gil::rgba8_image_t::view_t;

  public:
    constexpr static size_t pixel_size{sizeof(image_type::value_type)};

    size_t used_mem() override { return this->byte_size(); }

    size_t row_bytes() { return this->_spec.scanline_bytes(); }

    size_t width() const { return this->_spec.width; }

    size_t height() const { return this->_spec.height; }

    ISize2D size() const {
        return ISize2D::Make(this->width(), this->height());
    }

    const void *data() { return this->_image_buf.localpixels(); }

    size_t byte_size() { return this->_spec.image_bytes(); }

    sk_sp<SkImage> sk_image() {
        return SkImage::MakeRasterData(
            SkImageInfo::MakeN32Premul(this->width(), this->height()),
            SkData::MakeWithCopy(this->data(), this->byte_size()),
            this->row_bytes());
    }

    static std::shared_ptr<Image> make(const std::shared_ptr<Blob> &blob) {
        return std::make_shared<Image>(blob);
    }

    void export_bmp24(const fs::path &path) { this->_export(path, ".bmp"); }

    void export_png(const fs::path &path) { this->_export(path, ".png"); }

    void _export(const fs::path &path, const std::string &ext) {
        if (!this->_image_buf.write(path.string(), ext)) {
            throw std::runtime_error("export failure");
        }
    }

    Image(const std::shared_ptr<Blob> &blob) {
        OIIO::Filesystem::IOMemReader mr(const_cast<void *>(blob->data()),
                                         blob->size());
        auto in = OIIO::ImageInput::open(".exr", nullptr, &mr);
        if (!in) {
            char buf[] = "my_gui_XXXXXX";
            if (mkstemp(buf) == -1) {
                throw std::runtime_error(std::strerror(errno));
            }

            fs::path path{buf};

            boost::iostreams::copy(blob->stream(), *make_ofstream(path));

            in = OIIO::ImageInput::open(path);
            fs::remove(path);
        }

        if (!in) {
            throw std::runtime_error(
                (boost::format("oiio image load failure: %1%") %
                 OIIO::geterror())
                    .str());
        }

        auto tmp_spec = in->spec();
        std::vector<uint8_t> tmp(tmp_spec.image_bytes(), 0);
        in->read_image(OIIO::TypeDesc::UINT8, tmp.data());
        in->close();

        OIIO::ImageBuf tmp_buf(tmp_spec, tmp.data());

        this->_image_buf = OIIO::ImageBufAlgo::channels(
            tmp_buf, 4,
            {tmp_spec.channelindex("R"), tmp_spec.channelindex("G"),
             tmp_spec.channelindex("B"), tmp_spec.channelindex("A")},
            {0, 0, 0, 255});
        if (!this->_image_buf.initialized()) {
            throw std::runtime_error("oiio convert image failure");
        }
        this->_spec = this->_image_buf.spec();
    }

  private:
    OIIO::ImageBuf _image_buf;
    OIIO::ImageSpec _spec;
};

class ImageView {};

template <> class ResourceProvider<Image> {
  public:
    static std::shared_ptr<Image> load(const ResourceFileProvideInfo & info) {
        return ResourceProvider<Image>::load(ResourceStreamProvideInfo::make(info));
    }
    static std::shared_ptr<Image> load(const ResourceStreamProvideInfo &info) {
        return Image::make(Blob::make(info));
    }
};

} // namespace my
