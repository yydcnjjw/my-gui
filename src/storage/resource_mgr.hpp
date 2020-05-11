#pragma once

#include <shared_mutex>
#include <unordered_map>

#define BOOST_URL_HEADER_ONLY
#include <boost/gil.hpp>
#include <boost/noncopyable.hpp>
#include <boost/url/url.hpp>

#include <my_gui.h>
#include <resource.h>
#include <storage/archive.h>
#include <util/async_task.hpp>

namespace my {

class ResourceMgr {
  public:
    ~ResourceMgr() = default;

    bool exist(const uri &uri) {
        fs::path path{uri.encoded_path().to_string()};

        auto cache = this->_search_cache(uri.encoded_url().to_string());
        if (cache.has_value()) {
            return true;
        }

        auto archive = this->_exist_and_get_archive(path);
        if (archive.has_value()) {
            const auto query_path = uri.params().at("path");
            return archive.value()->exists(query_path);
        } else {
            return this->exist(path);
        }
    }

    static bool exist(const fs::path path) { return fs::exists(path); }

    template <typename res,
              typename = std::enable_if_t<std::is_base_of_v<Resource, res>>>
    future<std::shared_ptr<res>> load(const uri &uri) {
        return this->_async_task->do_async<
            std::shared_ptr<res>>([this, &uri](auto promise) {
            {
                std::shared_lock<std::shared_mutex> l_lock(this->_lock);
                auto cache = this->_search_cache(uri.encoded_url().to_string());

                if (cache.has_value()) {
                    GLOG_D("load resource %s",
                           uri.encoded_url().to_string().c_str());
                    promise->set_value(
                        std::dynamic_pointer_cast<res>(cache.value()->second));
                    return;
                }
            }

            std::shared_ptr<res> ptr;

            {
                fs::path path{uri.encoded_path().to_string()};
                auto archive = this->_exist_and_get_archive(path);

                if (archive.has_value()) {
                    // TODO: "path" to var
                    const auto query_path = uri.params().at("path");
                    path = uri.encoded_url().to_string();
                    auto archive_file = archive.value()->extract(query_path);
                    // from stream read
                    ptr = ResourceProvice<res>::load(ResourceStreamInfo{
                        path, archive_file.org_size, archive_file.is});
                } else {
                    ptr = ResourceProvice<res>::load(path);
                }
            }

            GLOG_D("load resource %s", uri.encoded_url().to_string().c_str());
            promise->set_value(ptr);

            {
                std::unique_lock<std::shared_mutex> l_lock(this->_lock);
                this->_resources.insert({uri.encoded_url().to_string(), ptr});
            }
        });
    }

    template <typename res,
              typename = std::enable_if_t<std::is_base_of_v<Resource, res>>>
    future<std::shared_ptr<res>> load(const std::string &path) {
        return this->_async_task->do_async<std::shared_ptr<res>>(
            [this, &path](auto promise) {
                auto full_path = fs::absolute(path);
                {
                    std::shared_lock<std::shared_mutex> l_lock(this->_lock);
                    auto cache = this->_search_cache(full_path);

                    if (cache.has_value()) {
                        promise->set_value(std::dynamic_pointer_cast<res>(
                            cache.value()->second));
                        return;
                    }
                }
                GLOG_D("load resource %s", full_path.c_str());
                auto ptr = ResourceProvice<res>::load(full_path);
                promise->set_value(ptr);

                {
                    std::unique_lock<std::shared_mutex> l_lock(this->_lock);
                    this->_resources.insert({full_path, ptr});
                }
            });
    }

    void release(Resource *);

    static std::unique_ptr<ResourceMgr> create(AsyncTask *);
    ResourceMgr(AsyncTask *task) : _async_task(task) {}

  private:
    typedef std::unordered_map<std::string, std::shared_ptr<Resource>>
        resource_map;
    resource_map _resources;
    std::map<fs::path, std::shared_ptr<Archive>> _archives;
    std::map<std::string, Archive::make_archive_func> _supported_archives =
        Archive::supported_archives();
    mutable std::shared_mutex _lock;
    AsyncTask *_async_task;

    std::optional<resource_map ::iterator> _search_cache(const std::string &k) {
        std::shared_lock<std::shared_mutex> l_lock(this->_lock);
        auto it = this->_resources.find(k);
        if (it != this->_resources.end()) {
            return it;
        }
        return std::nullopt;
    }

    std::optional<std::shared_ptr<Archive>>
    _exist_and_get_archive(const fs::path path) {
        const auto extension = path.extension();

        std::shared_lock<std::shared_mutex> l_lock(this->_lock);
        auto it = this->_supported_archives.find(extension);
        if (it != this->_supported_archives.end()) {

            auto archive_it = this->_archives.find(path);

            if (archive_it != this->_archives.end()) {
                return archive_it->second;
            }

            l_lock.unlock();

            if (this->exist(path)) {
                auto archive = it->second(path);
                std::unique_lock<std::shared_mutex> l_w_lock(this->_lock);
                this->_archives.insert({path, archive});
                return archive;
            }
        }
        return std::nullopt;
    }
};

} // namespace my
