// #include <LLGL/LLGL.h>
// #include <LLGL/Log.h>
// #include <boost/timer/timer.hpp>
// #include <media/audio_mgr.h>
// #include <media/avplayer.hpp>
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
    auto resource_mgr = app->resource_mgr();
    // auto audio_mgr = my::AudioMgr::create();
    auto win_mgr = app->win_mgr();
    // auto font_mgr = my::FontMgr::create();

    // auto url =
    //     "/home/yydcnjjw/workspace/code/project/my-gui/assets/media/10.mp4";

    auto win = win_mgr->create_window("test", 1280, 720);
    // LLGL::Log::SetReportCallbackStd();
    // auto renderer = LLGL::RenderSystem::Load("Vulkan");
    // my::Canvas canvas(renderer.get(), win, ev_bus.get(), resource_mgr.get(),
    //                   font_mgr.get());
    // auto player = my::av::AVPlayer(url, &canvas, async_task.get());

    // std::this_thread::sleep_for(std::chrono::seconds(100));

    auto audio = resource_mgr->load<my::Audio>("test.mp3").get();
    auto audio2 = resource_mgr
                      ->load<my::Audio>(my::make_archive_search_uri(
                          "data.xp3", "bgm/title.ogg"))
                      .get();
    audio->play();
    audio2->play();

    auto test_image = resource_mgr
                          ->load<my::Image>(my::make_archive_search_uri(
                              "data.xp3", "image/sysbt_bt_auto_3mode.png"))
                          .get();

    auto surface = win->get_sk_surface();
    auto canvas = surface->getCanvas();

    struct Paint {};
    auto timer = async_task->create_timer_interval(
        [&]() { ev_bus->post<Paint>(); }, std::chrono::milliseconds(1000 / 60));

    timer->start();

    SkFont font{};
    font.setSize(30);

    auto my_font =
        resource_mgr->load<my::Font>("NotoSansCJK-Regular.ttc").get();
    font.setTypeface(my_font->get_sk_typeface());

    auto cpu_surface = SkSurface::MakeRasterN32Premul(400, 400);
    auto cpu_canvas = cpu_surface->getCanvas();
    {
        SkPaint paint{};
        paint.setBlendMode(SkBlendMode::kSrc);
        paint.setColor(0);
        // paint.setAlpha(128);
        cpu_canvas->drawIRect(my::IRect::MakeXYWH(0, 0, 100, 100), paint);
    }
    {
        SkPaint paint{};
        paint.setColor(0xffffffff);
        cpu_canvas->drawString("Hello World!", 0, 50, font, paint);
    }

    auto image = cpu_surface->makeImageSnapshot();

    ev_bus->on_event<Paint>()
        .subscribe_on(ev_bus->ev_bus_worker())
        .observe_on(ev_bus->ev_bus_worker())
        .subscribe([&](const auto &) {
            canvas->clear(SK_ColorWHITE);

            canvas->save();
            {
                SkPaint paint;
                paint.setColor(SK_ColorRED);
                canvas->drawIRect(my::IRect::MakeXYWH(0, 0, 200, 200), paint);
            }

            canvas->drawImage(image, 100, 100);

            canvas->restore();
            canvas->flush();

            win->swap_window();
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

    auto cs = subject.get_observable()
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
    test2();
    return 0;
}
