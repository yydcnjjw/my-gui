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

struct ResourceStreamInfo {
    my::uri uri{};
    /**
     * only hint
     */
    size_t size{};
    std::unique_ptr<std::istream> is{};

    static ResourceStreamInfo make(const fs::path &path) {
        return ResourceStreamInfo{my::uri(path.string()), fs::file_size(path),
                                  make_ifstream(path)};
    }
};

template <typename res,
          typename = std::enable_if_t<std::is_base_of_v<Resource, res>>>
class ResourceProvider {
    /**
     * @brief      load from fs
     */
    static std::shared_ptr<res> load(const fs::path &path) {
        return ResourceProvider<res>::load(ResourceStreamInfo::make(path));
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
