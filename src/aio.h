#pragma once

#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#include <boost/thread/future.hpp>
#include <filesystem>
#include <fstream>
namespace my {

namespace aio {
template <typename T> using future = boost::future<T>;

future<std::string> read(std::ifstream &is);
future<std::string> wirte();

} // namespace aio
} // namespace my
