#pragma once

#include <boost/format.hpp>

#include <core/basic_service.hpp>
#include <core/config.hpp>
namespace my {
class Logger : public my::BasicService {
    typedef my::BasicService base_type;

  public:
    enum Level { DEBUG, INFO, WARN, ERROR };

    struct LogMsg {
        Logger::Level level;
        std::string msg;
        const char *file_name;
        int file_line;
        LogMsg(Level level, std::string msg, const char *file_name,
               int file_line)
            : level(level), msg(std::move(msg)), file_name(file_name),
              file_line(file_line) {}

        std::string format() const {
            // TODO: In order to obtain temporary short path
            auto file_name = std::string(this->file_name);
            file_name = file_name.substr(file_name.rfind("src"));

            std::ostringstream os;
            os << "[" << Logger::to_level_str(this->level) << "]"
               << " " << file_name << " " << this->file_line << ": " << msg;
            return os.str();
        }
    };

    class LoggerOutput {
      public:
        explicit LoggerOutput(Level level = INFO) : limit_level(level) {}
        virtual void operator()(const Logger::LogMsg &msg) = 0;
        virtual ~LoggerOutput() = default;
        Logger::Level limit_level;
    };

    typedef rxcpp::subjects::subject<shared_ptr<LogMsg>> observable_type;

    Logger();
    ~Logger() { this->logger_exit(); }

    void addLogOutputTarget(const shared_ptr<LoggerOutput> &output);

    template <typename... Args>
    void Log(Logger::Level type, const char *file_name, int file_len,
             const char *fmt, Args &&... args) {
        auto log_msg = std::make_shared<LogMsg>(
            type, (boost::format(fmt) % ... % std::forward<Args>(args)).str(),
            file_name, file_len);
        this->log_source().get_subscriber().on_next(log_msg);
    }

    static Logger *get() {
        static Logger instance;
        return &instance;
    }

    void set_level(Level level) { this->_level = level; }

    static const std::string &to_level_str(Logger::Level l) {
        static const std::map<Logger::Level, const std::string> _level_str = {
            {Logger::DEBUG, "DEBUG"},
            {Logger::INFO, "INFO"},
            {Logger::WARN, "WARN"},
            {Logger::ERROR, "ERROR"}};
        return _level_str.at(l);
    }

  private:
    Level _level{DEBUG};

    std::set<rxcpp::composite_subscription> _output_targets;

    observable_type _log_source;

    observable_type &log_source() { return this->_log_source; }

    void logger_exit() {
        this->log_source().get_observable().subscribe();
        this->log_source().get_subscriber().on_completed();
    }
};
} // namespace my
#define LOG_D(logger, fmt, args...)                                            \
    (logger)->Log(my::Logger::DEBUG, __FILE__, __LINE__, fmt, ##args)

#define LOG_I(logger, fmt, args...)                                            \
    (logger)->Log(my::Logger::INFO, __FILE__, __LINE__, fmt, ##args)

#define LOG_W(logger, fmt, args...)                                            \
    (logger)->Log(my::Logger::WARN, __FILE__, __LINE__, fmt, ##args)

#define LOG_E(logger, fmt, args...)                                            \
    (logger)->Log(my::Logger::ERROR, __FILE__, __LINE__, fmt, ##args)

#define GLOG_D(fmt, args...) LOG_D(my::Logger::get(), fmt, ##args)

#define GLOG_I(fmt, args...) LOG_I(my::Logger::get(), fmt, ##args)

#define GLOG_W(fmt, args...) LOG_W(my::Logger::get(), fmt, ##args)

#define GLOG_E(fmt, args...) LOG_E(my::Logger::get(), fmt, ##args)
