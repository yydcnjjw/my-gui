#pragma once

#include <rx.hpp>

#include <bitset>
#include <mutex>

class Logger {
  public:
    enum Level { DEBUG, INFO, WARN, ERROR };
    typedef std::bitset<32> bitmap;

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

    Logger();
    ~Logger();

    void addLogOutputTarget(const std::shared_ptr<LoggerOutput> &output);

    void Log(bitmap bitmap, Logger::Level type, const char *file_name,
             int file_len, const char *fmt, ...);

  private:
    bitmap _bitmap;

    std::string _buf;
    std::mutex _lock;
    void _addLogOutputTarget(unsigned long offset,
                             const std::shared_ptr<LoggerOutput> &output);

    std::map<bitmap, rxcpp::subscription> _output_targets;

    rxcpp::subjects::subject<std::shared_ptr<LogMsg>> _log_source;
    // NOTE: log thread log_worker ref(stack var)
    // do not need to release
    rxcpp::observe_on_one_worker *_log_worker;
    std::thread _log_thread;
};

Logger *_get_g_logger();

#define LOG_D(logger, target, fmt, args...)                                    \
    (logger)->Log(target, Logger::Level::DEBUG, __FILE__, __LINE__, fmt, ##args)

#define LOG_I(logger, target, fmt, args...)                                    \
    (logger)->Log(target, Logger::Level::INFO, __FILE__, __LINE__, fmt, ##args)

#define LOG_W(logger, target, fmt, args...)                                    \
    (logger)->Log(target, Logger::Level::WARN, __FILE__, __LINE__, fmt, ##args)

#define LOG_E(logger, target, fmt, args...)                                    \
    (logger)->Log(target, Logger::Level::ERROR, __FILE__, __LINE__, fmt, ##args)

#define GLOG_T_D(target, fmt, args...)                                         \
    LOG_D(_get_g_logger(), target, fmt, ##args)

#define GLOG_T_I(target, fmt, args...)                                         \
    LOG_I(_get_g_logger(), target, fmt, ##args)

#define GLOG_T_W(target, fmt, args...)                                         \
    LOG_W(_get_g_logger(), target, fmt, ##args)

#define GLOG_T_E(target, fmt, args...)                                         \
    LOG_E(_get_g_logger(), target, fmt, ##args)

#define GLOG_D(fmt, args...)                                                   \
    GLOG_T_D(Logger::logger_all_target, fmt, ##args)

#define GLOG_I(fmt, args...)                                                   \
    GLOG_T_I(Logger::logger_all_target, fmt, ##args)

#define GLOG_W(fmt, args...)                                                   \
    GLOG_T_W(Logger::logger_all_target, fmt, ##args)

#define GLOG_E(fmt, args...)                                                   \
    GLOG_T_E(Logger::logger_all_target, fmt, ##args)
