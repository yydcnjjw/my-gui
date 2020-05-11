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
            this->_blob_data.data(), this->_blob_data.size());
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

  protected:
    std::vector<uint8_t> _blob_data;
    Blob(const ResourceStreamInfo &info) {
        this->_blob_data.reserve(info.size);
        this->_blob_data.assign(std::istream_iterator<uint8_t>(*info.is),
                                std::istream_iterator<uint8_t>());
    }

  private:
    static std::shared_ptr<Blob> make(const ResourceStreamInfo &info) {
        return std::make_shared<Blob>(info);
    }
    friend class ResourceProvider<Blob>;
};

template <> class ResourceProvider<Blob> {
  public:
    static std::shared_ptr<Blob> load(const ResourceStreamInfo &info) {
        return Blob::make(info);
    }
};

} // namespace my
