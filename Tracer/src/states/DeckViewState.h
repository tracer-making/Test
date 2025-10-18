#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include "../core/Deck.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <string>

// 牌库查看界面
class DeckViewState : public State {
public:
    DeckViewState();
    ~DeckViewState();

    void onEnter(App& app) override;
    void onExit(App& app) override;
    void handleEvent(App& app, const SDL_Event& e) override;
    void update(App& app, float dt) override;
    void render(App& app) override;

private:
    // 屏幕尺寸
    int screenW_ = 1280;
    int screenH_ = 720;
    
    // 字体
    _TTF_Font* titleFont_ = nullptr;
    _TTF_Font* smallFont_ = nullptr;
    
    // 标题
    SDL_Texture* titleTex_ = nullptr;
    int titleW_ = 0;
    int titleH_ = 0;
    
    // 牌库数据
    std::vector<Card> libraryCards_;
    std::vector<SDL_Rect> cardRects_;
    
    // 滚动
    int scrollY_ = 0;
    int maxScrollY_ = 0;
    int cardHeight_ = 180;
    int cardWidth_ = 120;
    int cardSpacing_ = 10;
    int cardsPerRow_ = 10;
    
    // 状态切换
    bool pendingGoMapExplore_ = false;
    
    void layoutCards();
    void updateScrollBounds();
};
