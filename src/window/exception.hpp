#pragma once

#include <window/config.hpp>
#include <window/type.hpp>

namespace my {
class WindowServiceError : public std::runtime_error {
  public:
    WindowServiceError(const std::string &msg)
        : std::runtime_error(msg), _msg(msg) {}

    const char *what() const noexcept override { return this->_msg.c_str(); }

  private:
    std::string _msg;
};
} // namespace my
