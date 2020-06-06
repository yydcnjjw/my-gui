#include "resource_mgr.hpp"

#include <fstream>

namespace my {

uri make_archive_search_uri(const my::fs::path &archive_path,
                            const my::fs::path &query, size_t offset) {
    return my::uri((boost::format("file://%1%?path=%2%&offset=%3%") %
                    fs::absolute(archive_path).string() % query.string() %
                    offset)
                       .str());
}

uri make_path_search_uri(const my::fs::path &file, size_t offset) {
    return my::uri((boost::format("file://%1%?offset=%2%") %
                    fs::absolute(file).string() % offset)
                       .str());
}

// TJS2Script::TJS2Script(const fs::path &path) : Resource(path) {
//     this->_load_from_stream(make_ifstream(path));
// }

// TJS2Script::TJS2Script(const fs::path &path, std::shared_ptr<std::istream>
// is)
//     : Resource(path) {
//     this->_load_from_stream(is);
// }

// void TJS2Script::_load_from_stream(std::shared_ptr<std::istream> is) {

// }

std::unique_ptr<ResourceMgr> ResourceMgr::create(AsyncTask *task) {
    return std::make_unique<ResourceMgr>(task);
}

void ResourceMgr::release(const std::shared_ptr<Resource> &r) {
    std::shared_lock<std::shared_mutex> l_lock(this->_lock);
    auto it = std::find_if(
        this->_resources.begin(), this->_resources.end(),
        [&r](resource_map::value_type v) { return v.second == r; });
    if (it != this->_resources.end()) {
        l_lock.unlock();
        std::unique_lock<std::shared_mutex> l_w_lock(this->_lock);
        this->_resources.erase(it);
    }
}

} // namespace my
