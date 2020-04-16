#include <cstddef>

namespace my {

namespace render {
enum class BuferType { VERTEX, INDEX, UNIFORM };

class Buffer {
  public:
    Buffer(const BuferType &type, const size_t &size)
        : type(type), size(size) {}
    ~Buffer() = default;

    virtual void copy_to_gpu() = 0;
    BuferType type;
    size_t size;
};
} // namespace render
} // namespace my
