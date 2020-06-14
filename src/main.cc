// #include <LLGL/LLGL.h>
// #include <LLGL/Log.h>
// #include <boost/timer/timer.hpp>
// #include <media/audio_mgr.h>
#include <media/avplayer.hpp>
// #include <render/canvas.h>
#include <application.h>
#include <media/audio_mgr.hpp>
#include <render/window/window_mgr.h>
#include <skia/include/core/SkFontMgr.h>
#include <skia/include/core/SkTypeface.h>
#include <storage/font.hpp>
#include <storage/image.hpp>
#include <storage/resource_mgr.hpp>

void test1(int argc, char *argv[]) {
    auto app = my::new_application(argc, argv, {});

    auto ev_bus = app->ev_bus();
    auto async_task = app->async_task();
    // auto resource_mgr = app->resource_mgr();
    // auto audio_mgr = my::AudioMgr::create();
    auto win_mgr = app->win_mgr();
    // auto font_mgr = my::FontMgr::create();

    auto url =
        "/home/yydcnjjw/workspace/code/project/my-gui/assets/media/10.mp4";

    auto win = win_mgr->create_window("test", 1280, 720);
    // LLGL::Log::SetReportCallbackStd();
    // auto renderer = LLGL::RenderSystem::Load("Vulkan");
    // my::Canvas canvas(renderer.get(), win, ev_bus.get(), resource_mgr.get(),
    //                   font_mgr.get());

    auto player = my::av::AVPlayer(url, async_task);

    struct PaintEvent {
        std::shared_ptr<SkBitmap> render_bitmap{};
        PaintEvent(std::shared_ptr<SkBitmap> render_bitmap)
            : render_bitmap(render_bitmap) {}
    };
    ev_bus->on_event<PaintEvent>()
        .observe_on(ev_bus->ev_bus_worker())
        .subscribe([win](const auto &e) {
            auto render_bitmap = e->data->render_bitmap;
            auto canvas = win->get_sk_surface()->getCanvas();
            canvas->clear(SkColors::kWhite.toSkColor());
            if (render_bitmap) {
                canvas->drawBitmap(*render_bitmap, 0, 0);
            }
            canvas->flush();
            win->swap_window();
        });

    player.set_render_cb([ev_bus](std::shared_ptr<SkBitmap> bitmap) {
        ev_bus->post<PaintEvent>(bitmap);
    });

    ev_bus->on_event<my::WindowEvent>()
        .subscribe_on(ev_bus->ev_bus_worker())
        .observe_on(ev_bus->ev_bus_worker())
        .subscribe([&](const std::shared_ptr<my::Event<my::WindowEvent>> &e) {
            if (e->data->event == SDL_WINDOWEVENT_CLOSE) {
                ev_bus->post<my::QuitEvent>();
            }
        });

    app->run();
}

void test2() {
    rxcpp::schedulers::run_loop rlp;
    auto rlp_worker = rxcpp::observe_on_run_loop(rlp);

    rxcpp::subjects::subject<int> subject;

    auto cs = subject
                  .get_observable()
                  // .subscribe_on(rlp_worker)
                  .observe_on(rlp_worker)
                  .subscribe([](int i) { std::cout << i << std::endl; });

    subject.get_subscriber().on_next(1);
    subject.get_subscriber().on_next(2);

    while (!rlp.empty() && rlp.peek().when < rlp.now()) {
        rlp.dispatch();
    }
}

int main(int argc, char *argv[]) {
    test1(argc, argv);
    return 0;
}
