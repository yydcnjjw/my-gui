#pragma once

#include <boost/format.hpp>
#include <boost/gil/extension/io/bmp.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/io/read_image.hpp>

#include <storage/resource.hpp>

namespace my {

class Image : public Resource {
  public:
    Image() {}

    size_t mem_size() override {}
};

class ImageView {
    
};

class ImageProvider : ResourceProvider {
  public:
    std::shared_ptr<Resource> load(const fs::path &path) override {
        return this->load(make_ifstream(path), my::uri(path.string()));
    }
    std::shared_ptr<Resource> load(std::shared_ptr<std::istream> is,
                                   const uri uri = {}) override {
        auto path = fs::path(uri.encoded_path().to_string());

        boost::gil::rgba8_image_t image;
        // XXX: clear failbit
        is->exceptions(std::ifstream::badbit);
        static const std::set<std::string> bmp_extensions{".bmp"};
        static const std::set<std::string> jpeg_extensions{
            ".jpg", ".JPEG", ".jpe", ".jfif", ".jfi", ".jif"};
        static const std::set<std::string> png_extensions{".png"};

        const auto extension = path.extension();
        if (bmp_extensions.find(extension) != bmp_extensions.end()) {
            boost::gil::read_and_convert_image(*is, image,
                                               boost::gil::bmp_tag());
        } else if (jpeg_extensions.find(extension) != jpeg_extensions.end()) {
            boost::gil::read_and_convert_image(*is, image,
                                               boost::gil::jpeg_tag());
        } else if (png_extensions.find(extension) != png_extensions.end()) {
            boost::gil::read_and_convert_image(*is, image,
                                               boost::gil::png_tag());
        } else {
            throw std::runtime_error(
                (boost::format("%1%: image extension %2% is not supported") %
                 uri.encoded_url().to_string() % extension)
                    .str());
        }

        auto view = boost::gil::view(image);
    }
};

} // namespace my
