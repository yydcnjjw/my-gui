#include <application.hpp>
#include <render/render.hpp>

namespace my {} // namespace my
using namespace my;
int main(int argc, char **argv) {
    // try {
    GLOG_D("application run");
    auto app = Application::create(argc, argv);

    auto win = app->service<WindowService>()
                   ->create_window("my gui", {512, 512})
                   .get();

    auto root = RootNode2D::make_root(ISize2D::Make(400, 400));
    {
        SkPaint paint;
        {
            paint.setColor(SkColors::kBlue);
            root->canvas().drawIRect(IRect::MakeXYWH(0, 0, 400, 400), paint);
        }
        {
            auto sub_n = Node2D::make(IRect::MakeXYWH(100, 100, 512, 512));

            sub_n->bind_event<MouseButtonEvent>(
                [](observable_event_type<MouseButtonEvent> o) {
                    return o.subscribe([](event_type<MouseButtonEvent> e) {
                        GLOG_D("%d %d", (*e)->pos().x(), (*e)->pos().y());
                    });
                });
            sub_n->canvas().clear(SkColors::kGray);
            paint.setColor(SkColors::kRed);
            sub_n->canvas().drawIRect(IRect::MakeWH(200, 200), paint);
            root->add_sub_node(sub_n);
        }
    }

    auto render = RenderService::make(win);

    render->run_at([root](RenderService *render) { render->root_node(root); });

    root->subscribe(win.get());

    app->run();
    GLOG_D("application exit!");
    // } catch (std::exception &e) {
    //     std::cout << e.what() << std::endl;
    // }
}
