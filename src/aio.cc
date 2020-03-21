#include "aio.h"

namespace {} // namespace

namespace my {
namespace aio {
boost::asio::thread_pool &AIOThreadPool::get() {
    static AIOThreadPool pool;
    return pool._pool;
}

future<std::string> file_read_all(const fs::path &path) {
    return do_async<std::string>(
        [&path](std::shared_ptr<promise<std::string>> promise) {
            try {
                auto size = fs::file_size(path);
                std::ifstream ifs(path);
                std::string data(size, 0);
                ifs.readsome(data.data(), size);
                promise->set_value(std::move(data));
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });
}

} // namespace aio
} // namespace my
