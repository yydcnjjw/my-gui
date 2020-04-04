#pragma once

#include <boost/noncopyable.hpp>
#include <shared_mutex>
#include <unordered_map>

#include "async_task.hpp"
namespace my {

class Resource : boost::noncopyable {
  public:
    Resource(const std::string &path) : _key(path){};
    virtual ~Resource() = default;
    const std::string &get_key() { return this->_key; }

  private:
    std::string _key;
};

class Texture : public Resource {
  public:
    Texture(const std::string &path);
    ~Texture();
    size_t width, height;
    u_char *pixels;
};

// typedef future<Resource *> AsyncResource;

class ResourceMgr {
  public:
    ~ResourceMgr() = default;
    template <typename T>
    future<std::shared_ptr<T>> load_from_path(const std::string &path) {
        return this->_async_task->do_async<std::shared_ptr<T>>(
            [this, &path](auto promise) {
                {
                    std::shared_lock<std::shared_mutex> l_lock(
                        this->_resources_lock);
                    auto it = _resources.find(path);
                    if (it != _resources.end()) {
                        promise->set_value(
                            std::dynamic_pointer_cast<T>(it->second));
                        this->_resources_lock.unlock();
                        return;
                    }
                }

                auto ptr = std::make_shared<T>(path);
                promise->set_value(ptr);

                {
                    std::unique_lock<std::shared_mutex> l_lock(
                        this->_resources_lock);
                    this->_resources.insert({path, ptr});
                }
            });
    }
    void release(Resource *);

    static std::unique_ptr<ResourceMgr> create(AsyncTask *);
    ResourceMgr(AsyncTask *task) : _async_task(task) {}

  private:
    std::unordered_map<std::string, std::shared_ptr<Resource>> _resources;
    mutable std::shared_mutex _resources_lock;
    AsyncTask *_async_task;

    // ResourceMgr(AsyncTask *task) : _async_task(task) {}
};

} // namespace my
