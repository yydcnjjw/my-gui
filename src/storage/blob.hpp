#pragma once

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <my_render.hpp>
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
        if (info.offset != 0) {
            info.is->seekg(info.offset);
        }

        auto size = info.size - info.offset;
        
        this->_blob_data.assign(size, 0);
        
        info.is->read(
            reinterpret_cast<char *>(this->_blob_data.data()), size);
        
        auto read_size = info.is->gcount();
        if (static_cast<std::streamsize>(size) != read_size) {
            throw std::runtime_error(
                (boost::format("%1%: size error read: %2%, info: %3%") %
                 info.uri.encoded_url().to_string() % read_size % info.size)
                    .str());
        }
    }

  private:
    std::vector<uint8_t> _blob_data;
};

template <> class ResourceProvider<Blob> {
  public:
    static std::shared_ptr<Blob> load(const ResourcePathInfo &info) {
        return ResourceProvider<Blob>::load(ResourceStreamInfo::make(info));
    }
    static std::shared_ptr<Blob> load(const ResourceStreamInfo &info) {
        return Blob::make(info);
    }
};

} // namespace my
