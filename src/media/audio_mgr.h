#pragma once

#include <SDL2/SDL_mixer.h>
#include <storage/resource_mgr.hpp>

namespace my {
class AudioMgr;
class Audio : public Resource {
  public:
    explicit Audio(const fs::path &path);
    Audio(const fs::path &path, std::shared_ptr<std::istream> is);    
    ~Audio();

    void set_volume(uint8_t volume);
    uint8_t get_volume();

    Mix_Chunk *sample;
    int channel = -1;
};

class AudioMgr {
  public:
    AudioMgr() = default;
    virtual ~AudioMgr() = default;

    virtual void play(Audio *) = 0;
    virtual void pause(Audio *) = 0;
    virtual void play_fade(Audio *, int ms) = 0;
    virtual void pause_fade(Audio *, int ms) = 0;
    // virtual void set_panning(Audio *, uint8_t left, uint8_t right) = 0;
    // virtual void set_pos(Audio *, int16_t angle, uint8_t distance) = 0;

    static std::unique_ptr<AudioMgr> create();
};

} // namespace my
