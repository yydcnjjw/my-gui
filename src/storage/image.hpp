#pragma once

#include <boost/format.hpp>
#include <boost/gil/extension/io/bmp.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/io/read_image.hpp>

#include <my_render.hpp>
#include <storage/resource.hpp>

namespace my {

class Image : public Resource {
    using image_type = boost::gil::rgba8_image_t;

  public:
    constexpr static size_t pixel_size{sizeof(image_type::value_type)};

    size_t used_mem() override {
        return this->width() * this->height() * Image::pixel_size;
    }

    size_t row_bytes() { return this->width() * Image::pixel_size; }

    size_t width() { return this->_image->width(); }

    size_t height() { return this->_image->height(); }

    static std::shared_ptr<Image> make(std::shared_ptr<image_type> gil_image) {
        return std::make_shared<Image>(gil_image);
    }

    Image(std::shared_ptr<image_type> gil_image) : _image(gil_image) {}

  private:
    std::shared_ptr<image_type> _image;
};

class ImageView {};

template <> class ResourceProvider<Image> {
  public:
    static std::shared_ptr<Image> load(const fs::path &path) {
        return ResourceProvider<Image>::load(ResourceStreamInfo::make(path));
    }
    static std::shared_ptr<Image> load(ResourceStreamInfo info) {
        auto &is = info.is;
        auto uri = info.uri;

        auto path = fs::path(uri.encoded_path().to_string());

        std::shared_ptr<boost::gil::rgba8_image_t> image;
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
