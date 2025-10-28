#pragma once

#include <SDL.h>
#if __has_include(<SDL2/SDL_mixer.h>)
#include <SDL2/SDL_mixer.h>
#define HAVE_SDL_MIXER 1
#elif __has_include(<SDL_mixer.h>)
#include <SDL_mixer.h>
#define HAVE_SDL_MIXER 1
#else
#define HAVE_SDL_MIXER 0
#endif
#include <vector>
#include <string>
#include <random>

class MusicManager {
public:
    static MusicManager& instance();
    
    bool init();
    void shutdown();
    
    void startRandomPlaylist();
    void stopMusic();
    void pauseMusic();
    void resumeMusic();
    
    void update(); // 检查音乐是否播放完毕，自动播放下一首
    
    bool isPlaying() const;
    bool isPaused() const;
    
private:
    MusicManager() = default;
    ~MusicManager() = default;
    MusicManager(const MusicManager&) = delete;
    MusicManager& operator=(const MusicManager&) = delete;
    
    std::vector<std::string> musicFiles_;
    std::mt19937 rng_;
    int currentTrackIndex_ = -1;
    bool initialized_ = false;
    
    void playNextTrack();
    void loadMusicFiles();
};
