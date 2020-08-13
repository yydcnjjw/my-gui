#pragma once

#include <shared_mutex>

#include <boost/gil.hpp>

#include <my_gui.hpp>
#include <storage/archive.h>
#include <storage/resource.hpp>
#include <util/async_task.hpp>

namespace my {
class ResourceMgr {
  public:
    ~ResourceMgr() = default;

    bool exist(std::shared_ptr<ResourceLocator> locator) {
        return this->exist(locator->get_id());
    }
    bool exist(std::string uri) { return this->_search_cache(uri).has_value(); }

    template <typename res,
              typename = std::enable_if_t<std::is_base_of_v<Resource, res>>>
    future<std::shared_ptr<res>>
    load(std::shared_ptr<ResourceLocator> locator) {
        return this->_async_task->do_async<std::shared_ptr<res>>(
            [this, locator](auto promise) {
                auto uri = locator->get_id();
                std::shared_ptr<res> resource{};
                resource = this->load_from_cache<res>(uri);

                if (resource) {
                    promise->set_value(resource);
                    return;
                }

                if (!resource) {
                    auto file_provider = locator->make_file_provide_info();

                    if (file_provider.has_value()) {
                        resource =
                            ResourceProvider<res>::load(file_provider.value());
                    }
                }

                if (!resource) {
                    auto stream_provider = locator->make_stream_provide_info();
                    if (stream_provider.has_value()) {
                        resource = ResourceProvider<res>::load(
                            stream_provider.value());
                    }
                }

                if (resource) {
                    GLOG_D("load resource %s", uri.c_str());
                    promise->set_value(resource);

                    this->_add_cache(uri, resource);
                } else {
                    throw std::runtime_error(
                        (boost::format("load resource failure: %1%") % uri)
                            .str());
                }
            });
    }

    template <typename res,
              typename = std::enable_if_t<std::is_base_of_v<Resource, res>>>
    std::shared_ptr<res> load_from_cache(const std::string &uri) {
        auto cache = this->_search_cache(uri);
        if (cache.has_value()) {
            return std::dynamic_pointer_cast<res>(cache.value()->second);
        } else {
            return nullptr;
        }
    }

    template <typename res,
              typename = std::enable_if_t<std::is_base_of_v<Resource, res>>>
    future<std::shared_ptr<res>> load(const fs::path &path) {
        return this->load<res>(FSResourceLocator::make(path));
    }

    void release(const std::shared_ptr<Resource> &);

    static std::unique_ptr<ResourceMgr> create(AsyncTask *);
    ResourceMgr(AsyncTask *task) : _async_task(task) {}

  private:
    typedef std::map<std::string, std::shared_ptr<Resource>> resource_map;
    resource_map _resources;

    mutable std::shared_mutex _lock;
    AsyncTask *_async_task;

    void _add_cache(const std::string &id, std::shared_ptr<Resource> resource) {
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);
        this->_resources.insert({id, resource});
    }

    std::optional<resource_map::const_iterator>
    _search_cache(const std::string &id) const {
        std::shared_lock<std::shared_mutex> l_lock(this->_lock);
        const auto it = this->_resources.find(id);
        if (it != this->_resources.cend()) {
            return it;
        }
        return std::nullopt;
    }
};

} // namespace my
