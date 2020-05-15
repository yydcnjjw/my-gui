#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace my {
using uuid = boost::uuids::uuid;
namespace uuids {
using namespace boost::uuids;
}

uuid uuid_gen();

} // namespace my
