#include "render_server.h"

#include "vulkan_ctx.h"

namespace {

class MyRenderServer : public my::RenderServer {
  public:
    MyRenderServer(my::EventBus *bus, my::Window *win) : _window(win) {}

    ~MyRenderServer() override {}

    void run() override {
        std::thread([this]() {
            auto ctx = my::VulkanCtx::create(this->_window);
            for (;;) {
                auto start = std::chrono::high_resolution_clock::now();
                ctx->prepare_buffer();

                ctx->swap_buffer();
                auto end = std::chrono::high_resolution_clock::now();
            }
        });
    }

  private:
    my::Window *_window;
};
} // namespace

namespace my {
static std::unique_ptr<RenderServer> create(EventBus *bus, Window *win) {
    return std::make_unique<MyRenderServer>(bus, win);
}
} // namespace my
