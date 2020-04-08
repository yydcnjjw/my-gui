#include "resource_mgr.hpp"

#include <fstream>

#include <boost/format.hpp>
#include <boost/gil/extension/io/bmp.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/io/read_image.hpp>
#include <boost/iostreams/copy.hpp>

#include <codecvt.h>

namespace my {
std::shared_ptr<std::ifstream> make_ifstream(const fs::path &path) {
    auto ifs = std::make_shared<std::ifstream>();
    ifs->exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ifs->open(path);
    return ifs;
}
std::shared_ptr<std::ofstream> make_ofstream(const fs::path &path) {
    auto ofs = std::make_shared<std::ofstream>();
    ofs->exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ofs->open(path);
    return ofs;
}
    
Texture::Texture(const fs::path &path) : Resource(path) {
    this->_load_from_stream(path, make_ifstream(path));
}

Texture::Texture(const fs::path &path, std::shared_ptr<std::istream> is)
    : Resource(path) {
    this->_load_from_stream(path, is);
}

void Texture::_load_from_stream(const fs::path &path,
                                std::shared_ptr<std::istream> is) {
    // XXX: clear failbit
    is->exceptions(std::ifstream::badbit);
    static std::set<std::string> bmp_extensions{".bmp"};
    static std::set<std::string> jpeg_extensions{".jpg",  ".JPEG", ".jpe",
                                                 ".jfif", ".jfi",  ".jif"};
    static std::set<std::string> png_extensions{".png"};

    const auto extension = path.extension();
    if (bmp_extensions.find(extension) != bmp_extensions.end()) {
        boost::gil::read_and_convert_image(*is, this->_image, boost::gil::bmp_tag());
    } else if (jpeg_extensions.find(extension) != jpeg_extensions.end()) {
        boost::gil::read_and_convert_image(*is, this->_image, boost::gil::jpeg_tag());
    } else if (png_extensions.find(extension) != png_extensions.end()) {
        boost::gil::read_and_convert_image(*is, this->_image, boost::gil::png_tag());
    } else {
        throw std::runtime_error(
            (boost::format("image extension %1% is not supported") % extension)
                .str());
    }
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
    this->blob = ss.str();
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
            continue;
        }
        break;
    }
}

std::unique_ptr<ResourceMgr> ResourceMgr::create(AsyncTask *task) {
    return std::make_unique<ResourceMgr>(task);
}

void ResourceMgr::release(Resource *r) { this->_resources.erase(r->get_key()); }

} // namespace my
