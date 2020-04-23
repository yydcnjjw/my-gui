#include "logger.h"

#include <cstdarg>
#include <cstring>

#include <future>
#include <iostream>

namespace {
class StdLoggerOutput : public Logger::LoggerOutput {
  public:
    explicit StdLoggerOutput(Logger::Level level = Logger::INFO)
        : Logger::LoggerOutput(level) {}
    ~StdLoggerOutput() override = default;
    void operator()(const Logger::LogMsg &msg) override {
        std::cout << msg.format() << std::endl;
        if (msg.level == Logger::ERROR) {
            std::terminate();
        }
    };
};
} // namespace

const Logger::bitmap Logger::logger_all_target = bitmap().set();

Logger::Logger() : _buf(4096, 0) {
    std::promise<rxcpp::observe_on_one_worker *> promise;
    auto future = promise.get_future();
    this->_log_thread = std::thread(
        [this](std::promise<rxcpp::observe_on_one_worker *> promise) {
            rxcpp::schedulers::run_loop rlp;
            auto rlp_worker = rxcpp::observe_on_run_loop(rlp);
            promise.set_value(&rlp_worker);
            pthread_setname_np(pthread_self(), "logger");
            for (;;) {
                while (!rlp.empty() && rlp.peek().when < rlp.now()) {
                    rlp.dispatch();
                }
                if (!this->_log_source.get_subscriber().is_subscribed()) {
                    break;
                }
                if (rlp.empty()) {
                    std::unique_lock<std::mutex> local_lock(this->_lock);
                    this->_cv.wait(local_lock, [&rlp] {
                        return !rlp.empty() && rlp.peek().when < rlp.now();
                    });
                }
            }
        },
        std::move(promise));
    this->_log_worker = future.get();

    // init output target
    this->addLogOutputTarget(std::make_shared<StdLoggerOutput>(Logger::DEBUG));
}

void Logger::Log(Logger::bitmap bit, Logger::Level type, const char *file_name,
                 int file_len, const char *fmt, ...) {
    std::lock_guard<std::mutex> local_lock(this->_lock);

    std::memset(this->_buf.data(), 0, this->_buf.size());
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(this->_buf.data(), this->_buf.length(), fmt, ap);
    va_end(ap);
    auto log_msg = std::make_shared<LogMsg>(bit, type, this->_buf.c_str(),
                                            file_name, file_len);
    this->_log_source.get_subscriber().on_next(log_msg);
    this->_cv.notify_all();
}

void Logger::addLogOutputTarget(const std::shared_ptr<LoggerOutput> &output) {
    if (this->_bitmap.all()) {
        throw std::runtime_error("number of output target overflow");
    }
    auto offset = this->_bitmap.count();
    this->_bitmap.set(offset);
    this->_addLogOutputTarget(offset, output);
}

void Logger::_addLogOutputTarget(unsigned long offset,
                                 const std::shared_ptr<LoggerOutput> &output) {
    bitmap bit;
    bit.set(offset);
    std::lock_guard<std::mutex> local_lock(this->_lock);
    this->_log_source.get_observable()
        .observe_on(*_log_worker)
        .filter([bit](const std::shared_ptr<LogMsg> &msg) {
            return (bit & msg->bitmap).any();
        })
        .filter([output](const std::shared_ptr<LogMsg> &msg) {
            return msg->level >= output->limit_level;
        })
        .subscribe(
            [output](const std::shared_ptr<LogMsg> &msg) { (*output)(*msg); });

    // _output_targets[bit] = s;
}

void Logger::close() {
    std::unique_lock<std::mutex> l_lock(this->_lock);
    this->_log_source.get_subscriber().on_completed();
    this->_log_source.get_subscriber().unsubscribe();
    this->_cv.notify_all();
    l_lock.unlock();
    if (this->_log_thread.joinable()) {
        this->_log_thread.join();
    }
}

std::string Logger::LogMsg::format() const {
    // TODO: In order to obtain temporary short path
    auto file_name = std::string(this->file_name);
    file_name = file_name.substr(file_name.rfind("src"));

    std::ostringstream os;
    os << "[" << Logger::LogMsg::get_level_str().at(this->level) << "]"
       << " " << file_name << " " << this->file_line << ": " << msg;
    return os.str();
}

const std::map<Logger::Level, const std::string> &
Logger::LogMsg::get_level_str() {
    static const std::map<Logger::Level, const std::string> _level_str = {
        {Logger::Level::DEBUG, "DEBUG"},
        {Logger::Level::INFO, "INFO"},
        {Logger::Level::WARN, "WARN"},
        {Logger::Level::ERROR, "ERROR"}};
    return _level_str;
}
