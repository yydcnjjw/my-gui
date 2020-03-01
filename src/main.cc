#include <iostream>
#include <memory>

#include "logger.h"

// class App {
//   public:
//     App() : _log(std::make_shared<Logger>()) { init(); }

//     void Run() const {}

//   private:
//     std::shared_ptr<Logger> _log;
//     std::shared_ptr<WindowMgr> _window_mgr;

//     void init() const {}
// };

#include "camera.hpp"
#include "vulkan_ctx.h"
#include "window_mgr.h"

#include <SDL2/SDL.h>
int main(int argc, char *argv[]) {
    // auto app = std::make_shared<App>();
    // app->Run();

    try {
        auto win_mgr = my::get_window_mgr();

        auto win = win_mgr->create_window("Test", 800, 600);
        auto ctx = my::make_vulkan_ctx(win);

        bool is_quit = false;
        win_mgr->get_observable()
            .filter([](const std::shared_ptr<my::Event> &e) {
                return e->type == my::EventType::EVENT_QUIT;
            })
            .subscribe([&is_quit](const std::shared_ptr<my::Event> &e) {
                GLOG_I("application quit!");
                is_quit = true;
            });

        auto draw_thread = std::thread([&]() {
            GLOG_I("draw begin ...");

            while (!is_quit) {
                ctx->draw();
            }
            GLOG_I("draw end ...");
        });

        // while (!is_quit) {
        //     float a, b, c;
        //     ::scanf("%f %f %f", &a, &b, &c);
        //     ctx->set_view(a, b, c);
        //     GLOG_I("%f %f %f\n", a, b, c);
        // }

        if (draw_thread.joinable()) {
            draw_thread.join();
        }

    } catch (std::exception &e) {
        GLOG_E(e.what());
    }
    return 0;
}
