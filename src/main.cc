#include <application.hpp>
#include <render/node.hpp>
#include <render/render_service.hpp>
#include <storage/image.hpp>

namespace my {} // namespace my

int main(int argc, char **argv) {
    // try {
    GLOG_D("application run");
    auto app = my::Application::create(argc, argv);

    auto win = app->service<my::WindowService>()
                   ->create_window("my gui", {512, 512})
                   .get();
    auto render = my::RenderService::make(win);

    render->run_at([](my::RenderService *render) {
        auto root = my::Node2D::make(my::ISize2D::Make(400, 400));
        SkPaint paint;
        {
            paint.setColor(SkColors::kBlue);
            root->canvas()->drawIRect(my::IRect::MakeXYWH(0, 0, 400, 400),
                                      paint);
        }
        {
            auto sub_n =
                my::Node2D::make(my::IRect::MakeXYWH(100, 100, 512, 512));

            sub_n->canvas()->clear(SkColors::kGray);
            paint.setColor(SkColors::kRed);
            sub_n->canvas()->drawIRect(my::IRect::MakeWH(200, 200), paint);

            root->add_sub_node(sub_n);
        }
        render->root_node(root);
    });

    app->run();
    GLOG_D("application exit!");
    // } catch (std::exception &e) {
    //     std::cout << e.what() << std::endl;
    // }
}
