#pragma once

#include <core/basic_service.hpp>
#include <storage/exception.hpp>
#include <storage/resource.hpp>

namespace my {

class ResourceService : public BasicService {
  public:
    future<bool> exist(shared_ptr<ResourceLocator> locator) {
        return this->exist(locator->get_id());
    }

    future<bool> exist(const std::string &uri) {
        return this->schedule<bool>(
            [this, uri]() { return this->search_cache(uri).has_value(); });
    }

    template <typename res,
              typename = std::enable_if_t<std::is_base_of_v<Resource, res>>>
    future<shared_ptr<res>> load(shared_ptr<ResourceLocator> locator) {
        return this->schedule<shared_ptr<res>>([this, locator]() {
            auto resource = this->load_resource<res>(locator);
            this->add_cache(locator->get_id(), resource);
            return resource;
        });
    }

    template <typename res,
              typename = std::enable_if_t<std::is_base_of_v<Resource, res>>>
    future<shared_ptr<res>> load(const fs::path &path) {
        return this->load<res>(FSResourceLocator::make(path));
    }

    future<void> release(const shared_ptr<Resource> &r) {
        return this->schedule<void>([this, r]() {
            auto it = std::find_if(
                this->_resources.begin(), this->_resources.end(),
                [&r](resource_map::value_type v) { return v.second == r; });
            if (it != this->_resources.end()) {
                this->_resources.erase(it);
            }
        });
    }

    static unique_ptr<ResourceService> create() {
        return std::make_unique<ResourceService>();
    }

  private:
    typedef std::map<std::string, shared_ptr<Resource>> resource_map;
    resource_map _resources;

    template <typename res,
              typename = std::enable_if_t<std::is_base_of_v<Resource, res>>>
    shared_ptr<res> load_from_cache(const std::string &uri) {
        auto cache = this->search_cache(uri);
        if (cache.has_value()) {
            return std::dynamic_pointer_cast<res>(cache.value()->second);
        } else {
            return nullptr;
        }
    }

    void add_cache(const std::string &id, shared_ptr<Resource> resource) {
        this->_resources.insert({id, resource});
    }

    std::optional<resource_map::const_iterator>
    search_cache(const std::string &id) const {
        const auto it = this->_resources.find(id);
        if (it != this->_resources.cend()) {
            return it;
        }
        return std::nullopt;
    }

    template <typename res,
              typename = std::enable_if_t<std::is_base_of_v<Resource, res>>>
    shared_ptr<res> load_resource(shared_ptr<ResourceLocator> locator) {
        auto uri = locator->get_id();
        shared_ptr<res> resource{};
        {
            resource = this->load_from_cache<res>(uri);

            if (resource) {
                return resource;
            }
        }

        {
            auto file_provider = locator->make_file_provide_info();

            if (file_provider.has_value()) {
                resource = ResourceProvider<res>::load(file_provider.value());
            }

            if (resource) {
                return resource;
            }
        }

        {
            auto stream_provider = locator->make_stream_provide_info();
            if (stream_provider.has_value()) {
                resource = ResourceProvider<res>::load(stream_provider.value());
            }

            if (resource) {
                return resource;
            }
        }
        throw ResourceServiceError(
            (boost::format("load resource failure: %1%") % uri).str());
    }
};

} // namespace my
