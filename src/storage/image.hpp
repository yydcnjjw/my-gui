#pragma once

#include <boost/format.hpp>
#include <boost/gil/extension/io/bmp.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/io/read_image.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

#include <my_render.hpp>
#include <storage/resource.hpp>

namespace my {

class Image : public Resource {
    using image_type = boost::gil::rgba8_image_t;

  public:
    constexpr static size_t pixel_size{sizeof(image_type::value_type)};

    size_t used_mem() override { return this->byte_size(); }

    size_t row_bytes() { return this->width() * Image::pixel_size; }

    size_t width() { return this->_image->width(); }

    size_t height() { return this->_image->height(); }

    ISize2D size() { return ISize2D::Make(this->width(), this->height()); }

    const void *data() {
        return boost::gil::interleaved_view_get_raw_data(
            boost::gil::view(*this->_image));
    }

    size_t byte_size() {
        return this->width() * this->height() * Image::pixel_size;
    }

    sk_sp<SkImage> sk_image() { return this->_sk_image; }

    static std::shared_ptr<Image> make(std::shared_ptr<image_type> gil_image) {
        return std::make_shared<Image>(gil_image);
    }

    static std::shared_ptr<Image> make(sk_sp<SkImage> sk_image) {
        return std::make_shared<Image>(sk_image);
    }

    static std::shared_ptr<Image> make(const uint8_t *data,
                                       const my::ISize2D &size) {
        return std::make_shared<Image>(data, size);
    }

    void export_bmp24(const fs::path &path) {
        boost::gil::write_view(
            path.string(),
            boost::gil::color_converted_view<boost::gil::rgb8_pixel_t>(
                boost::gil::view(*this->_image)),
            boost::gil::bmp_tag());
    }

    Image(std::shared_ptr<image_type> gil_image)
        : _image(gil_image),
          _sk_image(SkImage::MakeRasterData(
              SkImageInfo::MakeN32Premul(this->width(), this->height()),
              SkData::MakeWithoutCopy(this->data(), this->byte_size()),
              this->row_bytes())) {}

    Image(sk_sp<SkImage> sk_image)
        : _image(std::make_shared<image_type>()), _sk_image(sk_image) {
        auto data = this->_sk_image->encodeToData();
        boost::iostreams::stream<boost::iostreams::array_source> in(
            reinterpret_cast<const char *>(data->bytes()), data->size());
        boost::gil::read_and_convert_image(in, *this->_image,
                                           boost::gil::png_tag());
    }

    Image(const uint8_t *data, const my::ISize2D &size)
        : Image(SkImage::MakeRasterData(
              SkImageInfo::MakeN32Premul(size.width(), size.height()),
              SkData::MakeWithoutCopy(data, size.width() * size.height() *
                                                Image::pixel_size),
              size.width() * Image::pixel_size)) {}

  private:
    std::shared_ptr<image_type> _image{};
    sk_sp<SkImage> _sk_image{};
};

class ImageView {};

template <> class ResourceProvider<Image> {
  public:
    static std::shared_ptr<Image> load(const ResourcePathInfo &info) {
        return ResourceProvider<Image>::load(ResourceStreamInfo::make(info));
    }
    static std::shared_ptr<Image> load(ResourceStreamInfo info) {
        auto &is = info.is;
        auto &uri = info.uri;

        auto path = fs::path(uri.encoded_path().to_string());

        auto image = std::make_shared<boost::gil::rgba8_image_t>();
        // XXX: clear failbit
        is->exceptions(std::ifstream::badbit);
        static const std::set<std::string> bmp_extensions{".bmp"};
        static const std::set<std::string> jpeg_extensions{
            ".jpg", ".JPEG", ".jpe", ".jfif", ".jfi", ".jif"};
        static const std::set<std::string> png_extensions{".png"};

        const auto extension = path.extension();
        if (bmp_extensions.find(extension) != bmp_extensions.end()) {
            boost::gil::read_and_convert_image(*is, *image,
                                               boost::gil::bmp_tag());
        } else if (jpeg_extensions.find(extension) != jpeg_extensions.end()) {
            boost::gil::read_and_convert_image(*is, *image,
                                               boost::gil::jpeg_tag());
        } else if (png_extensions.find(extension) != png_extensions.end()) {
            boost::gil::read_and_convert_image(*is, *image,
                                               boost::gil::png_tag());
        } else {
            throw std::runtime_error(
                (boost::format("%1%: image extension %2% is not supported") %
                 uri.encoded_url().to_string() % extension)
                    .str());
        }

        return Image::make(image);
    }
};

} // namespace my
