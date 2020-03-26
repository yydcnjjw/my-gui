#include "application.h"

#include "event_bus.hpp"
#include "logger.h"
#include "window_mgr.h"

#include <boost/program_options.hpp>

namespace {
namespace program_options = boost::program_options;
class PosixApplication : public my::Application {
  public:
    PosixApplication(int argc, char *argv[])
        : Application(argc, argv), _ev_bus(std::make_unique<my::EventBus>()) {
        pthread_setname_np(pthread_self(), "main");
        this->_parse_program_options(argc, argv);
    }

    ~PosixApplication() override {}

    void run() override {
        try {
            auto win_mgr = my::get_window_mgr(this->_ev_bus.get());
            auto win = win_mgr->create_window("Test", 800, 600);

            this->_ev_bus->on_event<my::MouseMotionEvent>()
                .observe_on(this->_ev_bus->ev_bus_worker())
                .subscribe(
                    [](std::shared_ptr<my::Event<my::MouseMotionEvent>> e) {
                        GLOG_D("pos x = %d y = %d", e->data->pos.x, e->data->pos.y);
                    });

            this->_ev_bus->run();
        } catch (std::exception &e) {
            GLOG_E(e.what());
        }
        // auto t = std::thread([this]() {
        //     std::this_thread::sleep_for(std::chrono::seconds(1));
        // });

        // if (t.joinable()) {
        //     t.join();
        // }
    }

  private:
    program_options::variables_map _program_option_map;
    std::unique_ptr<my::EventBus> _ev_bus;

    void _parse_program_options(int argc, char *argv[]) {
        program_options::options_description desc("options");
        program_options::store(
            program_options::parse_command_line(argc, argv, desc),
            this->_program_option_map);
        program_options::notify(this->_program_option_map);
    }
};
} // namespace

namespace my {
std::shared_ptr<Application> new_application(int argc, char *argv[]) {
    return std::make_shared<PosixApplication>(argc, argv);
}
} // namespace my
