#pragma once

#include <typeindex>

#include <core/config.hpp>
#include <core/type.hpp>

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

using shared_event_type = shared_ptr<IEvent>;

template <typename DataType>
class Event : public IEvent,
              public std::enable_shared_from_this<Event<DataType>> {
  public:
    typedef DataType data_type;
    template <typename... Args>
    Event(Args &&... args)
        : IEvent(typeid(data_type)),
          _data(std::make_shared<data_type>(std::forward<Args>(args)...)) {}

    Event(shared_ptr<data_type> data)
        : IEvent(typeid(data_type)), _data(data) {}

    template <typename... Args> static decltype(auto) make(Args &&... args) {
        return std::make_shared<Event>(std::forward<Args>(args)...);
    }

    template <typename T,
              typename = std::enable_if_t<std::is_base_of_v<T, data_type>>>
    decltype(auto) cast_to() {
        return Event<T>::make(std::reinterpret_pointer_cast<T>(this->data()));
    }

    shared_event_type as_dynamic() {
        return std::reinterpret_pointer_cast<IEvent>(this->shared_from_this());
    }

    data_type *operator->() const noexcept { return this->_data.get(); }

    shared_ptr<data_type> data() { return this->_data; }

  private:
    shared_ptr<data_type> _data;
};

template <typename DataType> using event_type = shared_ptr<Event<DataType>>;
using observable_dynamic_event_type = rx::observable<shared_event_type>;
template <typename EventDataType>
using observable_event_type = rx::observable<event_type<EventDataType>>;
using subject_dynamic_event_type = rx::subjects::subject<shared_event_type>;

struct Observable {
    using observable_type = observable_dynamic_event_type;
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
        return this->_subject.get_observable();
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
    void post(shared_event_type e) {
        this->_subject.get_subscriber().on_next(e);
    }

  private:
    subject_dynamic_event_type _subject;
};

} // namespace my
