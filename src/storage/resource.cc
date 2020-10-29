#include "resource.hpp"

namespace my {
unique_ptr<std::ifstream> make_ifstream(const fs::path &path) {
    auto ifs = std::make_unique<std::ifstream>();
    ifs->exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ifs->open(path, std::ios::binary);
    return ifs;
}
unique_ptr<std::ofstream> make_ofstream(const fs::path &path) {
    auto ofs = std::make_unique<std::ofstream>();
    ofs->exceptions(std::ofstream::failbit | std::ofstream::badbit);
    ofs->open(path);
    return ofs;
}

const XP3ResourceLocator::make_archive_map
    XP3ResourceLocator::_supported_archives{Archive::supported_archives()};

} // namespace my
