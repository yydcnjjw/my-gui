#include "resource_mgr.hpp"

#include <fstream>

namespace my {

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
