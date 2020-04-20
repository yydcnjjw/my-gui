// #include <LLGL/LLGL.h>
// #include <LLGL/Log.h>
// #include <boost/timer/timer.hpp>
// #include <media/audio_mgr.h>
// #include <render/canvas.h>
// #include <render/window/window_mgr.h>
// #include <storage/resource_mgr.hpp>
// #include <util/event_bus.hpp>

// #include <util/logger.h>

#include <stdexcept>
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
}

#ifdef __cplusplus

char av_error[AV_ERROR_MAX_STRING_SIZE] = {0};
#undef av_err2str
#define av_err2str(errnum)                                                     \
    av_make_error_string(av_error, AV_ERROR_MAX_STRING_SIZE, errnum)

#endif // __cplusplus

int main(int, char *[]) {
    // auto ev_bus = my::EventBus::create();
    // auto async_task = my::AsyncTask::create();
    // auto resource_mgr = my::ResourceMgr::create(async_task.get());
    // auto audio_mgr = my::AudioMgr::create();
    // auto win_mgr = my::WindowMgr::create(ev_bus.get());
    // auto font_mgr = my::FontMgr::create();
    // auto win = win_mgr->create_window("test", 800, 600);
    // LLGL::Log::SetReportCallbackStd();
    // auto renderer = LLGL::RenderSystem::Load("Vulkan");
    // my::Canvas canvas(renderer.get(), win, resource_mgr.get(),
    // font_mgr.get());

    auto url =
        "/home/yydcnjjw/workspace/code/project/my-gui/assets/media/01.mp4";
    AVFormatContext *fmt_ctx{};
    int code{};
    if ((code = ::avformat_open_input(&fmt_ctx, url, nullptr, nullptr))) {
        throw std::runtime_error(av_err2str(code));
    }

    if ((code = ::avformat_find_stream_info(fmt_ctx, nullptr))) {
        throw std::runtime_error(av_err2str(code));
    }
    std::cout << fmt_ctx->nb_streams << std::endl;

    ::av_dump_format(fmt_ctx, 0, url, 0);
    // Logger::get()->close();
    return 0;
}
