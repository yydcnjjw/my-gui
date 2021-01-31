#pragma once

#include <boost/signals2.hpp>

namespace my {
template <typename Signature> using signal = boost::signals2::signal<Signature>;

} // namespace my
