#pragma once

#include <fstream>
#include <typeindex>
#include <typeinfo>

#include <boost/noncopyable.hpp>

#include <my_gui.hpp>
#include <util/uuid.hpp>

namespace my {

class Resource : private boost::noncopyable {
  public:
    explicit Resource() {}
    virtual ~Resource() = default;
    const uuid &id() { return this->_uuid; }

    /**
     * @brief      resource memory size
     */
    virtual size_t used_mem() = 0;

  private:
    const uuid _uuid{uuid_gen()};
};

std::unique_ptr<std::ifstream> make_ifstream(const fs::path &path);

std::unique_ptr<std::ofstream> make_ofstream(const fs::path &path);

struct ResourcePathInfo {
    fs::path path{};

    size_t offset{0};

    static ResourcePathInfo make(const fs::path &path, size_t offset) {
        return {path, offset};
    }
};

struct ResourceStreamInfo {
    my::uri uri{};

    /**
     * only hint
     */
    size_t size{};
    std::unique_ptr<std::istream> is{};
    size_t offset{0};

    static ResourceStreamInfo make(const ResourcePathInfo &info) {
        return ResourceStreamInfo{my::uri(info.path.string()),
                                  fs::file_size(info.path),
                                  make_ifstream(info.path), info.offset};
    }
};

template <typename res,
          typename = std::enable_if_t<std::is_base_of_v<Resource, res>>>
class ResourceProvider {
    /**
     * @brief      load from fs
     */
    static std::shared_ptr<res> load(const ResourcePathInfo &) {
        throw std::invalid_argument((boost::format("%1% can not be load") %
                                     ResourceProvider<res>::type().name)
                                        .str());
    }

    /**
     * @brief      load from stream
     */
    static std::shared_ptr<res> load(const ResourceStreamInfo &) {
        throw std::invalid_argument((boost::format("%1% can not be load") %
                                     ResourceProvider<res>::type().name)
                                        .str());
    }
};

} // namespace my
