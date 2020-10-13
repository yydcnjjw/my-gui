#pragma once

#include <filesystem>
#include <memory>

#include <boost/format.hpp>
#include <boost/mp11.hpp>
#include <boost/noncopyable.hpp>
#include <boost/program_options.hpp>
#include <boost/coroutine2/all.hpp>

#include <util/async_task.hpp>
#include <util/codecvt.hpp>
#include <util/logger.h>
#include <util/uuid.hpp>

namespace my {
namespace fs = std::filesystem;
namespace po = boost::program_options;
namespace mp11 = boost::mp11;
using coro_t = boost::coroutines2::coroutine<void>;

#define MY_DEBUG
} // namespace my
