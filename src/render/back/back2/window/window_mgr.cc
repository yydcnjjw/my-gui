#include "window_mgr.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

namespace {
static const int stencil_bits{8};      // Skia needs 8 stencil bits
static const int msaa_sample_count{0}; // 4;

struct WindowPaint {
    my::WindowID win_id;
    WindowPaint(my::WindowID win_id) : win_id(win_id) {}
};

class SDLWindow : public my::window {
  public:
    class SDLSurface : public LLGL::Surface {
      public:
        SDLSurface(SDLWindow *win) : _win(win) {}

        bool GetNativeHandle(void *nativeHandle, std::size_t) const override {
            auto handle = reinterpret_cast<LLGL::NativeHandle *>(nativeHandle);
            SDL_SysWMinfo info;
            SDL_VERSION(&info.version);
            if (!::SDL_GetWindowWMInfo(this->_win->_sdl_window, &info)) {
                throw std::runtime_error(SDL_GetError());
            }
            handle->display = info.info.x11.display;
            handle->window = info.info.x11.window;

            return true;
        };

        LLGL::Extent2D GetContentSize() const override {
            auto size = this->_win->get_frame_buffer_size();
            return {SkToU32(size.width()), SkToU32(size.height())};
        };

        bool
        AdaptForVideoMode(LLGL::VideoModeDescriptor &videoModeDesc) override {
            this->_win->set_size({SkToInt(videoModeDesc.resolution.width),
                                  SkToInt(videoModeDesc.resolution.height)});
            this->_win->set_full_screen(videoModeDesc.fullscreen);
            return true;
        }

        void ResetPixelFormat() override {}

        bool ProcessEvents() override { return true; }

        std::unique_ptr<LLGL::Display> FindResidentDisplay() const override {
            return std::move(LLGL::Display::InstantiateList().front());
        };

      private:
        SDLWindow *_win;
    };

    SDLWindow(SDL_Window *win)
        : _sdl_window(win), _win_id(SDL_GetWindowID(this->_sdl_window))
    // ,_surface(std::make_shared<SDLSurface>(this))
    {}

    ~SDLWindow() {
        this->_paint_cs.unsubscribe();
        if (this->_gl_context) {
            ::SDL_GL_DeleteContext(this->_gl_context);
            this->_gl_context = nullptr;
        }

        ::SDL_DestroyWindow(this->_sdl_window);
    }

    std::shared_ptr<LLGL::Surface> get_surface() override {
        return this->_surface;
    };

    sk_sp<SkSurface> get_sk_surface() override {
        auto [w, h] = this->get_frame_buffer_size();
        if (!this->_sk_surface || this->_sk_surface->width() != w ||
            this->_sk_surface->height() != h) {
            this->_sk_surface = this->_make_sk_surface();
        }

        this->set_gl_context();

        assert(std::this_thread::get_id() == this->_thread_id);
        return this->_sk_surface;
    }

    void swap_window() override {
        ::SDL_GL_SwapWindow(this->_sdl_window);
        // this->_ev_bus->post<WindowPaint>(this->get_window_id());
    }

    my::ISize2D get_frame_buffer_size() override {
        // TODO: gl vulkan
        int w, h;
        // SDL_Vulkan_GetDrawableSize(this->_sdl_window, &w, &h);
        ::SDL_GL_GetDrawableSize(this->_sdl_window, &w, &h);
        return my::ISize2D::Make(w, h);
    }

    void set_frame_buffer_size(const my::ISize2D &size) override {
        this->set_size(size);
    }

    my::ISize2D get_min_size() override {
        int w, h;
        ::SDL_GetWindowMinimumSize(this->_sdl_window, &w, &h);
        return my::ISize2D::Make(w, h);
    }
    my::ISize2D get_max_size() override {
        int w, h;
        ::SDL_GetWindowMaximumSize(this->_sdl_window, &w, &h);
        return my::ISize2D::Make(w, h);
    }

    void set_min_size(const my::ISize2D &size) override {
        ::SDL_SetWindowMinimumSize(this->_sdl_window, size.width(),
                                   size.height());
    }
    void set_max_size(const my::ISize2D &size) override {
        ::SDL_SetWindowMaximumSize(this->_sdl_window, size.width(),
                                   size.height());
    }

    my::ISize2D get_size() override {
        int w, h;
        ::SDL_GetWindowSize(this->_sdl_window, &w, &h);
        return my::ISize2D::Make(w, h);
    }

    void set_size(const my::ISize2D &size) override {
        ::SDL_SetWindowSize(this->_sdl_window, size.width(), size.height());
    }
    my::IPoint2D get_pos() override {
        int x, y;
        ::SDL_GetWindowPosition(this->_sdl_window, &x, &y);
        return {x, y};
    }
    void set_pos(const my::IPoint2D &pos) override {
        ::SDL_SetWindowPosition(this->_sdl_window, pos.x(), pos.y());
    }

    virtual void set_full_screen(bool full_screen) override {
        ::SDL_SetWindowFullscreen(this->_sdl_window,
                                  full_screen ? SDL_WINDOW_FULLSCREEN : 0);
    }

    my::WindowID get_window_id() override { return this->_win_id; }

    void hide() override { this->set_visible(false); };
    void show() override { this->set_visible(true); }

    void set_visible(bool visible) override {
        if (visible == this->_is_visible) {
            return;
        }
        if (visible) {
            ::SDL_ShowWindow(this->_sdl_window);
        } else {
            ::SDL_HideWindow(this->_sdl_window);
        }
        this->_is_visible = visible;
    }

    bool is_visible() override { return this->_is_visible; }

    void show_cursor(bool toggle) override {
        ::SDL_ShowCursor(toggle ? SDL_ENABLE : SDL_DISABLE);
    }

    void set_mouse_pos(const my::IPoint2D &pos) override {
        ::SDL_WarpMouseInWindow(this->_sdl_window, pos.x(), pos.y());
    }

  private:
    SDL_Window *_sdl_window{};
    my::WindowID _win_id{};
    std::shared_ptr<SDLSurface> _surface{};
    sk_sp<SkSurface> _sk_surface{};

    bool _is_visible{true};

    SDL_GLContext _gl_context{};
    std::thread::id _thread_id;

    rxcpp::composite_subscription _paint_cs;

    void set_gl_context() {
        if (!this->_gl_context) {
            return;
        }
        auto success =
            ::SDL_GL_MakeCurrent(this->_sdl_window, this->_gl_context);
        if (success != 0) {
            throw std::runtime_error(SDL_GetError());
        }
    }

    void _build_gl_context(SDL_Window *window) {
        this->_gl_context = ::SDL_GL_CreateContext(window);
        if (!this->_gl_context) {
            throw std::runtime_error(SDL_GetError());
        }

        // set_gl_context();

        this->_thread_id = std::this_thread::get_id();
    }

    sk_sp<SkSurface> _make_sk_surface() {
        if (!this->_gl_context) {
            this->_build_gl_context(this->_sdl_window);
        }

        auto [w, h] = this->get_frame_buffer_size();

        ::glViewport(0, 0, w, h);
        ::glClearColor(1, 1, 1, 1);
        ::glClearStencil(0);
        ::glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // setup GrContext
        auto interface = GrGLMakeNativeInterface();

        // setup contexts
        sk_sp<GrContext> gr_context{GrContext::MakeGL(interface)};

        GrGLFramebufferInfo info{};
        {
            GLint fbo{};
            ::glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
            if (::glGetError() != GL_NO_ERROR) {
                throw std::runtime_error("framebuffer binding error");
            }
            info.fFBOID = fbo;
            info.fFormat = GL_RGBA8;
        }

        SkColorType color_type{kRGBA_8888_SkColorType};

        GrBackendRenderTarget target{w, h, msaa_sample_count, stencil_bits,
                                     info};

        // setup SkSurface
        // To use distance field text, use commented out SkSurfaceProps instead
        // SkSurfaceProps props(SkSurfaceProps::kUseDeviceIndependentFonts_Flag,
        //                      SkSurfaceProps::kLegacyFontHost_InitType);
        SkSurfaceProps props{SkSurfaceProps::kLegacyFontHost_InitType};

        return SkSurface::MakeFromBackendRenderTarget(
            gr_context.get(), target, kBottomLeft_GrSurfaceOrigin, color_type,
            nullptr, &props);
    }
};

class SDLWindowMgr : public my::WindowMgr {

  public:
    SDLWindowMgr(my::EventBus *bus) : _ev_bus(bus) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO) != 0) {
            throw std::runtime_error(SDL_GetError());
        }
        this->_init_gl();
        this->_init_desktop_display_mode();
        bus->on_event<my::QuitEvent>()
            .observe_on(bus->ev_bus_worker())
            .subscribe([this](const auto &) { this->_is_exit = true; });
        this->_win_mgr_thread = std::thread([this, bus]() {
            pthread_setname_np(pthread_self(), "win mgr");
            GLOG_D("window loop running!");

            while (!this->_is_exit) {
                SDL_Event sdl_event{};
                SDL_WaitEventTimeout(&sdl_event, 200);
                switch (sdl_event.type) {
                case SDL_QUIT:
                    // bus->post<my::QuitEvent>();
                    // this->_is_exit = true;
                    break;
                case SDL_MOUSEMOTION:
                    bus->post<my::MouseMotionEvent>(
                        sdl_event.motion.windowID,
                        my::IPoint2D::Make(sdl_event.motion.x,
                                           sdl_event.motion.y),
                        my::ISize2D::Make(sdl_event.motion.xrel,
                                          sdl_event.motion.yrel),
                        sdl_event.motion.state);
                    break;
                case SDL_MOUSEWHEEL:
                    bus->post<my::MoushWheelEvent>(
                        sdl_event.wheel.windowID,
                        my::IPoint2D{sdl_event.wheel.x, sdl_event.wheel.y},
                        sdl_event.wheel.which, sdl_event.wheel.direction);
                    break;
                case SDL_KEYUP:
                case SDL_KEYDOWN:
                    bus->post<my::KeyboardEvent>(
                        sdl_event.key.windowID, sdl_event.key.state,
                        sdl_event.key.repeat != 0, sdl_event.key.keysym);
                    break;
                case SDL_WINDOWEVENT:
                    bus->post<my::WindowEvent>(sdl_event.window.windowID,
                                               sdl_event.window.event);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    bus->post<my::MouseButtonEvent>(
                        sdl_event.button.windowID, sdl_event.button.which,
                        sdl_event.button.button, sdl_event.button.state,
                        sdl_event.button.clicks,
                        my::IPoint2D{sdl_event.button.x, sdl_event.button.y});
                    break;
                default:
                    break;
                }
            }
            GLOG_D("window loop exit!");
        });
    }

    ~SDLWindowMgr() {
        if (!this->_is_exit) {
            this->_is_exit = true;
        }
        if (this->_win_mgr_thread.joinable()) {
            this->_win_mgr_thread.join();
        }
        SDL_Quit();
    }
    my::window *create_window(const std::string &title, uint32_t w,
                              uint32_t h) override {
        // TODO: 考虑 继承
        SDL_Window *sdl_win =
            SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, w, h,
                             SDL_WINDOW_UTILITY | SDL_WINDOW_ALLOW_HIGHDPI |
                                 SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
                             // | SDL_WINDOW_VULKAN
            );

        if (sdl_win == nullptr) {
            throw std::runtime_error(SDL_GetError());
        }
        auto win = std::make_unique<SDLWindow>(sdl_win);
        auto id = win->get_window_id();
        GLOG_D("----------create window %d %s", id, title.c_str());
        this->_windows[id] = std::move(win);
        return this->_windows.at(id).get();
    }

    void remove_window(my::window *win) override {
        GLOG_D("----------remove window %d", win->get_window_id());
        this->_windows.erase(win->get_window_id());
    }

    const my::DisplayMode &get_desktop_display_mode() override {
        return this->_desktop_display_mode;
    }

    my::Keymod get_key_mode() override {
        return static_cast<my::Keymod>(SDL_GetModState());
    }

    my::MouseState get_mouse_state() override {
        int x, y;
        auto mask = SDL_GetMouseState(&x, &y);
        return my::MouseState(mask, {x, y});
    }

    bool has_window(my::WindowID id) override {
        return this->_windows.find(id) != this->_windows.end();
    }

  private:
    std::map<my::window_id, std::unique_ptr<SDLWindow>> _windows;
    my::EventBus *_ev_bus{};
    std::thread _win_mgr_thread;
    my::DisplayMode _desktop_display_mode;
    std::atomic_bool _is_exit = false;
    void _init_desktop_display_mode() {
        SDL_DisplayMode mode;
        if (SDL_GetDesktopDisplayMode(0, &mode) != 0) {
            throw std::runtime_error(SDL_GetError());
        }
        this->_desktop_display_mode = {
            mode.format, {mode.w, mode.h}, mode.refresh_rate};
    }

    void _init_gl() {
        ::SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        ::SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        ::SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                              SDL_GL_CONTEXT_PROFILE_CORE);

        ::SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        ::SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        ::SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        ::SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        ::SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

        ::SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencil_bits);

        ::SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

        // If you want multisampling, uncomment the below lines and set a sample
        // count
        // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, kMsaaSampleCount);
    }
};

} // namespace

namespace my {
std::unique_ptr<WindowMgr> WindowMgr::create(EventBus *bus) {
    return std::make_unique<SDLWindowMgr>(bus);
}

} // namespace my
