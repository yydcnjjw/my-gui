#pragma once

#include <map>

#include <core/core.hpp>

namespace my {

struct ArchiveFile {
    fs::path path;
    size_t arc_size;
    size_t org_size;
    std::unique_ptr<std::istream> is;
};

class Archive {
  public:
    Archive() = default;
    virtual ~Archive() = default;

    virtual bool exists(const fs::path &) = 0;

    virtual ArchiveFile extract(const fs::path &path) = 0;

    virtual std::vector<std::string> list_files() = 0;

    typedef std::function<std::shared_ptr<Archive>(const fs::path &)>
        make_archive_func;
    static const std::map<std::string, make_archive_func> &supported_archives();
};

} // namespace my
