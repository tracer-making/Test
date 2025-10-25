#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include "../core/Deck.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <string>

// 初始牌组选择界面
class DeckSelectState : public State {
public:
    DeckSelectState();
    ~DeckSelectState();

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
    _TTF_Font* deckNameFont_ = nullptr;
    _TTF_Font* deckDescFont_ = nullptr;
    
    // 标题
    SDL_Texture* titleTex_ = nullptr;
    int titleW_ = 0;
    int titleH_ = 0;
    
    // 初始牌组定义
    struct InitialDeck {
        std::string id;
        std::string name;
        std::string description;
        std::vector<std::string> cardIds;
        SDL_Rect rect;
    };
    
    // 六大初始牌组
    std::vector<InitialDeck> initialDecks_;
    int selectedDeckIndex_ = -1;
    bool viewingDeck_ = false; // 是否在查看牌组内容
    int hoveredDeckIndex_ = -1; // 悬停的牌组索引
    
    // 状态切换
    bool pendingGoMapExplore_ = false;
    
    // 教程按钮
    Button* tutorialButton_ = nullptr;
    
    void initializeDecks();
    void layoutDecks();
    void renderDeckView(App& app);
    
    // 教程系统
    void startTutorial();
};
