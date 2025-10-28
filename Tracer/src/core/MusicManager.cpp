#include "MusicManager.h"
#include <iostream>
#include <fstream>
#include <SDL.h>
#include <algorithm>

MusicManager& MusicManager::instance() {
    static MusicManager instance;
    return instance;
}

bool MusicManager::init() {
    if (initialized_) return true;
#if HAVE_SDL_MIXER
    // 初始化 SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }
    // 设置音乐音量（0-128）
    Mix_VolumeMusic(64);
    // 加载音乐文件列表
    loadMusicFiles();
    // 初始化随机数生成器
    rng_.seed(std::random_device{}());
    initialized_ = true;
    SDL_Log("[Music] MusicManager initialized with %d tracks", (int)musicFiles_.size());
    return true;
#else
    std::cerr << "SDL_mixer not available at compile time, music disabled." << std::endl;
    return false;
#endif
}

void MusicManager::shutdown() {
    if (!initialized_) return;
#if HAVE_SDL_MIXER
    stopMusic();
    Mix_CloseAudio();
#endif
    initialized_ = false;
}

static bool fileExists(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return f.good();
}

void MusicManager::loadMusicFiles() {
    musicFiles_.clear();
    
    // 直接添加播放列表（不做存在性过滤，避免中文/编码引发的误判）
    musicFiles_.push_back("assets/music/1.ogg");
    musicFiles_.push_back("assets/music/2.mp3");
    musicFiles_.push_back("assets/music/1.mp3");
    musicFiles_.push_back("assets/music/2.ogg");
    
    SDL_Log("[Music] Playlist prepared with %d entries", (int)musicFiles_.size());
}

void MusicManager::startRandomPlaylist() {
    if (!initialized_ || musicFiles_.empty()) return;
#if HAVE_SDL_MIXER
    // 随机选择第一首歌
    std::uniform_int_distribution<int> dist(0, static_cast<int>(musicFiles_.size()) - 1);
    currentTrackIndex_ = dist(rng_);
    playNextTrack();
#endif
}

void MusicManager::playNextTrack() {
    if (!initialized_ || musicFiles_.empty()) return;
#if HAVE_SDL_MIXER
    stopMusic();
    const std::string& trackPath = musicFiles_[currentTrackIndex_];
    Mix_Music* music = Mix_LoadMUS(trackPath.c_str());
    if (!music) {
        std::cerr << "Failed to load music: " << trackPath << " Error: " << Mix_GetError() << std::endl;
        return;
    }
    if (Mix_PlayMusic(music, 0) == -1) {
        std::cerr << "Failed to play music: " << trackPath << " Error: " << Mix_GetError() << std::endl;
        Mix_FreeMusic(music);
        return;
    }
    SDL_Log("[Music] Now playing: %s", trackPath.c_str());
#endif
}

void MusicManager::stopMusic() {
#if HAVE_SDL_MIXER
    if (Mix_PlayingMusic()) {
        Mix_HaltMusic();
    }
#endif
}

void MusicManager::pauseMusic() {
#if HAVE_SDL_MIXER
    if (Mix_PlayingMusic() && !Mix_PausedMusic()) {
        Mix_PauseMusic();
    }
#endif
}

void MusicManager::resumeMusic() {
#if HAVE_SDL_MIXER
    if (Mix_PausedMusic()) {
        Mix_ResumeMusic();
    }
#endif
}

void MusicManager::update() {
    if (!initialized_ || musicFiles_.empty()) return;
#if HAVE_SDL_MIXER
    // 检查当前音乐是否播放完毕
    if (!Mix_PlayingMusic()) {
        // 播放下一首（循环播放）
        currentTrackIndex_ = (currentTrackIndex_ + 1) % musicFiles_.size();
        playNextTrack();
    }
#endif
}

bool MusicManager::isPlaying() const {
#if HAVE_SDL_MIXER
    return Mix_PlayingMusic() != 0;
#else
    return false;
#endif
}

bool MusicManager::isPaused() const {
#if HAVE_SDL_MIXER
    return Mix_PausedMusic() != 0;
#else
    return false;
#endif
}
