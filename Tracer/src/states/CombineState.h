#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include "../core/Deck.h"
#include "../core/Cards.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>

// 合卷：上方两个并排的大牌位；点击后检索牌库中同名的两张牌，展示在下方区域
class CombineState : public State {
public:
    CombineState();
    ~CombineState();

    void onEnter(App& app) override;
    void onExit(App& app) override;
    void handleEvent(App& app, const SDL_Event& e) override;
    void update(App& app, float dt) override;
    void render(App& app) override;

private:
    _TTF_Font* titleFont_ = nullptr;
    _TTF_Font* smallFont_ = nullptr;
    _TTF_Font* nameFont_  = nullptr;
    _TTF_Font* statFont_  = nullptr;
    SDL_Texture* titleTex_ = nullptr;
    Button* backButton_ = nullptr;
    Button* combineButton_ = nullptr; // 合卷按钮

    int screenW_ = 1600, screenH_ = 1000;
    bool pendingGoMapExplore_ = false;
    std::string message_;

    // 上方并排大牌位
    SDL_Rect slotLeft_{0,0,0,0};
    SDL_Rect slotRight_{0,0,0,0};
    bool selecting_ = false; // 是否显示下方选择界面

    // 下方展示：所有同名两张一组
    std::vector<std::pair<int,int>> pairs_; // 每组在library中的两个索引
    std::vector<SDL_Rect> pairRectsA_; // 每组左卡缩略图rect
    std::vector<SDL_Rect> pairRectsB_; // 每组右卡缩略图rect
    int selectedPair_ = -1; // 选中的组，用于放到上方牌位
    int pairLibIndexA_ = -1; // 当前上方左牌引用的library索引
    int pairLibIndexB_ = -1; // 当前上方右牌引用的library索引

    // 融合动画
    struct CombineAnim {
        bool active = false;
        float time = 0.0f;
        float duration = 1.2f;
        SDL_Rect leftRect{0,0,0,0};
        SDL_Rect rightRect{0,0,0,0};
        SDL_Rect centerRect{0,0,0,0};
        Card resultCard;
    };
    CombineAnim combineAnim_;

    // 获取动画
    struct AcquireAnim {
        bool active = false;
        float time = 0.0f;
        float duration = 0.8f;
        SDL_Rect rect{0,0,0,0};
        Card card;
    };
    AcquireAnim acquireAnim_;

    void layoutUI();
    void buildAllPairs();
    void layoutPairsGrid();
    void combineSelectedPair();
};


