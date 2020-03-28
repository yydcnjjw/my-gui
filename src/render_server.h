#pragma once

#include <memory>

#include "event_bus.hpp"
#include "window_mgr.h"

namespace my {
class RenderServer {
  public:
    RenderServer() = default;
    virtual ~RenderServer() = default;

    virtual void run() = 0;
    static std::unique_ptr<RenderServer> create(EventBus *, Window *);
};
} // namespace my
