#pragma once

#include <typeindex>

#include <core/config.hpp>

namespace my {
class IEvent {
  public:
    typedef std::chrono::system_clock::time_point time_point;
    time_point timestamp;
    IEvent(const std::type_index &type)
        : timestamp(std::chrono::system_clock::now()), _type(type) {}

    virtual ~IEvent() = default;
    template <typename T> bool is() { return this->_type == typeid(T); }

  private:
    std::type_index _type;
};

template <typename DataType> class Event : public IEvent, public DataType {
  public:
    typedef DataType data_type;
    template <typename... Args>
    Event(Args &&... args)
        : IEvent(typeid(data_type)), data_type(std::forward<Args>(args)...) {}

    template <typename... Args> static decltype(auto) make(Args &&... args) {
        return std::make_shared<Event>(std::forward<Args>(args)...);
    }
};

struct Observable {
    using observable_type = rxcpp::observable<std::shared_ptr<IEvent>>;
    virtual ~Observable() = default;
    virtual observable_type event_source() = 0;
};

struct Observer {
    virtual ~Observer() = default;
    virtual void subscribe(Observable *) = 0;
};

struct QuitEvent {
    bool is_on_error{false};
    QuitEvent(bool is_on_error = false) : is_on_error(is_on_error) {}
};

template <typename T> inline decltype(auto) on_event(Observable *o) {
    return o->event_source()
        .filter([](auto e) { return e->template is<T>(); })
        .map([](auto e) { return std::dynamic_pointer_cast<Event<T>>(e); });
}

class EventBus : public Observable, public Observer {
    using observable_type = Observable::observable_type;

  public:
    observable_type event_source() override {
        return this->_event_source.get_observable();
    }

    void subscribe(Observable *observable) override {
        observable->event_source().subscribe([this](auto e) { this->post(e); });
    }

    template <typename T, typename... Args> void post(Args &&... args) {
        this->post(Event<T>::make(std::forward<Args>(args)...));
    }

    static std::unique_ptr<EventBus> create() {
        return std::make_unique<EventBus>();
    }

  protected:
    void post(std::shared_ptr<IEvent> e) {
        this->_event_source.get_subscriber().on_next(e);
    }

  private:
    rxcpp::subjects::subject<std::shared_ptr<IEvent>> _event_source;
};

} // namespace my