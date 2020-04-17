#include <LLGL/LLGL.h>
#include <LLGL/Log.h>
#include <boost/timer/timer.hpp>
#include <media/audio_mgr.h>
#include <render/canvas.h>
#include <render/window/window_mgr.h>
#include <storage/resource_mgr.hpp>
#include <util/event_bus.hpp>

#include <util/logger.h>

int main(int, char *[]) {
    auto ev_bus = my::EventBus::create();
    auto async_task = my::AsyncTask::create();
    auto resource_mgr = my::ResourceMgr::create(async_task.get());
    auto audio_mgr = my::AudioMgr::create();
    auto win_mgr = my::WindowMgr::create(ev_bus.get());
    auto font_mgr = my::FontMgr::create();
    auto win = win_mgr->create_window("test", 800, 600);
    LLGL::Log::SetReportCallbackStd();
    auto renderer = LLGL::RenderSystem::Load("Vulkan");
    my::Canvas canvas(renderer.get(), win, resource_mgr.get(), font_mgr.get());


    

    Logger::get()->close();
    return 0;
}
