#include <condition_variable>
#include <queue>
#include <shared_mutex>

namespace my {
template <class T,
          auto max_size =
              std::numeric_limits<typename std::queue<T>::size_type>::max()>
class Queue {
  public:
    T &front() {
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);
        this->_cv.wait(l_lock, [this]() { return !this->_queue.empty(); });
        return this->_queue.front();
    }

    T &back() {
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);
        this->_cv.wait(l_lock, [this]() { return !this->_queue.empty(); });
        return this->_queue.back();
    }

    bool empty() {
        std::shared_lock<std::shared_mutex> l_lock(this->_lock);
        return this->_queue.empty();
    }

    void push(const T &v) {
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);
        this->_cv.wait(l_lock,
                       [this]() { return this->_queue.size() < max_size; });

        this->_queue.push(v);
        this->_cv.notify_one();
    }

    void push(T &&v) {
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);
        this->_cv.wait(l_lock,
                       [this]() { return this->_queue.size() < max_size; });

        this->_queue.push(std::move(v));
        this->_cv.notify_one();
    }

    T pop() {
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);
        this->_cv.wait(l_lock, [this]() { return !this->_queue.empty(); });

        auto v = this->_queue.front();
        this->_queue.pop();
        this->_cv.notify_one();
        return v;
    }

    auto size() {
        std::shared_lock<std::shared_mutex> l_lock(this->_lock);
        return this->_queue.size();
    }

  protected:
    std::queue<T> _queue;
    std::condition_variable_any _cv;
    std::shared_mutex _lock;
};

} // namespace my
