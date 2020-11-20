#pragma once

#include <core/event_bus.hpp>

namespace my {
class Object : public Observable {
  // public:
  //   Object() : _event_bus(Application::get()->event_source()) {}

  //   template <typename T, typename... Args> void post(Args &&...args) {
  //       Application::get()->post<T>(std::forward<Args>(args)...);
  //   }

  //   observable_type event_source() override { return this->_event_bus; }

  // private:
  //   observable_type _event_bus;
};

// class Bindable {
//   public:
//     // template <typename _T, typename = std::enable_if_t<false>>
//     // void connect(_T &&) {}

//     // template <typename _T, typename = std::enable_if_t<false>>
//     // void disconnect(_T &&) {}

//     template <typename T1, typename T2,
//               typename = std::enable_if_t<std::is_base_of_v<Bindable, T1> &&
//                                           std::is_base_of_v<Bindable, T2>>>
//     static void connect(T1 &&t1, T2 &&t2) {
//         t1.connect(t2);
//         t2.connect(t1);
//     }

//     template <typename T1, typename T2,
//               typename = std::enable_if_t<std::is_base_of_v<Bindable, T1> &&
//                                           std::is_base_of_v<Bindable, T2>>>
//     static void disconnect(T1 &&t1, T2 &&t2) {
//         t1.disconnect(t2);
//         t2.disconnect(t1);
//     }
// };

} // namespace my
