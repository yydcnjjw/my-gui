#include "resource.hpp"
#include "storage/resource_mgr.hpp"

namespace my {
std::unique_ptr<std::ifstream> make_ifstream(const fs::path &path) {
    auto ifs = std::make_unique<std::ifstream>();
    ifs->exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ifs->open(path, std::ios::binary);
    return ifs;
}
std::unique_ptr<std::ofstream> make_ofstream(const fs::path &path) {
    auto ofs = std::make_unique<std::ofstream>();
    ofs->exceptions(std::ofstream::failbit | std::ofstream::badbit);
    ofs->open(path);
    return ofs;
}

std::shared_mutex XP3ResourceLocator::_lock{};
XP3ResourceLocator::archive_map XP3ResourceLocator::_archives{};
const XP3ResourceLocator::make_archive_map
    XP3ResourceLocator::_supported_archives{Archive::supported_archives()};

std::optional<std::shared_ptr<Archive>>
XP3ResourceLocator::archive_get_if_exist(const fs::path &path) {
    const auto extension = path.extension();
    std::shared_lock<std::shared_mutex> l_lock(XP3ResourceLocator::_lock);
    auto it = XP3ResourceLocator::_supported_archives.find(extension);
    if (it != XP3ResourceLocator::_supported_archives.end()) {

        auto archive_it = XP3ResourceLocator::_archives.find(path);

        if (archive_it != XP3ResourceLocator::_archives.end()) {
            return archive_it->second;
        }

        if (FSResourceLocator::make(path)->exist()) {
            auto archive = it->second(path);
            {
                l_lock.unlock();
                std::unique_lock<std::shared_mutex> l_w_lock(
                    XP3ResourceLocator::_lock);
                XP3ResourceLocator::_archives.insert({path, archive});
            }

            return archive;
        }
    }
    return std::nullopt;
}

} // namespace my
