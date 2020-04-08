#pragma once

#include <map>
#include <my_gui.h>

namespace my {

class Archive {
  public:
    Archive() = default;
    virtual ~Archive() = default;

    virtual bool exists(const fs::path &) = 0;

    virtual std::shared_ptr<std::istream> extract(const fs::path &path) = 0;

    typedef std::function<std::shared_ptr<Archive>(const fs::path &)>
        make_archive_func;
    static const std::map<std::string, make_archive_func> &supported_archives();
};

} // namespace my
