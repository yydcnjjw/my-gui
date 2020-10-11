// #include <LLGL/LLGL.h>
// #include <LLGL/Log.h>
// #include <boost/timer/timer.hpp>
// #include <media/audio_mgr.h>
// #include <media/avplayer.hpp>
// // #include <render/canvas.h>
// #include <application.h>
// #include <media/audio_mgr.hpp>
// #include <render/window/window_mgr.h>
// #include <skia/include/core/SkFontMgr.h>
// #include <skia/include/core/SkImageGenerator.h>
// #include <skia/include/core/SkTypeface.h>
// #include <storage/font.hpp>
// #include <storage/image.hpp>
// #include <storage/resource_mgr.hpp>

#include <rx.hpp>

// void test1(int argc, char *argv[]) {
//     auto app = my::new_application(argc, argv, {});

//     auto ev_bus = app->ev_bus();
//     auto async_task = app->async_task();
//     // auto resource_mgr = app->resource_mgr();
//     // auto audio_mgr = my::AudioMgr::create();
//     auto win_mgr = app->win_mgr();
//     // auto font_mgr = my::FontMgr::create();

//     auto url =
//         "/home/yydcnjjw/workspace/code/project/my-gui/assets/media/10.mp4";

//     auto win = win_mgr->create_window("test", 1280, 720);
//     // LLGL::Log::SetReportCallbackStd();
//     // auto renderer = LLGL::RenderSystem::Load("Vulkan");
//     // my::Canvas canvas(renderer.get(), win, ev_bus.get(),
//     resource_mgr.get(),
//     //                   font_mgr.get());

//     auto player = my::av::AVPlayer(url, async_task);

//     struct PaintEvent {
//         std::shared_ptr<SkBitmap> render_bitmap{};
//         PaintEvent(std::shared_ptr<SkBitmap> render_bitmap)
//             : render_bitmap(render_bitmap) {}
//     };

//     ev_bus->on_event<PaintEvent>()
//         .observe_on(ev_bus->ev_bus_worker())
//         .subscribe([win](const auto &e) {
//             auto render_bitmap = e->data->render_bitmap;
//             auto canvas = win->get_sk_surface()->getCanvas();
//             canvas->clear(SkColors::kWhite.toSkColor());
//             if (render_bitmap) {
//                 canvas->drawBitmap(*render_bitmap, 0, 0);
//             }
//             canvas->flush();
//             win->swap_window();
//         });

//     player.set_render_cb([ev_bus](std::shared_ptr<SkBitmap> bitmap) {
//         ev_bus->post<PaintEvent>(bitmap);
//     });

//     ev_bus->on_event<my::WindowEvent>()
//         .subscribe_on(ev_bus->ev_bus_worker())
//         .observe_on(ev_bus->ev_bus_worker())
//         .subscribe([&](const std::shared_ptr<my::Event<my::WindowEvent>> &e)
//         {
//             if (e->data->event == SDL_WINDOWEVENT_CLOSE) {
//                 ev_bus->post<my::QuitEvent>();
//             }
//         });

//     app->run();
// }

// void test2() {
//     rxcpp::schedulers::run_loop rlp;
//     auto rlp_worker = rxcpp::observe_on_run_loop(rlp);

//     rxcpp::subjects::subject<int> subject;

//     auto cs = subject
//                   .get_observable()
//                   // .subscribe_on(rlp_worker)
//                   .observe_on(rlp_worker)
//                   .subscribe([](int i) { std::cout << i << std::endl; });

//     subject.get_subscriber().on_next(1);
//     subject.get_subscriber().on_next(2);

//     while (!rlp.empty() && rlp.peek().when < rlp.now()) {
//         rlp.dispatch();
//     }
// }

// void test3(int argc, char *argv[]) {
//     auto image =
//         my::Image::make(my::Blob::make(my::ResourceStreamProvideInfo::make(
//             my::ResourceFileProvideInfo{argv[1]})));

//     auto data = image->sk_image()->encodeToData();
//     std::ofstream ofs("test.png");
//     ofs.write(reinterpret_cast<const char *>(data->data()), data->size());

//     image->export_png("test_export.png");
//     image->export_bmp24("test_export.bmp");
//     Logger::get()->close();
// }

// void test_multi_window(int argc, char *argv[]) {
//     auto app = my::new_application(argc, argv, {});
//     auto ev_bus = app->ev_bus();

//     auto win_mgr = app->win_mgr();

//     auto win1 = win_mgr->create_window("test1", 200, 200);
//     auto win2 = win_mgr->create_window("test2", 200, 200);

//     auto c1 = [&]() { return win1->get_sk_surface()->getCanvas(); };
//     auto c2 = [&]() { return win2->get_sk_surface()->getCanvas(); };

//     auto canvas1 = c1;
//     auto canvas2 = c2();

//     canvas1()->clear(SkColors::kBlue.toSkColor());
//     canvas1()->flush();
//     win1->swap_window();

//     // canvas2->clear(SkColors::kRed.toSkColor());
//     // canvas2->flush();
//     // win2->swap_window();

//     SkPaint paint;
//     paint.setColor(SkColors::kGreen);
//     canvas1()->drawIRect(my::IRect::MakeWH(100, 100), paint);
//     canvas1()->flush();
//     win1->swap_window();

//     // c1()->clear(SkColors::kBlue.toSkColor());
//     // c1()->flush();
//     // win1->swap_window();

//     // c2()->clear(SkColors::kRed.toSkColor());
//     // c2()->flush();
//     // win2->swap_window();

//     // SkPaint paint;
//     // paint.setColor(SkColors::kGreen);
//     // c1()->drawIRect(my::IRect::MakeWH(100, 100), paint);
//     // c1()->flush();
//     // win1->swap_window();

//     ev_bus->on_event<my::WindowEvent>()
//         .subscribe_on(ev_bus->ev_bus_worker())
//         .observe_on(ev_bus->ev_bus_worker())
//         .subscribe([&](const std::shared_ptr<my::Event<my::WindowEvent>> &e)
//         {
//             if (e->data->event == SDL_WINDOWEVENT_CLOSE) {
//                 ev_bus->post<my::QuitEvent>();
//             }
//         });
//     app->run();
// }
#include <application.h>
#include <boost/format.hpp>
#include <util/logger.h>

void set_current_thread_name(const std::string &name) {
    pthread_setname_np(pthread_self(), name.c_str());
}

std::string current_thread_name() {
    std::string s(32, 0);
    pthread_getname_np(pthread_self(), s.data(), s.size());
    return s;
}

std::string get_pid() {
    std::stringstream s;
    s << std::this_thread::get_id();
    return s.str();
}

int main(int argc, char **argv) {
    auto app = my::Application::create(argc, argv);

    rxcpp::observable<>::timer(std::chrono::seconds(3))
        .subscribe_on(app->coordination())
        .subscribe([&app](auto) {
            GLOG_I("need exit!");
            app->post<my::QuitEvent>();
        });

    app->run();
    GLOG_I("exit!");
    std::this_thread::sleep_for(std::chrono::seconds(2));
}
