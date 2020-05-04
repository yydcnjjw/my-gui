#include "audio_mgr.h"

#include <exception>
#include <fstream>

#include <boost/iostreams/copy.hpp>

namespace {
class SDLAudioMgr : public my::AudioMgr {
  public:
    SDLAudioMgr() {
        auto flags = MIX_INIT_MID | MIX_INIT_FLAC | MIX_INIT_MOD |
                     MIX_INIT_MP3 | MIX_INIT_OGG | MIX_INIT_OPUS;
        if (::Mix_Init(flags) != flags) {
            throw std::runtime_error(Mix_GetError());
        }

        if (::Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, AUDIO_S16SYS, 2, 512) < 0) {
            throw std::runtime_error(Mix_GetError());
        }

        ::Mix_AllocateChannels(16);
    }

    ~SDLAudioMgr() override {
        ::Mix_CloseAudio();
        ::Mix_Quit();
    }

    void play(my::Audio *audio) override {
        int channel;
        if ((channel = ::Mix_PlayChannel(-1, audio->sample, 0)) == -1) {
            throw std::runtime_error(Mix_GetError());
        }

        audio->channel = channel;
    }

    void pause(my::Audio *audio) override {
        if (audio->channel == -1) {
            return;
        }
        ::Mix_Pause(audio->channel);
    }

    bool is_paused(my::Audio *audio) override {
        if (audio->channel == -1) {
            GLOG_W("audio is not bind channel");
            return true;
        }
        return ::Mix_Paused(audio->channel);
    }

    void resume(my::Audio *audio) override {
        if (audio->channel == -1) {
            GLOG_W("audio is not bind channel");
            return;
        }
        ::Mix_Resume(audio->channel);
    }

    void play_fade(my::Audio *audio, int ms) override {
        int channel;
        if ((channel = ::Mix_FadeInChannel(-1, audio->sample, 0, ms)) == -1) {
            throw std::runtime_error(::Mix_GetError());
        }

        audio->channel = channel;
    }

    virtual void stop(my::Audio *audio) override {
        if (audio->channel == -1) {
            return;
        }
        ::Mix_HaltChannel(audio->channel);
    }

    void stop_fade(my::Audio *audio, int ms) override {
        if (audio->channel == -1) {
            return;
        }
        ::Mix_FadeOutChannel(audio->channel, ms);
    }

    void set_panning(my::Audio *audio, uint8_t left, uint8_t right) override {
        if (audio->channel == -1) {
            return;
        }
        if (!::Mix_SetPanning(audio->channel, left, right)) {
            throw std::runtime_error(::Mix_GetError());
        }
    }
};
} // namespace

namespace my {
std::atomic_int Audio::_id = 0;
Audio::Audio(const fs::path &path) : Resource(path) {
    if (!(this->sample = ::Mix_LoadWAV(path.c_str()))) {
        throw std::runtime_error(Mix_GetError());
    }
}

Audio::Audio(const fs::path &path, std::shared_ptr<std::istream> is)
    : Resource(path) {

    if (!my::fs::exists("tmp")) {
        my::fs::create_directory("tmp");
    }

    this->_tmp /= "tmp";
    this->_tmp /= std::to_string(Audio::_id);
    this->_tmp.replace_extension(path.extension());
    std::ofstream ofs;
    ofs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ofs.open(_tmp);
    boost::iostreams::copy(*is, ofs);

    if (!(this->sample = ::Mix_LoadWAV(this->_tmp.c_str()))) {
        throw std::runtime_error(Mix_GetError());
    }
}

void Audio::set_volume(uint8_t volume) {
    Mix_VolumeChunk(this->sample, volume);
}
uint8_t Audio::get_volume() { return sample->volume; }

Audio::~Audio() {
    ::Mix_FreeChunk(this->sample);
    my::fs::remove(this->_tmp);
}

std::unique_ptr<AudioMgr> AudioMgr::create() {
    return std::make_unique<SDLAudioMgr>();
}
} // namespace my
