#include "resource.hpp"

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
} // namespace my
