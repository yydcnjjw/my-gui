#include <application.hpp>
#include <render/node.hpp>
#include <render/render_service.hpp>
#include <storage/image.hpp>

namespace my {} // namespace my
using namespace my;
int main(int argc, char **argv) {
    // try {
    GLOG_D("application run");
    auto app = Application::create(argc, argv);

    auto win = app->service<WindowService>()
                   ->create_window("my gui", {512, 512})
                   .get();

    auto root = Node2D::make(ISize2D::Make(400, 400));
    {

        SkPaint paint;
        {
            paint.setColor(SkColors::kBlue);
            root->canvas()->drawIRect(IRect::MakeXYWH(0, 0, 400, 400), paint);
        }
        {
            auto sub_n = Node2D::make(IRect::MakeXYWH(100, 100, 512, 512));

            sub_n->canvas()->clear(SkColors::kGray);
            paint.setColor(SkColors::kRed);
            sub_n->canvas()->drawIRect(IRect::MakeWH(200, 200), paint);

            root->add_sub_node(sub_n);
        }
    }

    on_event<MouseButtonEvent>(win.get()).subscribe(
        [](shared_ptr<Event<MouseButtonEvent>> e) { e->pos; });

    auto render = RenderService::make(win);

    render->run_at([root](RenderService *render) { render->root_node(root); });

    my::Scene scene;
    auto root = scene.set_head(my::Node::make());
    auto i = scene.append_child(root, my::Node::make());

    app->run();
    GLOG_D("application exit!");
    // } catch (std::exception &e) {
    //     std::cout << e.what() << std::endl;
    // }
}
