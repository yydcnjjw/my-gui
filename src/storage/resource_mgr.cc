#include "resource_mgr.hpp"

#include <fstream>

#include <boost/format.hpp>



#include <util/codecvt.h>

namespace my {

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
