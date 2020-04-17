#include "application.h"

#include <sstream>

#include <util/logger.h>

namespace {
namespace program_options = boost::program_options;
class PosixApplication : public my::Application {
  public:
    PosixApplication(int argc, char **argv,
                     const program_options::options_description &opts_desc)
        : _ev_bus(my::EventBus::create()), _async_task(my::AsyncTask::create()),
          _win_mgr(my::WindowMgr::create(this->_ev_bus.get())),
          _resource_mgr(my::ResourceMgr::create(this->_async_task.get())),
          _font_mgr(my::FontMgr::create()), _audio_mgr(my::AudioMgr::create()) {
        // XXX:
        pthread_setname_np(pthread_self(), "main");
        this->_parse_program_options(argc, argv, opts_desc);
    }

    ~PosixApplication() override {}

    void run() override {
        try {
            this->_ev_bus->run();
            Logger::get()->close();
        } catch (std::exception &e) {
            GLOG_E(e.what());
        }
    }

    void quit() override { this->_ev_bus->post<my::QuitEvent>(); }

    my::EventBus *ev_bus() const override { return this->_ev_bus.get(); }
    my::WindowMgr *win_mgr() const override { return this->_win_mgr.get(); };
    my::FontMgr *font_mgr() const override { return this->_font_mgr.get(); }

    my::AsyncTask *async_task() const override {
        return this->_async_task.get();
    }

    my::ResourceMgr *resource_mgr() const override {
        return this->_resource_mgr.get();
    }

    my::AudioMgr *audio_mgr() const override {
        return this->_audio_mgr.get();
    }

    program_options::option_description &get_option_desc() {
        return this->_opts_desc;
    }

    bool
    get_program_option(const std::string &option,
                       program_options::variable_value &value) const override {
        if (this->_program_option_map.count(option)) {
            value = this->_program_option_map[option];
            return true;
        } else {
            return false;
        }
    }

  private:
    program_options::option_description _opts_desc;
    program_options::variables_map _program_option_map;
    std::unique_ptr<my::EventBus> _ev_bus;
    std::unique_ptr<my::AsyncTask> _async_task;
    std::unique_ptr<my::WindowMgr> _win_mgr;
    std::unique_ptr<my::ResourceMgr> _resource_mgr;
    std::unique_ptr<my::FontMgr> _font_mgr;
    std::unique_ptr<my::AudioMgr> _audio_mgr;

    void
    _parse_program_options(int argc, char **argv,
                           const program_options::options_description &opts) {
        program_options::store(program_options::command_line_parser(argc, argv)
                                   .options(opts)
                                   .allow_unregistered()
                                   .run(),
                               this->_program_option_map);
        program_options::notify(this->_program_option_map);
    }
};
} // namespace

namespace my {
std::shared_ptr<Application>
new_application(int argc, char **argv,
                const program_options::options_description &opts_desc) {
    return std::make_shared<PosixApplication>(argc, argv, opts_desc);
}
} // namespace my
