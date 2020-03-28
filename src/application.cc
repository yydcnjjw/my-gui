#include "application.h"

#include "logger.h"
#include "window_mgr.h"

namespace {
namespace program_options = boost::program_options;
class PosixApplication : public my::Application {
  public:
    PosixApplication(int argc, char *argv[])
        : Application(argc, argv), _ev_bus(std::make_unique<my::EventBus>()),
          _win_mgr(my::WindowMgr::create(this->_ev_bus.get())) {
        pthread_setname_np(pthread_self(), "main");
        this->_parse_program_options(argc, argv);
    }

    ~PosixApplication() override {}

    void run() override {
        try {
            auto win = this->_win_mgr->create_window("Test", 800, 600);

            this->_ev_bus->on_event<my::MouseMotionEvent>()
                .observe_on(this->_ev_bus->ev_bus_worker())
                .subscribe(
                    [](std::shared_ptr<my::Event<my::MouseMotionEvent>> e) {
                        GLOG_D("pos x = %d y = %d", e->data->pos.x,
                               e->data->pos.y);
                    });

            this->_ev_bus->run();
        } catch (std::exception &e) {
            GLOG_E(e.what());
        }
    }

    my::EventBus *get_ev_bus() const override { return this->_ev_bus.get(); }

    const program_options::variable_value &
    get_program_option_value(const std::string &option) const override {
        return this->_program_option_map[option];
    }

  private:
    program_options::variables_map _program_option_map;
    std::unique_ptr<my::EventBus> _ev_bus;
    std::unique_ptr<my::WindowMgr> _win_mgr;

    void _parse_program_options(int argc, char *argv[]) {
        program_options::options_description desc("options");
        program_options::store(
            program_options::parse_command_line(argc, argv, desc),
            this->_program_option_map);
        program_options::notify(this->_program_option_map);
        auto a = this->_program_option_map["aaa"];
    }
};
} // namespace

namespace my {
std::shared_ptr<Application> new_application(int argc, char *argv[]) {
    return std::make_shared<PosixApplication>(argc, argv);
}
} // namespace my
