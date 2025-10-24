#pragma once

#include <string>
#include <map>
#include <SDL.h>

class MarkIconDatabase {
public:
    static MarkIconDatabase& instance();
    
    // 获取印记对应的图标文件名
    std::string getMarkIconName(const std::string& markName);
    
    // 加载印记图标纹理
    SDL_Texture* loadMarkIcon(SDL_Renderer* renderer, const std::string& markName);
    
    // 清理资源
    void cleanup();

private:
    MarkIconDatabase() = default;
    ~MarkIconDatabase() = default;
    
    std::map<std::string, std::string> markToIconMap_;
    std::map<std::string, SDL_Texture*> loadedTextures_;
    
    void initializeMarkToIconMap();
};
