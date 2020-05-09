#include "resource_mgr.hpp"

#include <fstream>

#include <boost/format.hpp>

#include <boost/iostreams/copy.hpp>

#include <util/codecvt.h>

namespace my {

Image::Image(const fs::path &path) : Resource(path) {
    this->_load_from_stream(path, make_ifstream(path));
}

Image::Image(const fs::path &path, std::shared_ptr<std::istream> is)
    : Resource(path) {
    this->_load_from_stream(path, is);
}

Image::Image(const uint8_t *data, int w, int h)
    : Resource(""), _data(data), _w(w), _h(h) {}

void Image::_load_from_stream(const fs::path &path,
                              std::shared_ptr<std::istream> is) {
    // XXX: clear failbit
    is->exceptions(std::ifstream::badbit);
    static std::set<std::string> bmp_extensions{".bmp"};
    static std::set<std::string> jpeg_extensions{".jpg",  ".JPEG", ".jpe",
                                                 ".jfif", ".jfi",  ".jif"};
    static std::set<std::string> png_extensions{".png"};

    const auto extension = path.extension();
    if (bmp_extensions.find(extension) != bmp_extensions.end()) {
        boost::gil::read_and_convert_image(*is, this->_image,
                                           boost::gil::bmp_tag());
    } else if (jpeg_extensions.find(extension) != jpeg_extensions.end()) {
        boost::gil::read_and_convert_image(*is, this->_image,
                                           boost::gil::jpeg_tag());
    } else if (png_extensions.find(extension) != png_extensions.end()) {
        boost::gil::read_and_convert_image(*is, this->_image,
                                           boost::gil::png_tag());
    } else {
        throw std::runtime_error(
            (boost::format("image extension %1% is not supported") % extension)
                .str());
    }
    this->_w = this->_image.width();
    this->_h = this->_image.width();
    this->_data = boost::gil::interleaved_view_get_raw_data(
        boost::gil::view(this->_image));
}

Blob::Blob(const fs::path &path) : Resource(path) {
    this->_load_from_stream(make_ifstream(path));
}
Blob::Blob(const fs::path &path, std::shared_ptr<std::istream> is)
    : Resource(path) {
    this->_load_from_stream(is);
}

void Blob::_load_from_stream(std::shared_ptr<std::istream> is) {
    std::stringstream ss;
    boost::iostreams::copy(*is, ss);
    this->_blob = ss.str();
}

TJS2Script::TJS2Script(const fs::path &path) : Resource(path) {
    this->_load_from_stream(make_ifstream(path));
}

TJS2Script::TJS2Script(const fs::path &path, std::shared_ptr<std::istream> is)
    : Resource(path) {
    this->_load_from_stream(is);
}

void TJS2Script::_load_from_stream(std::shared_ptr<std::istream> is) {
    static std::vector<std::string> encodes{"UTF-8", "SHIFT_JIS", "GB18030"};

    std::stringstream ss;
    boost::iostreams::copy(*is, ss);
    auto code = ss.str();

    for (const auto &encode : encodes) {
        try {
            this->script =
                codecvt::to_utf<char>(code, encode, codecvt::method_type::stop);
        } catch (codecvt::conversion_error &) {
            this->script.clear();
            continue;
        }
        return;
    }
    throw std::runtime_error("tjs2 script codecvt failure");
}

std::unique_ptr<ResourceMgr> ResourceMgr::create(AsyncTask *task) {
    return std::make_unique<ResourceMgr>(task);
}

void ResourceMgr::release(Resource *r) { this->_resources.erase(r->get_key()); }

} // namespace my
