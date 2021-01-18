#pragma once

#include <window/config.hpp>
#include <window/type.hpp>

namespace my {
class WindowServiceError : public std::runtime_error {
public:
  WindowServiceError(std::string const &msg)
      : std::runtime_error(msg) {}
};
} // namespace my
