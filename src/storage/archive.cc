#include "archive.hpp"

namespace my {
extern std::shared_ptr<Archive> open_xp3(const fs::path &path);

const std::map<std::string, Archive::make_archive_func> &
Archive::supported_archives() {
    static std::map<std::string, Archive::make_archive_func> archives{
        {".xp3", open_xp3}};
    return archives;
}
} // namespace my
