#pragma once

#include <any>

#include <core/coordination.hpp>

// Experimental begin
#define DECL_API(service, name, ret, args...) virtual ret name(args) = 0;

#define DEFINE_API_INVOKE(service, name, ret, args...)                         \
    std::any invoke_##name(shared_ptr<service> self, shared_ptr<IEvent> e) {   \
        return std::apply(                                                     \
            static_cast<ret (service::*)(args)>(&service::name),               \
            std::tuple_cat(                                                    \
                std::make_tuple(self),                                         \
                std::dynamic_pointer_cast<Event<ServiceRequestEvent<args>>>(e) \
                    ->params));                                                \
    }

#define REGISTER_API(service, name, ret, args...)                              \
    { #name, &service::invoke_##name }

#define SERVICE_API(service, map)                                              \
    map(DECL_API) map(DEFINE_API_INVOKE)                                       \
        std::multimap<std::string, std::any (service::*)(shared_ptr<service>,  \
                                                         shared_ptr<IEvent>)>  \
            api_map_{map(REGISTER_API)};
// Experimental end

namespace my {

// rpc
template <typename... Args> struct ServiceRequestEvent {
    std::string service;
    std::string function;
    std::tuple<Args...> params;

    ServiceRequestEvent(const std::string &service, const std::string &function,
                        Args &&... args)
        : service(service), function(function),
          params(std::forward<Args>(args)...) {}
};

class BasicService {
  public:
    virtual ~BasicService() = default;
    observe_on_one_thread &coordination() { return this->coordination_; }

    using DEFAULT_EXIT_FUNCTION_TYPE = std::function<void()>;
    template <typename Before = DEFAULT_EXIT_FUNCTION_TYPE,
              typename After = DEFAULT_EXIT_FUNCTION_TYPE>
    future<void> exit(
        Before &&before = []() {}, After &&after = []() {}) {
        return this->schedule<void>(
            [this, before = std::forward<Before>(before),
             after = std::forward<After>(after)]() -> void {
                before();
                this->coordination().coordinator_state().unsubscribe();
                after();
            });
    }

  protected:
    template <typename T, typename Func>
    future<T> schedule(Func &&func,
                       typename std::enable_if_t<std::is_void_v<T>> * = 0) {
        auto p = std::make_shared<promise<T>>();
        if (this->coordination().get_thread_info().thread_id ==
            std::this_thread::get_id()) {
            func();
            p->set_value();
        } else {
            this->coordination().coordinator().get_worker().schedule(
                [p, func](auto) {
                    try {
                        func();
                        p->set_value();
                    } catch (...) {
                        try {
                            p->set_exception(std::current_exception());
                        } catch (...) {
                        }
                    }
                });
        }
        return p->get_future();
    }
    template <typename T, typename Func>
    future<T> schedule(Func &&func,
                       typename std::enable_if_t<!std::is_void_v<T>> * = 0) {
        auto p = std::make_shared<promise<T>>();
        if (this->coordination().get_thread_info().thread_id ==
            std::this_thread::get_id()) {
            p->set_value(func());
        } else {
            this->coordination().coordinator().get_worker().schedule(
                [p, func = std::forward<Func>(func)](auto) {
                    try {
                        p->set_value(func());
                    } catch (...) {
                        try {
                            p->set_exception(std::current_exception());
                        } catch (...) {
                        }
                    }
                });
        }
        return p->get_future();
    }

  private:
    observe_on_one_thread coordination_;
};
} // namespace my
