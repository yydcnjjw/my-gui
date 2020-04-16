#pragma once

#include <filesystem>
#include <memory>

#include <boost/mp11.hpp>
#include <boost/program_options.hpp>

#include <util/logger.h>

namespace my {
namespace fs = std::filesystem;
namespace program_options = boost::program_options;
namespace mp11 = boost::mp11;
#define MY_DEBUG
} // namespace my
