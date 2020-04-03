#include <event_bus.hpp>

namespace my {
std::unique_ptr<EventBus> EventBus::create() {
    return std::make_unique<EventBus>();
}
} // namespace my
