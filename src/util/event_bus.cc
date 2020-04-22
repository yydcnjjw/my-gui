#include <util/event_bus.hpp>
#include <util/logger.h>

namespace my {
std::unique_ptr<EventBus> EventBus::create() {
    return std::make_unique<EventBus>();
}

void EventBus::run() {
    // GLOG_D("main: thread id %li", std::this_thread::get_id());
    GLOG_D("event loop is running ...");
    this->on_event<QuitEvent>()
        .observe_on(this->_ev_bus_worker)
        .subscribe([this](auto) { this->_finish(); });

    for (;;) {
        while (!this->_rlp.empty() &&
               this->_rlp.peek().when < this->_rlp.now()) {
            this->_rlp.dispatch();
        }
        this->post<IdleEvent>();
        while (!this->_rlp.empty() &&
               this->_rlp.peek().when < this->_rlp.now()) {
            this->_rlp.dispatch();
        }
        if (this->_event_suject.get_subscriber().is_subscribed() ||
            !this->_rlp.empty()) {
            std::unique_lock<std::mutex> l_lock(this->_lock);
            this->_cv.wait(l_lock, [this] {
                return !this->_rlp.empty() &&
                       this->_rlp.peek().when < this->_rlp.now();
            });
        } else {
            break;
        }
    }
    GLOG_D("event loop is exited !");
};

} // namespace my
