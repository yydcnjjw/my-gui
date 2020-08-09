#pragma once

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <my_render.hpp>
#include <storage/resource.hpp>

namespace my {

class Blob : public Resource {
  public:
    virtual size_t used_mem() override { return this->size(); }

    size_t size() const { return this->_blob_size; }

    const void *data() const { return this->_blob_data; }

    std::unique_ptr<std::istream> stream() {
        return std::make_unique<
            boost::iostreams::stream<boost::iostreams::array_source>>(
            reinterpret_cast<const char *>(this->data()), this->size());
    }

    sk_sp<SkData> sk_data() const {
        return SkData::MakeWithoutCopy(this->data(), this->size());
    }

    /**
     * @brief      only read blob from string view
     *
     */
    std::string_view string_view() {
        return std::string_view(reinterpret_cast<const char *>(this->data()),
                                this->size());
    }

    static std::shared_ptr<Blob> make(const ResourceStreamInfo &info) {
        return std::shared_ptr<Blob>{new Blob(info)};
    }

    static std::shared_ptr<Blob> make(const void *data, size_t size) {
        return std::shared_ptr<Blob>{new Blob(data, size)};
    }

  protected:
    Blob(const void *data, size_t size) : _blob_data(data), _blob_size(size) {}

    Blob(const ResourceStreamInfo &info) {
        if (info.offset != 0) {
            info.is->seekg(info.offset);
        }

        auto size = info.size - info.offset;

        auto data = new uint8_t[size];

        info.is->read(reinterpret_cast<char *>(data), size);

        auto read_size = info.is->gcount();
        if (static_cast<std::streamsize>(size) != read_size) {
            throw std::runtime_error(
                (boost::format("%1%: size error read: %2%, info: %3%") %
                 info.uri.encoded_url().to_string() % read_size % info.size)
                    .str());
        }

        this->_blob_data = data;
        this->_blob_size = size;
    }

  private:
    const void *_blob_data;
    size_t _blob_size;
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
