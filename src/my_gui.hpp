#pragma once

#include <filesystem>
#include <memory>

#include <boost/format.hpp>
#include <boost/mp11.hpp>
#include <boost/noncopyable.hpp>
#include <boost/program_options.hpp>
#define BOOST_URL_HEADER_ONLY
#include <boost/coroutine2/all.hpp>
#include <boost/url/url.hpp>

#include <util/async_task.hpp>
#include <util/codecvt.hpp>
#include <util/logger.h>
#include <util/uuid.hpp>

namespace my {
namespace fs = std::filesystem;
namespace program_options = boost::program_options;
namespace mp11 = boost::mp11;
using uri = boost::urls::url;
using coro_t = boost::coroutines2::coroutine<void>;

#define MY_DEBUG
} // namespace my
