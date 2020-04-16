#pragma once
#include <vulkan/vulkan.hpp>

#include <render/render_device.h>

namespace my {
namespace render {
namespace vulkan {

template <class T> class Buffer : public render::Buffer<T> {
public:
    static std::shared_ptr<Buffer<T>> make(const T &);
};

} // namespace vulkan
} // namespace render

} // namespace my
