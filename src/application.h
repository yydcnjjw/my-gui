#pragma once

#include <memory>

namespace my {
class Application {
  public:
    Application(int, char *[]){};
    virtual ~Application() = default;
    virtual void run() = 0;
};

std::shared_ptr<Application> new_application(int argc, char *argv[]);
} // namespace my
