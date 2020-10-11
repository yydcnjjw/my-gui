#pragma once

#include <fstream>
#include <typeindex>
#include <typeinfo>
#include <shared_mutex>

#include <boost/noncopyable.hpp>

#include <my_gui.hpp>
#include <storage/archive.h>
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

struct ResourceFileProvideInfo {
    fs::path path;
    size_t offset{0};
};
struct ResourceStreamProvideInfo {
    /**
     * only hint
     */
    size_t size{0};
    size_t offset{0};
    std::unique_ptr<std::istream> is{};
    static ResourceStreamProvideInfo make(const ResourceFileProvideInfo &info) {
        return ResourceStreamProvideInfo{fs::file_size(info.path), info.offset,
                                         make_ifstream(info.path)};
    }
};

class ResourceLocator {
  public:
    virtual ~ResourceLocator() = default;
    virtual std::string get_id() = 0;
    virtual bool exist() = 0;
    virtual std::optional<ResourceFileProvideInfo> make_file_provide_info() = 0;
    virtual std::optional<ResourceStreamProvideInfo>
    make_stream_provide_info() = 0;

    void set_offset(size_t offset) { this->offset = offset; }

  protected:
    size_t offset{0};
};

class FSResourceLocator : public ResourceLocator {
  public:
    fs::path path;

    FSResourceLocator(const fs::path &path) : path(path) {}

    static std::shared_ptr<FSResourceLocator> make(const fs::path &path) {
        return std::make_shared<FSResourceLocator>(path);
    }

    std::string get_id() override {
        return (boost::format("file://%1%?offset=%2%") %
                fs::absolute(this->path).string() % this->offset)
            .str();
    }

    bool exist() override { return fs::exists(this->path); }

    std::optional<ResourceFileProvideInfo> make_file_provide_info() override {
        return ResourceFileProvideInfo{this->path, this->offset};
    }
    std::optional<ResourceStreamProvideInfo>
    make_stream_provide_info() override {
        return ResourceStreamProvideInfo::make(
            FSResourceLocator::make_file_provide_info().value());
    }
};

struct ResourceLocatorError : public std::invalid_argument {
    ResourceLocatorError(const std::string &msg) : std::invalid_argument(msg) {}
};

class XP3ResourceLocator : public ResourceLocator {
  public:
    using archive_map = std::map<fs::path, std::shared_ptr<Archive>>;
    using make_archive_map = std::map<std::string, Archive::make_archive_func>;

    fs::path archive_path;
    fs::path query_path;

    XP3ResourceLocator(const fs::path &archive_path, const fs::path &query_path)
        : archive_path(archive_path), query_path(query_path) {}

    static std::shared_ptr<XP3ResourceLocator>
    make(const fs::path &archive_path, const fs::path &query_path) {
        return std::make_shared<XP3ResourceLocator>(archive_path, query_path);
    }

    std::string get_id() override {
        return (boost::format("file://%1%?query=%2%&offset=%3%") %
                fs::absolute(this->archive_path).string() %
                this->query_path.string() % this->offset)
            .str();
    }

    bool exist() override {
        return this->archive_get()->exists(this->query_path);
    }

    std::optional<ResourceFileProvideInfo> make_file_provide_info() override {
        return std::nullopt;
    }

    std::optional<ResourceStreamProvideInfo>
    make_stream_provide_info() override {
        auto archive_file = this->archive_get()->extract(this->query_path);
        return ResourceStreamProvideInfo{archive_file.org_size, this->offset,
                                         std::move(archive_file.is)};
    }

  private:
    static std::shared_mutex _lock;
    static archive_map _archives;
    static const make_archive_map _supported_archives;

    static std::optional<std::shared_ptr<Archive>>
    archive_get_if_exist(const fs::path &path);

    std::shared_ptr<Archive> archive_get() {
        auto archive =
            XP3ResourceLocator::archive_get_if_exist(this->archive_path);
        if (!archive.has_value()) {
            throw ResourceLocatorError(
                (boost::format("archive %1% is not exist") % this->archive_path)
                    .str());
        }
        return archive.value();
    }
};

template <typename res,
          typename = std::enable_if_t<std::is_base_of_v<Resource, res>>>
class ResourceProvider {
    /**
     * @brief      load from fs
     */
    static std::shared_ptr<res> load(const ResourceFileProvideInfo &) {
        throw std::invalid_argument((boost::format("%1% can not be load") %
                                     ResourceProvider<res>::type().name)
                                        .str());
    }

    /**
     * @brief      load from stream
     */
    static std::shared_ptr<res> load(const ResourceStreamProvideInfo &) {
        throw std::invalid_argument((boost::format("%1% can not be load") %
                                     ResourceProvider<res>::type().name)
                                        .str());
    }
};

} // namespace my
