#pragma once

#include <core/config.hpp>

namespace my {
class RenderServiceError : public std::runtime_error {
  public:
    RenderServiceError(const std::string &msg)
        : std::runtime_error(msg), _msg(msg) {}

    const char *what() const noexcept override { return this->_msg.c_str(); }

  private:
    std::string _msg;
};
} // namespace my
