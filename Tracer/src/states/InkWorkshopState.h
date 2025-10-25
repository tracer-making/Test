#pragma once
#include "../core/State.h"
#include "../core/Card.h"
#include "../ui/CardRenderer.h"
#include "../ui/Button.h"
#include <vector>
#include <memory>

// 道具系统
struct Item {
    std::string id;
    std::string name;
    std::string description;
    int count;
    
    Item() = default;
    Item(const std::string& id, const std::string& name, const std::string& description, int count)
        : id(id), name(name), description(description), count(count) {}
};

class InkWorkshopState : public State {
public:
    InkWorkshopState(int mapLayer = 1);
    ~InkWorkshopState();

    void onEnter(App& app) override;
    void onExit(App& app) override;
    void update(App& app, float deltaTime) override;
    void render(App& app) override;
    void handleEvent(App& app, const SDL_Event& event) override;

private:
    void renderTitle(App& app);
    void renderSkinSlots(App& app);
    void renderAvailableSkins(App& app);
    void renderTool(App& app);
    void renderWenMai(App& app);
    void renderGoldenCard(App& app, const Card& card, const SDL_Rect& rect);
    void tryGetSkin(int skinType);
    void tryGetTool();
    void collectAllSkins();
    void layoutSkins();
    
    // 道具系统方法
    void addItem(const std::string& itemId, int count = 1);
    bool removeItem(const std::string& itemId, int count = 1);
    bool hasItem(const std::string& itemId) const;
    int getItemCount(const std::string& itemId) const;
    
    // 字体
    TTF_Font* titleFont_ = nullptr;
    TTF_Font* smallFont_ = nullptr;
    TTF_Font* cardNameFont_ = nullptr;  // 卡牌名称字体
    TTF_Font* cardStatFont_ = nullptr; // 卡牌属性字体
    
    // 返回按钮
    Button* backButton_ = nullptr;
    Button* tutorialButton_ = nullptr;
    
    // 牌位相关
    SDL_Rect rabbitSlotRect_;    // 兔皮牌位
    SDL_Rect wolfSlotRect_;      // 狼皮牌位
    SDL_Rect goldSheepSlotRect_; // 金羊皮牌位
    SDL_Rect toolRect_;          // 道具位置
    
    // 消耗的文脉点数
    int rabbitCost_ = 2;
    int wolfCost_ = 4;
    int goldSheepCost_ = 7;
    int toolCost_ = 7;
    
    // 可获得的毛皮
    std::vector<Card> availableSkins_;
    std::vector<SDL_Rect> skinRects_;
    int hoveredSkinIndex_ = -1;
    
    // 道具系统
    std::vector<Item> playerItems_;
    static constexpr int MAX_ITEMS = 3;
    
    // 动画相关
    float hoverTime_ = 0.0f;
    bool isAnimating_ = false;
    float animTime_ = 0.0f;
    float animDuration_ = 1.0f;
    
    // 屏幕尺寸
    int screenW_ = 0;
    int screenH_ = 0;
    
    // 状态提示
    std::string statusMessage_;
    float messageTime_ = 0.0f;
    float messageDuration_ = 2.0f;
    
    // 地图层级
    int mapLayer_ = 1;
    
    // 教程系统
    void startTutorial();
};
