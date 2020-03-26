#pragma once

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <rx.hpp>

#include "logger.h"

namespace my {
class IEvent {
  public:
    std::chrono::system_clock::time_point timestamp;
    IEvent(size_t type_id)
        : timestamp(std::chrono::system_clock::now()), _type_id(type_id) {}

    virtual ~IEvent() = default;
    template <typename T> bool is() {
        return this->_type_id == typeid(T).hash_code();
    }

  private:
    size_t _type_id;
};
template <typename T> class Event : public IEvent {
  public:
    Event(std::shared_ptr<T> t) : IEvent(typeid(T).hash_code()), data(t) {}

    template <typename... Args>
    static auto make(std::shared_ptr<T> data) -> decltype(auto) {
        return std::make_shared<Event<T>>(data);
    }

    template <typename... Args>
    static auto make(Args &&... args) -> decltype(auto) {
        return make(std::make_shared<T>(std::forward<Args>(args)...));
    }

    static auto make(T &&t) -> decltype(auto) {
        return make(std::make_shared<T>(std::forward(t)));
    }
    std::shared_ptr<T> data;
};

typedef glm::i32vec2 PixelPos;
struct QuitEvent {};
struct MouseMotionEvent {
    PixelPos pos;
    int32_t xrel;
    int32_t yrel;
    uint32_t state;
};

struct KeyboardEvent {
    uint8_t state;
    bool repeat;
    SDL_Keysym keysym;
};

class EventBus {
  public:
    EventBus() : _ev_bus_worker(rxcpp::observe_on_run_loop(this->_rlp)) {}

    template <typename T> auto on_event() -> decltype(auto) {
        std::unique_lock<std::mutex> local_lock(this->_lock);
        return this->_event_suject.get_observable()
            .filter([](const std::shared_ptr<IEvent> &e) {
                return e->template is<T>();
            })
            .map([](const std::shared_ptr<IEvent> &e) {
                return std::dynamic_pointer_cast<Event<T>>(e);
            })
            .subscribe_on(this->_ev_bus_worker);
    }

    template <typename T, typename... Args> void notify(T &&t) {
        notify(std::make_shared<T>(std::forward(t)));
    }

    template <typename T, typename... Args> void notify(Args &&... args) {
        notify(std::make_shared<T>(std::forward<Args>(args)...));
    }

    template <typename T> void notify(std::shared_ptr<T> data) {
        std::unique_lock<std::mutex> local_lock(this->_lock);
        this->_event_suject.get_subscriber().on_next(Event<T>::make(data));
        this->_cv.notify_all();
    }

    void run() {
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
            if (this->_event_suject.get_subscriber().is_subscribed() ||
                !this->_rlp.empty()) {
                GLOG_D("event loop is blocking ...");
                std::unique_lock<std::mutex> local_lock(this->_lock);
                this->_cv.wait(local_lock, [this] {
                    return !this->_rlp.empty() &&
                           this->_rlp.peek().when < this->_rlp.now();
                });
            } else {
                break;
            }
        }
        GLOG_D("event loop is exited !");
    };

    rxcpp::observe_on_one_worker &ev_bus_worker() {
        return this->_ev_bus_worker;
    }

  private:
    rxcpp::schedulers::run_loop _rlp;
    rxcpp::observe_on_one_worker _ev_bus_worker;

    rxcpp::subjects::subject<std::shared_ptr<IEvent>> _event_suject;

    std::mutex _lock;
    std::condition_variable _cv;

    void _finish() {
        this->_event_suject.get_subscriber().on_completed();
        this->_event_suject.get_subscriber().unsubscribe();
        this->_cv.notify_all();
    }
};

} // namespace my
