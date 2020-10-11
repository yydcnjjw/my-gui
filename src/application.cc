#include "application.h"

#include <sstream>

#include <my_gui.hpp>

namespace {

class PosixApplication : public my::Application {
  public:
    PosixApplication(int argc, char **argv,
                     const my::program_options::options_description &opts_desc)
    // _async_task(my::AsyncTask::create()),
    // _win_mgr(my::WindowMgr::create(this->_ev_bus.get())),
    // _resource_mgr(my::ResourceMgr::create(this->_async_task.get())),
    // _font_mgr(my::FontMgr::create()),
    // _audio_mgr(my::AudioMgr::create()),
    // _renderer(LLGL::RenderSystem::Load("Vulkan"))
    {
        this->_parse_program_options(argc, argv, opts_desc);
    }

    ~PosixApplication() override {}

    void run() override {
        this->on_event<my::QuitEvent>()
            .observe_on(this->coordination())
            .subscribe([this](auto) {
                GLOG_D("invoke quit");
                this->quit();
            });

        try {
            this->_main_loop.run();
        } catch (std::exception &e) {
            GLOG_E(e.what());
        }
    }

    void quit() override { this->_main_loop.quit(); }

    coordination_type coordination() override {
        return this->_main_loop.coordination();
    }

    // my::EventBus *ev_bus() const override { return this->_ev_bus.get(); }
    // my::WindowMgr *win_mgr() const override { return this->_win_mgr.get(); };
    // my::FontMgr *font_mgr() const override { return this->_font_mgr.get(); }

    // my::AsyncTask *async_task() const override {
    //     return this->_async_task.get();
    // }

    // my::ResourceMgr *resource_mgr() const override {
    //     return this->_resource_mgr.get();
    // }

    // my::AudioMgr *audio_mgr() const override { return this->_audio_mgr.get();
    // }

    // LLGL::RenderSystem *renderer() const override {
    //     return this->_renderer.get();
    // }

    // std::shared_ptr<my::Canvas> make_canvas(my::Window *win) override {
    //     return std::make_shared<my::Canvas>(
    //         this->renderer(), win, this->ev_bus(), this->resource_mgr(),
    //         this->font_mgr());
    // }

    my::program_options::option_description &get_option_desc() {
        return this->_opts_desc;
    }

    bool get_program_option(
        const std::string &option,
        my::program_options::variable_value &value) const override {
        if (this->_program_option_map.count(option)) {
            value = this->_program_option_map[option];
            return true;
        } else {
            return false;
        }
    }

  private:
    my::program_options::option_description _opts_desc;
    my::program_options::variables_map _program_option_map;
    my::main_loop _main_loop;

    // std::unique_ptr<my::AsyncTask> _async_task;
    // std::unique_ptr<my::WindowMgr> _win_mgr;
    // std::unique_ptr<my::ResourceMgr> _resource_mgr;
    // std::unique_ptr<my::FontMgr> _font_mgr;
    // std::unique_ptr<my::AudioMgr> _audio_mgr;
    // std::unique_ptr<LLGL::RenderSystem> _renderer;

    void _parse_program_options(
        int argc, char **argv,
        const my::program_options::options_description &opts) {
        my::program_options::store(
            my::program_options::command_line_parser(argc, argv)
                .options(opts)
                .allow_unregistered()
                .run(),
            this->_program_option_map);
        my::program_options::notify(this->_program_option_map);
    }
};
} // namespace

namespace my {
std::unique_ptr<Application>
Application::create(int argc, char **argv,
                    const program_options::options_description &opts_desc) {
    return std::make_unique<PosixApplication>(argc, argv, opts_desc);
}
} // namespace my
