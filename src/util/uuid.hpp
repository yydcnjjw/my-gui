#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <mutex>

namespace my {

using uuid = boost::uuids::uuid;

namespace uuids {
using namespace boost::uuids;
}

inline uuid uuid_gen() {
    static std::mutex lock;
    std::unique_lock<std::mutex> l_lock(lock);
    static uuids::random_generator gen;
    return gen();
}

} // namespace my
