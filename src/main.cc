#include <LLGL/LLGL.h>
#include <LLGL/Log.h>
#include <boost/timer/timer.hpp>
#include <media/audio_mgr.h>
#include <render/canvas.h>
#include <render/window/window_mgr.h>
#include <storage/resource_mgr.hpp>
#include <util/async_task.hpp>
#include <util/event_bus.hpp>

#include <util/logger.h>
#include <util/queue.hpp>

#include <boost/timer/timer.hpp>

#include <iostream>
#include <stdexcept>
// #include <rx.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#ifdef __cplusplus

char av_error[AV_ERROR_MAX_STRING_SIZE] = {0};
#undef av_err2str
#define av_err2str(errnum)                                                     \
    av_make_error_string(av_error, AV_ERROR_MAX_STRING_SIZE, errnum)

#endif // __cplusplus
namespace my {
using boost::timer::cpu_timer;
namespace av {

class Frame {
  public:
    Frame() {
        this->_frame = ::av_frame_alloc();
        if (!this->_frame) {
            throw std::runtime_error("frame alloc failure");
        }
    }

    virtual ~Frame() { ::av_frame_free(&this->_frame); }

    auto data() const { return this->_frame->data; }

    auto line_size() const { return this->_frame->linesize; }

    auto pts(const AVRational &time_base) const {
        return static_cast<int>(this->_frame->pts * ::av_q2d(time_base) * 1000);
    }

  protected:
    AVFrame *_frame;
    friend class DecCtx;
};
class VideoFrame : public Frame {
  public:
    VideoFrame() : Frame() {}
    auto w() const { return this->_frame->width; }
    auto h() const { return this->_frame->height; }
};

class AudioFrame : public Frame {
  public:
    AudioFrame() : Frame() {}
    auto nb_samples() const { return this->_frame->nb_samples; }
};

class DecCtx {
  public:
    DecCtx(AVFormatContext *fmt_ctx, AVMediaType type) {
        int ret{};
        AVCodec *dec{};
        if ((ret = ::av_find_best_stream(fmt_ctx, type, -1, -1, &dec, 0)) < 0) {
            throw std::runtime_error(av_err2str(ret));
        }
        int stream_idx = ret;
        auto st = fmt_ctx->streams[stream_idx];
        auto dec_ctx = ::avcodec_alloc_context3(dec);
        if (!dec_ctx) {
            throw std::runtime_error("avcodec context alloc failure");
        }

        if ((ret = ::avcodec_parameters_to_context(dec_ctx, st->codecpar)) <
            0) {
            throw std::runtime_error(av_err2str(ret));
        }

        if ((ret = avcodec_open2(dec_ctx, dec, nullptr)) < 0) {
            throw std::runtime_error(av_err2str(ret));
        }
        this->_dec_ctx = dec_ctx;
        this->_st = st;
        this->st_idx = stream_idx;
    }
    ~DecCtx() { ::avcodec_free_context(&_dec_ctx); }

    AVRational &time_base() const { return this->_st->time_base; }

    int decode(AVPacket *pkt, const std::shared_ptr<Frame> &frame) {
        int ret{0};
        if ((ret = avcodec_send_packet(this->_dec_ctx, pkt)) < 0) {
            throw std::runtime_error(av_err2str(ret));
        }

        if ((ret = avcodec_receive_frame(this->_dec_ctx, frame->_frame)) < 0) {
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                return ret;
            throw std::runtime_error(av_err2str(ret));
        }
        return ret;
    }
    int st_idx;

  protected:
    AVCodecContext *_dec_ctx;
    AVStream *_st;
};

class VideoDecCtx : public DecCtx {
  public:
    VideoDecCtx(AVFormatContext *fmt_ctx)
        : DecCtx(fmt_ctx, AVMEDIA_TYPE_VIDEO) {
        int w = this->w();
        int h = this->h();
        auto format = AV_PIX_FMT_RGBA;
        this->_sws_ctx =
            ::sws_getContext(w, h, this->format(), w, h, format, SWS_BILINEAR,
                             nullptr, nullptr, nullptr);
        if (!this->_sws_ctx) {
            throw std::runtime_error("sws ctx create failure");
        }

        int ret{0};
        if ((ret = ::av_image_alloc(this->_data, this->_linesize, w, h, format,
                                    1)) < 0) {
            throw std::runtime_error(av_err2str(ret));
        }
        assert(ret == w * h * 4);
        this->_image_cache = std::make_shared<my::Image>(this->_data[0], w, h);
    }

    ~VideoDecCtx() {
        ::av_freep(&this->_data[0]);
        ::sws_freeContext(this->_sws_ctx);
    }

    int w() const { return this->_st->codecpar->width; }
    int h() const { return this->_st->codecpar->height; }

    AVPixelFormat format() const {
        return (AVPixelFormat)this->_st->codecpar->format;
    }

    std::shared_ptr<my::Image>
    convert_cached(const std::shared_ptr<VideoFrame> &frame) {
        ::sws_scale(this->_sws_ctx, frame->data(), frame->line_size(), 0,
                    frame->h(), this->_data, this->_linesize);
        return _image_cache;
    }

  private:
    SwsContext *_sws_ctx;
    std::shared_ptr<Image> _image_cache;
    uint8_t *_data[4]{};
    int _linesize[4]{};
};

class AudioDecCtx : public DecCtx {
  public:
    auto sample_rate() const { return this->_dec_ctx->sample_rate; }

    auto format() const { return this->_dec_ctx->sample_fmt; }

    auto channel_layout() const { return this->_dec_ctx->channel_layout; }

    auto channels() const { return this->_dec_ctx->channels; }

    auto frame_size() const { return this->_dec_ctx->frame_size; }

    AudioDecCtx(AVFormatContext *fmt_ctx)
        : DecCtx(fmt_ctx, AVMEDIA_TYPE_AUDIO) {

        auto dst_sample_fmt = AV_SAMPLE_FMT_S16;
        auto dst_sample_rate = this->sample_rate();
        auto dst_channels = 2;
        auto dst_channel_layout = ::av_get_default_channel_layout(dst_channels);
        this->_nb_samples = this->frame_size();
        this->_swr_ctx = ::swr_alloc_set_opts(
            nullptr, dst_channel_layout, dst_sample_fmt, dst_sample_rate,
            dst_channel_layout, this->format(), dst_sample_rate, 0, nullptr);
        ::swr_init(this->_swr_ctx);

        int dst_sample_linesize{};
        int ret{0};
        if ((ret = ::av_samples_alloc_array_and_samples(
                 &this->_sample, &dst_sample_linesize, dst_channels,
                 this->_nb_samples, dst_sample_fmt, 0)) < 0) {
            throw std::runtime_error(av_err2str(ret));
        }
        if ((ret = av_samples_get_buffer_size(nullptr, dst_channels,
                                              this->_nb_samples, dst_sample_fmt,
                                              1)) < 0) {
            throw std::runtime_error(av_err2str(ret));
        }
        this->_size = ret;
    }

    ~AudioDecCtx() {
        av_freep(&this->_sample[0]);
        ::swr_free(&this->_swr_ctx);
    }

    struct SampleBuffer {
        void *sample;
        size_t size;
    };
    SampleBuffer convert_cached(const std::shared_ptr<AudioFrame> &frame) {
        auto ret =
            ::swr_convert(this->_swr_ctx, this->_sample, this->_nb_samples,
                          (const uint8_t **)frame->data(), frame->nb_samples());
        if (ret < 0) {
            throw std::runtime_error(av_err2str(ret));
        }
        return {this->_sample[0], this->_size};
    }

  private:
    SwrContext *_swr_ctx{};
    uint8_t **_sample{};
    int _nb_samples{};
    size_t _size{};
};

class AVPlayer {
  public:
    AVPlayer(const std::string &url, Canvas *canvas, AsyncTask *async_task)
        : _canvas(canvas), _video_queue(std::make_shared<Queue_t>()),
          _audio_queue(std::make_shared<Queue_t>()) {
        int ret{};
        if ((ret = ::avformat_open_input(&this->_fmt_ctx, url.c_str(), nullptr,
                                         nullptr))) {
            throw std::runtime_error(av_err2str(ret));
        }

        if ((ret = ::avformat_find_stream_info(this->_fmt_ctx, nullptr))) {
            throw std::runtime_error(av_err2str(ret));
        }

#ifdef MY_DEBUG
        ::av_dump_format(this->_fmt_ctx, 0, url.c_str(), 0);
#endif // MY_DEBUG

        this->_video_dec_ctx = std::make_shared<VideoDecCtx>(this->_fmt_ctx);
        this->_audio_dec_ctx = std::make_shared<AudioDecCtx>(this->_fmt_ctx);

        SDL_AudioSpec want_audio{}, audio{};
        want_audio.freq = this->_audio_dec_ctx->sample_rate();
        want_audio.format = AUDIO_S16SYS;
        want_audio.channels = 2;
        want_audio.samples = this->_audio_dec_ctx->frame_size();
        this->_audio_device = SDL_OpenAudioDevice(
            nullptr, 0, &want_audio, &audio,
            SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
        // GLOG_D("freq %d channels %d, format %d", audio.freq, au);
        ::SDL_PauseAudioDevice(this->_audio_device, false);

        this->_read_frame_thread = std::thread([this]() {
            pthread_setname_np(pthread_self(), "read frame");
            this->_read_frame_loop();
        });
        async_task->do_timer_for(
            [this](auto &, auto timer) { this->_handle_video_frame(timer); },
            std::chrono::milliseconds(0));
        async_task->do_timer_for(
            [this](auto &, auto timer) { this->_handle_audio_frame(timer); },
            std::chrono::milliseconds(0));
    }

    ~AVPlayer() {
        this->_is_exit = true;
        ::SDL_CloseAudioDevice(this->_audio_device);
        if (this->_read_frame_thread.joinable()) {
            this->_read_frame_thread.join();
        }
        ::avformat_close_input(&this->_fmt_ctx);
    }

  private:
    AVFormatContext *_fmt_ctx{};

    SDL_AudioDeviceID _audio_device;
    Canvas *_canvas;

    std::shared_ptr<VideoDecCtx> _video_dec_ctx;
    std::shared_ptr<AudioDecCtx> _audio_dec_ctx;

    typedef Queue<std::shared_ptr<Frame>, 15> Queue_t;
    std::shared_ptr<Queue_t> _video_queue;
    std::shared_ptr<Queue_t> _audio_queue;
    std::atomic_long _clock = -1;
    std::thread _read_frame_thread;
    std::atomic_bool _is_exit{false};

    void _handle_audio_frame(std::shared_ptr<boost::asio::steady_timer> timer) {
        cpu_timer cpu_timer;
        // if (this->_clock == -1) {
        //     this->_clock = 0;
        // }
        auto frame =
            std::static_pointer_cast<AudioFrame>(this->_audio_queue->pop());
        auto time_base = this->_audio_dec_ctx->time_base();
        auto buffer = this->_audio_dec_ctx->convert_cached(frame);
        if (::SDL_QueueAudio(this->_audio_device, buffer.sample, buffer.size) !=
            0) {
            throw std::runtime_error(::SDL_GetError());
        }
        auto next_frame =
            std::static_pointer_cast<AudioFrame>(this->_audio_queue->front());

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::nanoseconds(cpu_timer.elapsed().wall));
        auto pts = frame->pts(time_base);
        auto next_pts = next_frame->pts(time_base);

        auto wait_timer = next_pts - pts - elapsed.count();
        GLOG_D("handle audio frame pts %d, next_pts %d, wait_timer %d", pts,
               next_pts, wait_timer);
        if (wait_timer < 0) {
            wait_timer = 0;
        }
        timer->expires_after(std::chrono::milliseconds(wait_timer));
        timer->async_wait(boost::bind<void>(
            [this](auto &, auto timer) { this->_handle_audio_frame(timer); },
            boost::asio::placeholders::error, timer));
    }

    void _handle_video_frame(std::shared_ptr<boost::asio::steady_timer> timer) {
        cpu_timer cpu_timer;

        // while (this->_clock == -1)
            // ;

        auto frame =
            std::static_pointer_cast<VideoFrame>(this->_video_queue->pop());
        auto &time_base = this->_video_dec_ctx->time_base();
        auto image = this->_video_dec_ctx->convert_cached(frame);
        this->_canvas->draw_image(image, {0, 0}, {frame->w(), frame->h()});
        this->_canvas->render();

        auto next_frame =
            std::static_pointer_cast<VideoFrame>(this->_video_queue->front());

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::nanoseconds(cpu_timer.elapsed().wall));
        auto pts = frame->pts(time_base);
        auto next_pts = next_frame->pts(time_base);

        auto wait_timer = next_pts - pts - elapsed.count();
        GLOG_D("handle video frame: pts %d, next_pts %d, wait_timer %d", pts,
               next_pts, wait_timer);
        if (wait_timer < 0) {
            wait_timer = 0;
        }
        timer->expires_after(std::chrono::milliseconds(wait_timer));
        timer->async_wait(boost::bind<void>(
            [this](auto &, auto timer) { this->_handle_video_frame(timer); },
            boost::asio::placeholders::error, timer));
    }

    void _read_frame_loop() {
        auto pkt = ::av_packet_alloc();
        int ret{0};
        while (!this->_is_exit) {
            if ((ret = ::av_read_frame(this->_fmt_ctx, pkt)) < 0) {
                if (ret == AVERROR_EOF) {
                    break;
                } else {
                    throw std::runtime_error(av_err2str(ret));
                }
            }

            std::shared_ptr<DecCtx> dec_ctx{};
            std::shared_ptr<Queue_t> queue{};
            std::shared_ptr<Frame> frame{};
            if (pkt->stream_index == this->_video_dec_ctx->st_idx) {
                dec_ctx = this->_video_dec_ctx;
                queue = this->_video_queue;
                frame = std::make_shared<VideoFrame>();
            } else if (pkt->stream_index == this->_audio_dec_ctx->st_idx) {
                dec_ctx = this->_audio_dec_ctx;
                queue = this->_audio_queue;
                frame = std::make_shared<AudioFrame>();
            }

            if ((ret = dec_ctx->decode(pkt, frame)) < 0) {
                if (ret == AVERROR(EAGAIN)) {
                    GLOG_D("EAGAIN");
                    continue;
                } else if (AVERROR_EOF) {
                    break;
                } else {
                    throw std::runtime_error(av_err2str(ret));
                }
            }

            queue->push(frame);

            ::av_packet_unref(pkt);
        }
        ::av_packet_free(&pkt);
        GLOG_D("read frame thread exit");
    }
};

} // namespace av

} // namespace my

int main(int, char *[]) {
    auto ev_bus = my::EventBus::create();
    auto async_task = my::AsyncTask::create();
    auto resource_mgr = my::ResourceMgr::create(async_task.get());
    auto audio_mgr = my::AudioMgr::create();
    auto win_mgr = my::WindowMgr::create(ev_bus.get());
    auto font_mgr = my::FontMgr::create();

    auto url =
        "/home/yydcnjjw/workspace/code/project/my-gui/assets/media/10.mp4";

    auto win = win_mgr->create_window("test", 1280, 720);
    LLGL::Log::SetReportCallbackStd();
    auto renderer = LLGL::RenderSystem::Load("Vulkan");
    my::Canvas canvas(renderer.get(), win, ev_bus.get(), resource_mgr.get(),
                      font_mgr.get());
    auto player = my::av::AVPlayer(url, &canvas, async_task.get());

    std::this_thread::sleep_for(std::chrono::seconds(100));

    Logger::get()->close();
    return 0;
}
