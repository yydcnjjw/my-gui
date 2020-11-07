#include "render_service.hpp"

#include <stack>

#include <GL/gl.h>

#include <skia/include/core/SkSurface.h>
#include <skia/include/gpu/GrBackendSurface.h>
#include <skia/include/gpu/GrDirectContext.h>
#include <skia/include/gpu/gl/GrGLInterface.h>

#include <render/exception.hpp>

namespace {
using namespace my;

class SDLRenderService : public RenderService {
  public:
    SDLRenderService(shared_ptr<Window> win)
        : _win(win), _surface(this->create_sk_surface().get()) {
        this->run();
    }

    ~SDLRenderService() {
        this->exit([]() {}, [this]() { ::SDL_GL_DeleteContext(this->_glctx); })
            .get();
    }

    void draw_node2D(shared_ptr<Node2D> node) {
        auto parent = node->parent();
        auto region = parent ? parent->region() : this->root_node()->region();
        auto [l, t] = node->pos();
        auto snapshot = node->snapshot();

        this->canvas()->save();
        this->canvas()->clipRect(region);

        this->canvas()->drawImage(snapshot, l, t);

        this->canvas()->translate(l, t);
        for (auto sub : node->sub_nodes()) {
            draw_node2D(sub);
        }
        this->canvas()->restore();
    }

    void draw_root() {
        if (this->root_node()) {
            this->draw_node2D(this->root_node());
        }
    }

    void run() {
        rx::observable<>::interval(std::chrono::milliseconds(1000 / 60),
                                   this->coordination().get())
            .subscribe([this](auto) {
                this->canvas()->clear(SK_ColorWHITE);

                this->draw_root();

                this->canvas()->flush();
                SDL_GL_SwapWindow(this->native_instance());
            });
    }

    SkCanvas *canvas() override { return this->_surface->getCanvas(); }

    future<sk_sp<SkSurface>> create_sk_surface() {
        return this->schedule<sk_sp<SkSurface>>([this]() {
            static const int stencil_bits = 8;
            static const int msaa_sample_count = 0;

            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                                SDL_GL_CONTEXT_PROFILE_CORE);
            SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

            SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencil_bits);

            SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

            this->_glctx = ::SDL_GL_CreateContext(this->native_instance());
            if (!this->_glctx) {
                throw RenderServiceError("create gl context failure");
            }

            if (SDL_GL_MakeCurrent(this->native_instance(), this->_glctx)) {
                throw RenderServiceError("gl make current failure");
            }

            int w, h;
            SDL_GL_GetDrawableSize(this->native_instance(), &w, &h);

            glViewport(0, 0, w, h);
            glClearColor(1, 1, 1, 1);
            glClearStencil(0);
            glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            auto interface = GrGLMakeNativeInterface();

            // setup contexts
            sk_sp<GrDirectContext> gr_ctx(GrDirectContext::MakeGL(interface));

            GrGLFramebufferInfo info{};
            {
                GLint fbo{};
                ::glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
                if (::glGetError() != GL_NO_ERROR) {
                    throw RenderServiceError("framebuffer binding error");
                }
                info.fFBOID = fbo;
                info.fFormat = GL_RGBA8;
            }

            SkColorType color_type{kRGBA_8888_SkColorType};

            GrBackendRenderTarget target{w, h, msaa_sample_count, stencil_bits,
                                         info};

            SkSurfaceProps props;

            return SkSurface::MakeFromBackendRenderTarget(
                gr_ctx.get(), target, kBottomLeft_GrSurfaceOrigin, color_type,
                nullptr, &props);
        });
    }

  private:
    shared_ptr<Window> _win;
    SDL_GLContext _glctx;
    sk_sp<SkSurface> _surface;

    SDL_Window *native_instance() {
        return ::SDL_GetWindowFromID(this->_win->window_id());
    }
};

} // namespace

namespace my {
unique_ptr<RenderService> RenderService::make(shared_ptr<Window> win) {
    return std::make_unique<SDLRenderService>(win);
}
} // namespace my
