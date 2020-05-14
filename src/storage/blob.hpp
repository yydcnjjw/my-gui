#pragma once

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <my_render.h>
#include <storage/resource.hpp>

namespace my {

class Blob : public Resource {
  public:
    virtual size_t used_mem() override { return this->_blob_data.size(); }

    std::unique_ptr<std::istream> stream() {
        return std::make_unique<
            boost::iostreams::stream<boost::iostreams::array_source>>(
            reinterpret_cast<const char *>(this->_blob_data.data()),
            this->_blob_data.size());
    }

    /**
     * @brief      copy blob to string
     *
     */
    std::string string() {
        return std::string(this->_blob_data.begin(), this->_blob_data.end());
    }

    /**
     * @brief      only read blob from string view
     *
     */
    std::string_view string_view() {
        return std::string_view(
            reinterpret_cast<char *>(this->_blob_data.data()),
            this->_blob_data.size());
    }

    size_t size() { return this->_blob_data.size(); }

    const uint8_t *data() { return this->_blob_data.data(); }

    static std::shared_ptr<Blob> make(const ResourceStreamInfo &info) {
        return std::shared_ptr<Blob>{new Blob(info)};
    }

  protected:
    Blob(const ResourceStreamInfo &info) {
        this->_blob_data.assign(info.size, 0);
        auto read_size = info.is->readsome(
            reinterpret_cast<char *>(this->_blob_data.data()), info.size);
        assert(static_cast<std::streamsize>(info.size) == read_size);
    }

  private:
    std::vector<uint8_t> _blob_data;
};

template <> class ResourceProvider<Blob> {
  public:
    static std::shared_ptr<Blob> load(const fs::path &path) {
        return ResourceProvider<Blob>::load(ResourceStreamInfo::make(path));
    }
    static std::shared_ptr<Blob> load(const ResourceStreamInfo &info) {
        return Blob::make(info);
    }
};

} // namespace my
