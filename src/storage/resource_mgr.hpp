#pragma once

#include <shared_mutex>

#include <boost/gil.hpp>

#include <my_gui.hpp>
#include <storage/archive.h>
#include <storage/resource.hpp>
#include <util/async_task.hpp>

namespace my {

uri make_archive_search_uri(const my::fs::path &archive_path,
                            const my::fs::path &query, size_t offset = 0);

uri make_path_search_uri(const my::fs::path &file, size_t offset = 0);

class ResourceMgr {
  public:
    ~ResourceMgr() = default;

    bool exist(const uri &uri) {
        fs::path path{uri.encoded_path().to_string()};

        auto cache = this->_search_cache(uri);

        if (cache.has_value()) {
            return true;
        }

        auto archive = this->_exist_and_get_archive(path);

        if (archive.has_value()) {
            const auto query_path = uri.params().at("path");
            return archive.value()->exists(query_path);
        } else {
            return ResourceMgr::exist(path);
        }
    }

    static bool exist(const fs::path path) { return fs::exists(path); }

    template <typename res,
              typename = std::enable_if_t<std::is_base_of_v<Resource, res>>>
    future<std::shared_ptr<res>> load(const uri &uri) {
        return this->_async_task->do_async<std::shared_ptr<res>>(
            [this, &uri](auto promise) {
                auto cache = this->_search_cache(uri);
                if (cache.has_value()) {
                    promise->set_value(
                        std::dynamic_pointer_cast<res>(cache.value()->second));
                    return;
                }

                std::shared_ptr<res> resource{};
                fs::path path{uri.encoded_path().to_string()};

                auto archive = this->_exist_and_get_archive(path);
                GLOG_D("uri: %s", uri.encoded_url().to_string().c_str());

                static auto URI_PARAM_OFFSET = "offset";
                static auto URI_PARAM_PATH = "path";

                size_t offset{0};
                if (uri.params().find(URI_PARAM_OFFSET) != uri.params().end()) {
                    offset = std::stoi(uri.params().at(URI_PARAM_OFFSET));
                }

                if (archive.has_value() &&
                    uri.params().find(URI_PARAM_PATH) != uri.params().end()) {
                    const auto query_path = uri.params().at(URI_PARAM_PATH);
                    auto archive_file = archive.value()->extract(query_path);
                    // from stream read
                    resource = ResourceProvider<res>::load(ResourceStreamInfo{
                        my::uri(query_path), archive_file.org_size,
                        std::move(archive_file.is), offset});
                } else {
                    resource = ResourceProvider<res>::load(
                        ResourcePathInfo{path, offset});
                }

                GLOG_D("load resource %s",
                       uri.encoded_url().to_string().c_str());
                promise->set_value(resource);

                this->_add_cache(uri, resource);
            });
    }

    template <typename res,
              typename = std::enable_if_t<std::is_base_of_v<Resource, res>>>
    future<std::shared_ptr<res>> load(const fs::path &path) {
        return this->_async_task->do_async<std::shared_ptr<res>>(
            [this, &path](auto promise) {
                auto uri = make_path_search_uri(path);
                auto cache = this->_search_cache(uri);
                if (cache.has_value()) {
                    promise->set_value(
                        std::dynamic_pointer_cast<res>(cache.value()->second));
                    return;
                }

                GLOG_D("load resource %s",
                       uri.encoded_url().to_string().c_str());
                auto resource = ResourceProvider<res>::load(ResourcePathInfo{
                    fs::path(uri.encoded_path().to_string()), 0});
                promise->set_value(resource);

                this->_add_cache(uri, resource);
            });
    }

    void release(const std::shared_ptr<Resource> &);

    static std::unique_ptr<ResourceMgr> create(AsyncTask *);
    ResourceMgr(AsyncTask *task) : _async_task(task) {}

  private:
    typedef std::map<std::string, std::shared_ptr<Resource>> resource_map;
    resource_map _resources;

    std::map<fs::path, std::shared_ptr<Archive>> _archives;
    std::map<std::string, Archive::make_archive_func> _supported_archives =
        Archive::supported_archives();

    mutable std::shared_mutex _lock;
    AsyncTask *_async_task;

    void _add_cache(const uri &uri, std::shared_ptr<Resource> resource) {
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);
        this->_resources.insert({uri.encoded_url().to_string(), resource});
    }

    std::optional<resource_map::const_iterator>
    _search_cache(const uri &uri) const {
        std::shared_lock<std::shared_mutex> l_lock(this->_lock);
        const auto it = this->_resources.find(uri.encoded_url().to_string());
        if (it != this->_resources.cend()) {
            return it;
        }
        return std::nullopt;
    }

    std::optional<std::shared_ptr<Archive>>
    _exist_and_get_archive(const fs::path &path) {
        const auto extension = path.extension();
        std::shared_lock<std::shared_mutex> l_lock(this->_lock);
        auto it = this->_supported_archives.find(extension);
        if (it != this->_supported_archives.end()) {

            auto archive_it = this->_archives.find(path);

            if (archive_it != this->_archives.end()) {
                return archive_it->second;
            }

            if (ResourceMgr::exist(path)) {
                auto archive = it->second(path);
                {
                    l_lock.unlock();
                    std::unique_lock<std::shared_mutex> l_w_lock(this->_lock);
                    this->_archives.insert({path, archive});
                }

                return archive;
            }
        }
        return std::nullopt;
    }
};

} // namespace my
