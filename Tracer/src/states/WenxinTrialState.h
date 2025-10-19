#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include "../core/Card.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>

class WenxinTrialState : public State {
public:
    WenxinTrialState();
    ~WenxinTrialState();

    void onEnter(App& app) override;
    void onExit(App& app) override;
    void handleEvent(App& app, const SDL_Event& e) override;
    void update(App& app, float dt) override;
    void render(App& app) override;

private:
    _TTF_Font* titleFont_ = nullptr;
    _TTF_Font* smallFont_ = nullptr;
    _TTF_Font* cardNameFont_ = nullptr;
    _TTF_Font* cardStatFont_ = nullptr;
    _TTF_Font* hintFont_ = nullptr;
    SDL_Texture* titleTex_ = nullptr;
    Button* backButton_ = nullptr;
    
    int screenW_ = 1600, screenH_ = 1000;
    std::string message_;
    bool pendingGoMapExplore_ = false;

    // 试炼类型枚举
    enum class TrialType {
        Wisdom,     // 智慧试炼：至少3个印记
        Bone,       // 魂骨试炼：至少5根魂骨
        Power,      // 力量试炼：至少4点攻击力
        Life,       // 生命试炼：至少6点生命值
        Tribe,      // 同族试炼：至少2张同部族
        Blood       // 血腥试炼：抽出的3张牌至少花费4点墨量（无消耗或带“消耗骨头”视为0）
    };

    // 三个试炼卡牌
    struct TrialCard {
        TrialType type;
        std::string content; // 试炼内容
        std::string description; // 试炼描述
        SDL_Rect rect{0,0,0,0};
        bool flipped = false;
        bool completed = false; // 是否已完成
    };
    std::vector<TrialCard> trialCards_;
    
    // 试炼状态
    bool trialStarted_ = false;
    std::vector<Card> selectedCards_; // 选中的卡牌
    int currentTrialIndex_ = -1; // 当前试炼索引
    int hoveredTrialIndex_ = -1; // 悬停的试炼索引
    
    // 抽牌动画
    struct DrawAnim {
        bool active = false;
        float time = 0.0f;
        float duration = 0.8f;
        Card card;
        SDL_Rect fromRect{0,0,0,0};
        SDL_Rect toRect{0,0,0,0};
    };
    DrawAnim drawAnim_;
    
    // 抽牌状态
    int cardsDrawn_ = 0; // 已抽牌数量
    std::vector<SDL_Rect> drawnCardRects_; // 已抽卡牌的显示位置
    bool calculatingResult_ = false; // 是否正在计算结果
    float calculationTime_ = 0.0f; // 计算时间
    int currentCalculationStep_ = 0; // 当前计算步骤
    std::string calculationText_ = ""; // 计算过程文字
    
    // 试炼完成状态
    bool trialCompleted_ = false; // 试炼是否完成
    bool trialSuccess_ = false; // 试炼是否成功
    bool cardsFadingOut_ = false; // 卡牌是否正在淡出
    float fadeOutTime_ = 0.0f; // 淡出时间
    float fadeOutDuration_ = 1.0f; // 淡出持续时间
    
    // 奖励卡牌
    Card rewardCard_; // 奖励卡牌
    SDL_Rect rewardCardRect_{0,0,0,0}; // 奖励卡牌位置
    bool rewardCardReady_ = false; // 奖励卡牌是否准备好

    void buildTrialCards();
    void layoutTrialCards();
    void startTrial(int trialIndex);
    void drawNextCard();
    void finishTrial();
    void updateCalculationText();
    void startCardsFadeOut();
    void generateRewardCard();
    bool checkTrialCompletion(const std::vector<Card>& cards, TrialType type);
    std::string getTrialTypeName(TrialType type);
    std::string getTrialDescription(TrialType type);
};
