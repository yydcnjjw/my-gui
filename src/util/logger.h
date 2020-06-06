#pragma once

#include <rx.hpp>

#include <bitset>
#include <shared_mutex>

class Logger {
  public:
    enum Level { DEBUG, INFO, WARN, ERROR };
    using bitmap = std::bitset<32>;

    static const Logger::bitmap logger_all_target;
    struct LogMsg {
        Logger::bitmap bitmap;
        Logger::Level level;
        std::string msg;
        const char *file_name;
        int file_line;
        LogMsg(Logger::bitmap map, Level level, std::string msg,
               const char *file_name, int file_line)
            : bitmap(map), level(level), msg(std::move(msg)),
              file_name(file_name), file_line(file_line) {}

        [[nodiscard]] std::string format() const;

      private:
        static const std::map<Level, const std::string> &get_level_str();
    };

    class LoggerOutput {
      public:
        explicit LoggerOutput(Level level = INFO) : limit_level(level) {}
        virtual void operator()(const Logger::LogMsg &msg) = 0;
        virtual ~LoggerOutput() = default;
        Logger::Level limit_level;
    };

    void addLogOutputTarget(const std::shared_ptr<LoggerOutput> &output);

    void Log(bitmap bitmap, Logger::Level type, const char *file_name,
             int file_len, const char *fmt, ...);

    static Logger *get() {
        static Logger instance;
        return &instance;
    }

    void close();

    void set_level(Level level) {
        this->_level = level;
    }

  private:
    Logger();
    
    bitmap _bitmap;
    Level _level{DEBUG};

    std::string _buf;
    std::mutex _lock;
    std::condition_variable _cv;
    void _addLogOutputTarget(unsigned long offset,
                             const std::shared_ptr<LoggerOutput> &output);

    std::map<bitmap, rxcpp::subscription> _output_targets;

    rxcpp::subjects::subject<std::shared_ptr<LogMsg>> _log_source;
    // NOTE: log thread log_worker ref(stack var)
    // do not need to release
    rxcpp::observe_on_one_worker *_log_worker;
    std::thread _log_thread;
};

#define LOG_D(logger, target, fmt, args...)                                    \
    (logger)->Log(target, Logger::Level::DEBUG, __FILE__, __LINE__, fmt, ##args)

#define LOG_I(logger, target, fmt, args...)                                    \
    (logger)->Log(target, Logger::Level::INFO, __FILE__, __LINE__, fmt, ##args)

#define LOG_W(logger, target, fmt, args...)                                    \
    (logger)->Log(target, Logger::Level::WARN, __FILE__, __LINE__, fmt, ##args)

#define LOG_E(logger, target, fmt, args...)                                    \
    (logger)->Log(target, Logger::Level::ERROR, __FILE__, __LINE__, fmt, ##args)

#define GLOG_T_D(target, fmt, args...)                                         \
    LOG_D(Logger::get(), target, fmt, ##args)

#define GLOG_T_I(target, fmt, args...)                                         \
    LOG_I(Logger::get(), target, fmt, ##args)

#define GLOG_T_W(target, fmt, args...)                                         \
    LOG_W(Logger::get(), target, fmt, ##args)

#define GLOG_T_E(target, fmt, args...)                                         \
    LOG_E(Logger::get(), target, fmt, ##args)

#define GLOG_D(fmt, args...)                                                   \
    GLOG_T_D(Logger::logger_all_target, fmt, ##args)

#define GLOG_I(fmt, args...)                                                   \
    GLOG_T_I(Logger::logger_all_target, fmt, ##args)

#define GLOG_W(fmt, args...)                                                   \
    GLOG_T_W(Logger::logger_all_target, fmt, ##args)

#define GLOG_E(fmt, args...)                                                   \
    GLOG_T_E(Logger::logger_all_target, fmt, ##args)
