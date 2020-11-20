#include "application.hpp"

#include <sstream>

namespace {

class PosixApplication : public my::Application {
    using base_type = my::Application;

  public:
    PosixApplication(int argc, char **argv,
                     const base_type::options_description &opts_desc)
        : base_type(argc, argv, opts_desc)
    // _async_task(my::AsyncTask::create()),
    // _win_mgr(my::WindowMgr::create(this->_ev_bus.get())),
    // _resource_mgr(my::ResourceMgr::create(this->_async_task.get())),
    // _font_mgr(my::FontMgr::create()),
    // _audio_mgr(my::AudioMgr::create()),
    // _renderer(LLGL::RenderSystem::Load("Vulkan"))
    {}

    ~PosixApplication() override {}

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

  private:
    // std::unique_ptr<my::AsyncTask> _async_task;
    // std::unique_ptr<my::WindowMgr> _win_mgr;
    // std::unique_ptr<my::ResourceMgr> _resource_mgr;
    // std::unique_ptr<my::FontMgr> _font_mgr;
    // std::unique_ptr<my::AudioMgr> _audio_mgr;
    // std::unique_ptr<LLGL::RenderSystem> _renderer;
};
} // namespace

namespace my {

Application *Application::_instance = nullptr;

unique_ptr<Application>
Application::create(int argc, char **argv,
                    const po::options_description &opts_desc) {
    return std::make_unique<PosixApplication>(argc, argv, opts_desc);
}
} // namespace my
