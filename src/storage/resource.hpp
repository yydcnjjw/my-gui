#pragma once

#include <fstream>

#include <boost/noncopyable.hpp>
#include <boost/url/url.hpp>

#include <my_gui.h>
#include <util/uuid.hpp>

namespace my {

namespace fs = std::filesystem;
using uri = boost::urls::url;

struct MemoryInfo {
    
};

class Resource : boost::noncopyable {
  public:
    explicit Resource() {}
    virtual ~Resource() = default;
    const uuid &id() { return this->_uuid; }

    /**
     * @brief      resource memory size
     */
    virtual size_t mem_size() = 0;

  private:
    const uuid _uuid{uuid_gen()};
};

class ResourceProvider {
  public:
    virtual ~ResourceProvider() = default;
    /**
     * @brief      load from fs
     */
    virtual std::shared_ptr<Resource> load(const fs::path &path) = 0;

    /**
     * @brief      load from stream
     */
    virtual std::shared_ptr<Resource> load(std::shared_ptr<std::istream> is,
                                           const uri uri = {}) = 0;
};

std::shared_ptr<std::ifstream> make_ifstream(const fs::path &path) {
    auto ifs = std::make_shared<std::ifstream>();
    ifs->exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ifs->open(path);
    return ifs;
}
std::shared_ptr<std::ofstream> make_ofstream(const fs::path &path) {
    auto ofs = std::make_shared<std::ofstream>();
    ofs->exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ofs->open(path);
    return ofs;
}
    
} // namespace my
