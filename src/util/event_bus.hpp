#pragma once

#include <shared_mutex>
#include <typeindex>
#include <typeinfo>

#include <rx.hpp>

namespace my {
class IEvent {
  public:
    std::chrono::system_clock::time_point timestamp;
    IEvent(const std::type_index &type)
        : timestamp(std::chrono::system_clock::now()), _type(type) {}

    virtual ~IEvent() = default;
    template <typename T> bool is() { return this->_type == typeid(T); }

  private:
    std::type_index _type;
};

template <typename T> class Event : public IEvent {
  public:
    Event(std::shared_ptr<T> t) : IEvent(typeid(T)), data(t) {}

    template <typename... Args>
    static decltype(auto) make(std::shared_ptr<T> data) {
        return std::make_shared<Event<T>>(data);
    }

    template <typename... Args> static decltype(auto) make(Args &&... args) {
        return make(std::make_shared<T>(std::forward<Args>(args)...));
    }

    static decltype(auto) make(T &&t) {
        return make(std::make_shared<T>(std::forward(t)));
    }
    std::shared_ptr<T> data;
};

struct QuitEvent {
    bool is_on_error{false};
    QuitEvent(bool is_on_error = false) : is_on_error(is_on_error) {}
};

class EventBus {
  public:
    EventBus() : _ev_bus_worker(rxcpp::observe_on_run_loop(this->_rlp)) {}
    template <typename T> decltype(auto) on_event() {
        std::unique_lock<std::mutex> l_lock(this->_lock);
        return this->_event_suject.get_observable()
            .filter([](const std::shared_ptr<IEvent> &e) {
                return e->template is<T>();
            })
            .map([](const std::shared_ptr<IEvent> &e) {
                return std::dynamic_pointer_cast<Event<T>>(e);
            });
    }

    template <typename T, typename... Args> void post(Args &&... args) {
        this->post(std::make_shared<T>(std::forward<Args>(args)...));
    }

    template <typename T> void post(std::shared_ptr<T> data) {
        std::unique_lock<std::mutex> l_lock(this->_lock);
        this->_event_suject.get_subscriber().on_next(Event<T>::make(data));
        this->_cv.notify_all();
    }

    void run();

    rxcpp::observe_on_one_worker &ev_bus_worker() {
        return this->_ev_bus_worker;
    }

    static std::unique_ptr<EventBus> create();

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
