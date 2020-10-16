#pragma once

#include <filesystem>
#include <memory>

#include <boost/coroutine2/all.hpp>
#include <boost/mp11.hpp>
#include <boost/program_options.hpp>
#include <boost/locale/encoding.hpp>
#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#include <boost/thread/future.hpp>
#include <rx.hpp>

namespace my {
namespace fs = std::filesystem;

template <typename T> using shared_ptr = std::shared_ptr<T>;
template <typename T> using unique_ptr = std::unique_ptr<T>;

namespace po = boost::program_options;
namespace mp11 = boost::mp11;

namespace codecvt = boost::locale::conv;
template <typename T> using coroutine = boost::coroutines2::coroutine<T>;
template <typename T> using future = boost::future<T>;
template <typename T> using promise = boost::promise<T>;

namespace rx = rxcpp;

#define MY_DEBUG
} // namespace my
