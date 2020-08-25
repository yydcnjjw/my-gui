#pragma once

#include <SDL2/SDL_mixer.h>    
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/stream.hpp>

#include <storage/blob.hpp>
#include <storage/resource_mgr.hpp>

namespace my {

class Audio : public Resource {
  public:
    ~Audio() {
        if (this->_sample) {
            ::Mix_FreeChunk(this->_sample);
        }
    }

    size_t used_mem() override { return 0; }

    void play() {
        if (this->is_play()) {
            return;
        }

        int channel;
        if ((channel = ::Mix_PlayChannel(-1, this->_sample, 0)) == -1) {
            throw std::runtime_error(::Mix_GetError());
        }

        this->_channel = channel;
    }

    void play_fade(int ms) {
        if (this->is_play()) {
            return;
        }

        int channel;
        if ((channel = ::Mix_FadeInChannel(-1, this->_sample, 0, ms)) == -1) {
            throw std::runtime_error(::Mix_GetError());
        }

        this->_channel = channel;
    }

    void pause() {
        if (!this->is_paused()) {
            ::Mix_Pause(this->_channel);
        }
    }

    bool is_paused() {
        if (!this->is_play()) {
            GLOG_W("audio is not playing");
            return true;
        }

        return ::Mix_Paused(this->_channel);
    }

    bool is_play() { return this->_channel != -1; }

    void resume() {
        if (!this->is_play()) {
            GLOG_W("audio is not playing");
            return;
        }
        ::Mix_Resume(this->_channel);
    }

    void stop() {
        if (this->is_play()) {
            ::Mix_HaltChannel(this->_channel);
        }
    }

    void stop_fade(int ms) {
        if (this->is_play()) {
            ::Mix_FadeOutChannel(this->_channel, ms);
        }
    }
    void set_panning(uint8_t left, uint8_t right) {
        if (this->is_play()) {
            if (!::Mix_SetPanning(this->_channel, left, right)) {
                throw std::runtime_error(::Mix_GetError());
            }
        }
    }
    // virtual void set_pos(Audio *, int16_t angle, uint8_t distance) = 0;

    void set_volume(uint8_t volume) {
        ::Mix_VolumeChunk(this->_sample, volume);
    }

    uint8_t get_volume() { return this->_sample->volume; }

    static std::shared_ptr<Audio> make(Mix_Chunk *sample) {
        return std::make_shared<Audio>(sample);
    }

    Audio(Mix_Chunk *sample) : _sample(sample) {}

  private:
    Mix_Chunk *_sample{};
    int _channel{-1};
};

class AudioDevice : private boost::noncopyable,
                    std::enable_shared_from_this<AudioDevice> {
  public:
    AudioDevice(int freq, uint16_t samples) {
        SDL_AudioSpec want_audio{};
        want_audio.freq = freq;
        want_audio.format = AUDIO_S16SYS;
        want_audio.channels = 2;
        want_audio.samples = samples;
        this->_audio_device = ::SDL_OpenAudioDevice(
            nullptr, 0, &want_audio, &this->_audio_spec,
            SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);

        if (this->_audio_device <= 0) {
            throw std::runtime_error(::SDL_GetError());
        }
    }

    class AudioSink : public boost::iostreams::sink {
      public:
        AudioSink(std::shared_ptr<AudioDevice> device) : _device(device) {}

        std::streamsize write(const char *sample, std::streamsize size) {
            std::unique_lock<std::mutex> l_lock(this->_device->_lock);
            if (::SDL_QueueAudio(this->_device->_audio_device, sample, size) !=
                0) {
                throw std::runtime_error(::SDL_GetError());
            }
            return size;
        }

      private:
        std::shared_ptr<AudioDevice> _device;
    };

  public:
    std::shared_ptr<std::ostream> make_ostream() {
        return std::make_shared<boost::iostreams::stream<AudioSink>>(
            this->shared_from_this());
    }

  private:
    SDL_AudioSpec _audio_spec{};
    SDL_AudioDeviceID _audio_device{};
    std::mutex _lock{};
};

template <> class ResourceProvider<Audio> {
  public:
    static ResourceProvider<Audio> &get() {
        static ResourceProvider<Audio> _instance;
        return _instance;
    }

    ~ResourceProvider<Audio>() {
        ::Mix_CloseAudio();
        ::Mix_Quit();
    }

    /**
     * @brief      load from fs
     */
    static std::shared_ptr<Audio> load(const ResourceFileProvideInfo &info) {
        return ResourceProvider<Audio>::get()._load(
            ::SDL_RWFromFile(info.path.c_str(), "rb"), 1);
    }

    /**
     * @brief      load from stream
     */
    static std::shared_ptr<Audio> load(const ResourceStreamProvideInfo &info) {
        auto blob = Blob::make(info);
        return ResourceProvider<Audio>::get()._load(
            ::SDL_RWFromMem(const_cast<void *>(blob->data()), blob->size()), 0);
    }

  private:
    ResourceProvider<Audio>() {
        auto flags = MIX_INIT_MID | MIX_INIT_FLAC | MIX_INIT_MOD |
                     MIX_INIT_MP3 | MIX_INIT_OGG | MIX_INIT_OPUS;
        if (::Mix_Init(flags) != flags) {
            throw std::runtime_error(Mix_GetError());
        }

        if (::Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT,
                            MIX_DEFAULT_CHANNELS, 4096) < 0) {
            throw std::runtime_error(Mix_GetError());
        }

        ::Mix_AllocateChannels(16);
    }

    std::shared_ptr<Audio> _load(SDL_RWops *src, int freesrc) {
        Mix_Chunk *chunk{};
        if (!(chunk = ::Mix_LoadWAV_RW(src, freesrc))) {
            throw std::runtime_error(Mix_GetError());
        }
        return Audio::make(chunk);
    }
};

} // namespace my
