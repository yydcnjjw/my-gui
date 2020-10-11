#include "logger.h"

#include <cstdarg>
#include <cstring>

#include <fstream>
#include <future>
#include <iostream>

#include <my_gui.hpp>

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

class FileLoggerOutput : public Logger::LoggerOutput {
  public:
    explicit FileLoggerOutput(const my::fs::path &path,
                              Logger::Level level = Logger::INFO)
        : Logger::LoggerOutput(level) {
        my::fs::remove(path);
        this->_ofs.exceptions(std::ios::failbit | std::ios::badbit);
        this->_ofs.open(path);
    }
    ~FileLoggerOutput() override = default;
    void operator()(const Logger::LogMsg &msg) override {
        this->_ofs << msg.format() << std::endl;
        this->_ofs.flush();
        if (msg.level == Logger::ERROR) {
            std::terminate();
        }
    };

  private:
    std::ofstream _ofs;
};

} // namespace

Logger::Logger() {
    // init output target
    this->addLogOutputTarget(std::make_shared<StdLoggerOutput>(Logger::DEBUG));
    this->addLogOutputTarget(
        std::make_shared<FileLoggerOutput>("log.txt", Logger::DEBUG));
}

void Logger::addLogOutputTarget(const std::shared_ptr<LoggerOutput> &output) {
    this->log_source()
        .get_observable()
        .observe_on(this->coordination().get())
        .filter([output, this](const std::shared_ptr<LogMsg> &msg) {
            return msg->level >= output->limit_level &&
                   msg->level >= this->_level;
        })
        .subscribe(
            [output](const std::shared_ptr<LogMsg> &msg) { (*output)(*msg); });
}


