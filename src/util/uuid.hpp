#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <mutex>

namespace my {
using uuid = boost::uuids::uuid;

uuid uuid_gen() {
    static std::mutex lock;
    std::unique_lock<std::mutex> l_lock(lock);
    static boost::uuids::random_generator gen;
    return gen();
}

} // namespace my
