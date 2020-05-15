#include "uuid.hpp"

#include <mutex>

#include <boost/uuid/uuid_generators.hpp>

namespace my {
uuid uuid_gen() {
    static std::mutex lock;
    std::unique_lock<std::mutex> l_lock(lock);
    static uuids::random_generator gen;
    return gen();
}
} // namespace my
