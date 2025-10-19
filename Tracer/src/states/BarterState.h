#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include "../core/Card.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <string>

class BarterState : public State {
public:
    BarterState();
    ~BarterState();

    void onEnter(App& app) override;
    void onExit(App& app) override;
    void handleEvent(App& app, const SDL_Event& e) override;
    void update(App& app, float dt) override;
    void render(App& app) override;

private:
    _TTF_Font* titleFont_ = nullptr;
    _TTF_Font* smallFont_ = nullptr;
    SDL_Texture* titleTex_ = nullptr;
    Button* backButton_ = nullptr;
    int screenW_ = 1600, screenH_ = 1000;

    // 状态切换
    bool pendingGoMapExplore_ = false;  // 返回地图探索

    std::string message_;
    bool hasFur_ = false;
    
    // 交易界面相关
    enum class TradeType {
        None,
        RabbitFur,    // 兔皮交易
        WolfFur,      // 狼皮交易
        GoldenFur     // 金羊皮交易
    };
    TradeType currentTradeType_ = TradeType::None;
    
    // 左侧：玩家毛皮卡牌
    struct PlayerFurCard {
        Card card;
        SDL_Rect rect{0,0,0,0};
        bool selected = false;
        int count = 1; // 毛皮数量
    };
    std::vector<PlayerFurCard> playerFurCards_;
    
    // 右侧：随机交易卡牌
    struct TradeCard {
        Card card;
        SDL_Rect rect{0,0,0,0};
        bool selected = false;
    };
    std::vector<TradeCard> tradeCards_;
    
    // 动画相关
    bool animActive_ = false;
    float animTime_ = 0.0f;
    float animDuration_ = 1.0f;
    int animatingTradeCard_ = -1;
    int animatingPlayerCard_ = -1;

    void checkFurCards();
    bool hasFurCards();
    void startTrade(TradeType type);
    void buildPlayerFurCards();
    void buildTradeCards();
    void layoutCards();
    void performTrade(int tradeCardIndex);
    void nextTradeType();
};
