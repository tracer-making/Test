#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>

class VictoryState : public State {
public:
    VictoryState();
    ~VictoryState();

    void onEnter(App& app) override;
    void onExit(App& app) override;
    void handleEvent(App& app, const SDL_Event& e) override;
    void update(App& app, float dt) override;
    void render(App& app) override;

private:
    // 屏幕尺寸
    int screenW_ = 1280;
    int screenH_ = 720;
    
    // UI按钮
    Button* backToMenuButton_ = nullptr;
    Button* restartGameButton_ = nullptr;
    
    // 状态切换
    bool pendingGoMainMenu_ = false;
    bool pendingRestartGame_ = false;
    
    // 字体
    _TTF_Font* titleFont_ = nullptr;
    _TTF_Font* infoFont_ = nullptr;
    
    // 胜利信息
    std::string victoryMessage_ = "恭喜！您已击败最终Boss！";
    std::string finalStats_ = "";
    
    // 动画效果
    float titleScale_ = 0.0f;
    float titleTargetScale_ = 1.0f;
    float titleAnimTime_ = 0.0f;
    float titleAnimDuration_ = 1.0f;
    
    bool isTitleAnimating_ = true;
};
