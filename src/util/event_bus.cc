#include <util/event_bus.hpp>
#include <util/logger.h>

namespace my {
std::unique_ptr<EventBus> EventBus::create() {
    return std::make_unique<EventBus>();
}

void EventBus::run() {
    GLOG_D("event loop is running ...");
    this->on_event<QuitEvent>()
        .observe_on(this->_ev_bus_worker)
        .subscribe([this](auto) { this->_finish(); });

    for (;;) {
        while (!this->_rlp.empty() &&
               this->_rlp.peek().when < this->_rlp.now()) {
            this->_rlp.dispatch();
        }

        if (!this->_event_suject.get_subscriber().is_subscribed()) {
            break;
        }
        {
            std::unique_lock<std::mutex> l_lock(this->_lock);
            this->_cv.wait(l_lock, [this] {
                return !this->_rlp.empty() &&
                       this->_rlp.peek().when < this->_rlp.now();
            });
        }
    }
    GLOG_D("event loop is exited !");
};

} // namespace my
