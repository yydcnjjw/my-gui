#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <mutex>

namespace my {
using uuid = boost::uuids::uuid;

uuid uuid_gen();

} // namespace my
