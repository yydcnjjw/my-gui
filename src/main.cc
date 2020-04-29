#include <LLGL/LLGL.h>
#include <LLGL/Log.h>
#include <boost/timer/timer.hpp>
#include <render/canvas.h>
#include <render/window/window_mgr.h>
#include <storage/resource_mgr.hpp>
#include <util/async_task.hpp>
#include <util/event_bus.hpp>
#include <util/avplayer.hpp>

int main(int, char *[]) {
    auto ev_bus = my::EventBus::create();
    auto async_task = my::AsyncTask::create();
    auto resource_mgr = my::ResourceMgr::create(async_task.get());
    // auto audio_mgr = my::AudioMgr::create();
    auto win_mgr = my::WindowMgr::create(ev_bus.get());
    auto font_mgr = my::FontMgr::create();

    auto url =
        "/home/yydcnjjw/workspace/code/project/my-gui/assets/media/10.mp4";

    auto win = win_mgr->create_window("test", 1280, 720);
    LLGL::Log::SetReportCallbackStd();
    auto renderer = LLGL::RenderSystem::Load("Vulkan");
    my::Canvas canvas(renderer.get(), win, ev_bus.get(), resource_mgr.get(),
                      font_mgr.get());
    auto player = my::av::AVPlayer(url, &canvas, async_task.get());

    std::this_thread::sleep_for(std::chrono::seconds(100));

    Logger::get()->close();
    return 0;
}
