#pragma once
#include "../core/State.h"
#include "../core/Card.h"
#include "../ui/CardRenderer.h"
#include "../ui/Button.h"
#include <vector>
#include <memory>

class InkGhostState : public State {
public:
    InkGhostState();
    ~InkGhostState();

    void onEnter(App& app) override;
    void onExit(App& app) override;
    void update(App& app, float deltaTime) override;
    void render(App& app) override;
    void handleEvent(App& app, const SDL_Event& event) override;

private:
    void layoutHandCards();
    void renderHandCards(App& app);
    void renderCardSlots(App& app);
    void renderSelectedCard(App& app);
    void renderGeneratedCard(App& app);
    void generateCard();
    void addGeneratedCardToDeck();
    
    // 字体
    TTF_Font* titleFont_ = nullptr;
    TTF_Font* smallFont_ = nullptr;
    
    // 返回按钮
    Button* backButton_ = nullptr;
    
    // 手牌相关
    std::vector<Card> handCards_;
    std::vector<SDL_Rect> handCardRects_;
    int hoveredHandCardIndex_ = -1;
    float hoverTime_ = 0.0f;
    
    // 牌位相关
    SDL_Rect leftSlotRect_;   // 左侧牌位（显示生成的卡牌）
    SDL_Rect rightSlotRect_;  // 右侧牌位（可点击选择）
    bool rightSlotSelected_ = false;
    bool leftSlotFilled_ = false;
    
    // 选中的卡牌
    Card selectedCard_;
    bool hasSelectedCard_ = false;
    
    // 生成的卡牌
    Card generatedCard_;
    bool hasGeneratedCard_ = false;
    
    // 动画相关
    bool isAnimating_ = false;
    float animTime_ = 0.0f;
    float animDuration_ = 1.0f;
    
    // 屏幕尺寸
    int screenW_ = 0;
    int screenH_ = 0;
};
