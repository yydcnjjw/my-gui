#pragma once

#include <shared_mutex>
#include <unordered_map>

#define BOOST_URL_HEADER_ONLY
#include <boost/gil.hpp>
#include <boost/noncopyable.hpp>
#include <boost/url/url.hpp>

#include <my_gui.h>
#include <util/async_task.hpp>
#include <storage/archive.h>

namespace my {

class Resource : boost::noncopyable {
  public:
    explicit Resource(const fs::path &path) : _key(path) {}
    virtual ~Resource() = default;
    const std::string &get_key() { return this->_key; }

  private:
    std::string _key;
};

class Blob : public Resource {
  public:
    explicit Blob(const fs::path &path);
    Blob(const fs::path &path, std::shared_ptr<std::istream> is);

    u_char *data() {
        return reinterpret_cast<u_char*>(this->_blob.data());
    }
    size_t size() {
        return this->_blob.size();
    }

  private:
    void _load_from_stream(std::shared_ptr<std::istream> is);
    std::string _blob;
};

class Image : public Resource {
  public:
    explicit Image(const fs::path &path);
    Image(const fs::path &path, std::shared_ptr<std::istream> is);
    Image(const uint8_t *data, int w, int h);
    
    [[nodiscard]] size_t width() const { return this->_w; }
    [[nodiscard]] size_t height() const { return this->_h; }
    const uint8_t *raw_data() const {
        return this->_data;
    }

  private:
    void _load_from_stream(const fs::path &path,
                           std::shared_ptr<std::istream> is);
    
    boost::gil::rgba8_image_t _image;
    const uint8_t *_data;
    int _w;
    int _h;
};

class TJS2Script : public Resource {
  public:
    explicit TJS2Script(const fs::path &path);
    TJS2Script(const fs::path &path, std::shared_ptr<std::istream> is);
    std::string script;

  private:
    void _load_from_stream(std::shared_ptr<std::istream> is);
};

typedef boost::urls::url uri;
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

    template <typename T>
    future<std::shared_ptr<T>> load_from_uri(const uri &uri) {
        return this->_async_task->do_async<
            std::shared_ptr<T>>([this, &uri](auto promise) {
            {
                std::shared_lock<std::shared_mutex> l_lock(this->_lock);
                auto cache = this->_search_cache(uri.encoded_url().to_string());

                if (cache.has_value()) {
                    GLOG_D("load resource %s",
                           uri.encoded_url().to_string().c_str());
                    promise->set_value(
                        std::dynamic_pointer_cast<T>(cache.value()->second));
                    return;
                }
            }

            std::shared_ptr<T> ptr;

            {
                fs::path path{uri.encoded_path().to_string()};
                auto archive = this->_exist_and_get_archive(path);

                if (archive.has_value()) {
                    const auto query_path = uri.params().at("path");
                    path = uri.encoded_url().to_string();
                    // from stream read
                    ptr = std::make_shared<T>(
                        path, archive.value()->extract(query_path));
                } else {
                    ptr = std::make_shared<T>(path);
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

    template <typename T>
    future<std::shared_ptr<T>> load_from_path(const std::string &path) {
        return this->_async_task->do_async<std::shared_ptr<T>>(
            [this, &path](auto promise) {
                auto full_path = fs::absolute(path);
                {
                    std::shared_lock<std::shared_mutex> l_lock(this->_lock);
                    auto cache = this->_search_cache(full_path);

                    if (cache.has_value()) {
                        promise->set_value(std::dynamic_pointer_cast<T>(
                            cache.value()->second));
                        return;
                    }
                }
                GLOG_D("load resource %s", full_path.c_str());
                auto ptr = std::make_shared<T>(full_path);
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
            }
        }
        return std::nullopt;
    }
};

} // namespace my
