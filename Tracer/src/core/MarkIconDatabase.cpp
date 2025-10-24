#include "MarkIconDatabase.h"
#include <SDL_image.h>
#include <iostream>

MarkIconDatabase& MarkIconDatabase::instance() {
    static MarkIconDatabase instance;
    return instance;
}

std::string MarkIconDatabase::getMarkIconName(const std::string& markName) {
    if (markToIconMap_.empty()) {
        initializeMarkToIconMap();
    }
    
    auto it = markToIconMap_.find(markName);
    if (it != markToIconMap_.end()) {
        return it->second;
    }
    
    return ""; // 没有找到对应的图标
}

SDL_Texture* MarkIconDatabase::loadMarkIcon(SDL_Renderer* renderer, const std::string& markName) {
    // 检查是否已经加载过
    auto it = loadedTextures_.find(markName);
    if (it != loadedTextures_.end()) {
        return it->second;
    }
    
    // 获取图标文件名
    std::string iconName = getMarkIconName(markName);
    if (iconName.empty()) {
        return nullptr;
    }
    
    // 构建完整路径
    std::string path = "assets/abilitymarks/Texture2D/" + iconName + ".png";
    
    // 加载纹理
    SDL_Texture* texture = IMG_LoadTexture(renderer, path.c_str());
    if (texture) {
        loadedTextures_[markName] = texture;
        std::cout << "✓ Loaded mark icon: " << markName << " -> " << path << std::endl;
    } else {
        std::cout << "✗ Failed to load mark icon: " << markName << " -> " << path << " (iconName: " << iconName << ")" << std::endl;
    }
    
    return texture;
}

void MarkIconDatabase::cleanup() {
    for (auto& pair : loadedTextures_) {
        if (pair.second) {
            SDL_DestroyTexture(pair.second);
        }
    }
    loadedTextures_.clear();
}

void MarkIconDatabase::initializeMarkToIconMap() {
    // 根据印记名称映射到图标文件名（基于实际可用的图标文件）
    markToIconMap_ = {
        // 攻击类印记
        {"空袭", "ability_flying"},
        {"水袭", "ability_flying"}, // 使用飞行图标作为水袭
        {"双重攻击", "ability_doublestrike"},
        {"全向打击", "ability_allstrike"},
        {"双向攻击", "ability_splitstrike"},
        {"三向攻击", "ability_tristrike"},
        {"冲刺能手", "ability_strafe"},
        {"蛮力冲撞", "ability_strafepush"},
        
        // 防御类印记
        {"护主", "ability_guarddog"},
        {"坚硬之躯", "ability_madeofstone"},
        {"磐石之身", "ability_madeofstone"},
        {"反伤", "ability_deathtouch"},
        {"死神之触", "ability_deathtouch"},
        
        // 资源类印记
        {"消耗骨头", "ability_opponentbones"},
        {"半根骨头", "ability_opponentbones"},
        {"献祭之血", "ability_sacrificial"},
        {"铃铛距离", "ability_createbells"},
        
        // 特殊效果印记
        {"蚂蚁", "ability_drawant"},
        {"镜像", "ability_drawcopy"},
        {"手牌数", "ability_tutor"},
        {"生生不息", "ability_drawcopyondeath"},
        {"不死印记", "ability_deathshield"},
        {"优质祭品", "ability_morsel"},
        {"内心之蜂", "ability_drawant"},
        {"滋生寄生虫", "ability_drawrandomcardondeath"},
        {"断尾求生", "ability_tailonhit"},
        {"令人生厌", "ability_debuffenemy"},
        {"臭臭", "ability_debuffenemy"},
        {"蚁后", "ability_drawant"},
        {"一口之量", "ability_morsel"},
        {"兔窝", "ability_drawrabbits"},
        {"筑坝师", "ability_createdams"},
        {"检索", "ability_tutor"},
        {"道具商", "ability_randomconsumable"},
        {"随机", "ability_randomability"},
        
        // 意境系统
        {"意", "ability_randomability"},
        {"境", "ability_transformer"}
    };
}
