#include "async_task.hpp"

namespace {} // namespace

namespace my {
std::unique_ptr<AsyncTask> AsyncTask::create() {

    return std::make_unique<AsyncTask>();
}
} // namespace my
