#pragma once
#include "../core/State.h"
#include <vector>
#include <string>

class App;

class TutorialState : public State {
public:
    TutorialState();
    ~TutorialState() override = default;
    
    void onEnter(App& app) override;
    void onExit(App& app) override;
    void update(App& app, float dt) override;
    void render(App& app) override;
    void handleEvent(App& app, const SDL_Event& e) override;
    
    // 设置教程内容
    void setTutorialTexts(const std::vector<std::string>& texts);
    void setTutorialHighlights(const std::vector<SDL_Rect>& highlights);
    void startTutorial();
    
private:
    std::vector<std::string> tutorialTexts_;
    std::vector<SDL_Rect> tutorialHighlights_;
    bool tutorialStarted_;
    
    // 屏幕尺寸
    int screenW_;
    int screenH_;
    
    // 字体
    _TTF_Font* font_;
};
