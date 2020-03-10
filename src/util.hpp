#pragma once

#include <atomic>
#include <cassert>
#include <fstream>
#include <limits>
#include <vector>

namespace ID {
typedef uint64_t UUID;
static UUID get() {
    static std::atomic<UUID> id;
    assert(id < std::numeric_limits<UUID>::max());
    return id++;
}
}; // namespace ID
namespace Util {
static std::vector<char> read_file(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

} // namespace Util
