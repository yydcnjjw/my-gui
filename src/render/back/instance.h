#pragma once

#include <render/device.h>
#include <render/context.h>

namespace my {
namespace render {

class Instance {
  public:
    Instance() = default;
    virtual ~Instance() = default;
    virtual std::shared_ptr<Device> make_device(const Context &) = 0;
};

} // namespace render
} // namespace my
