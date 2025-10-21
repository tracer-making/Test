#pragma once

#include "../core/State.h"
#include "../core/Card.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include "../ui/Button.h"
#include <vector>
#include <string>
#include <array>

// 记忆修复：最初始占位版本（无交互与逻辑）
class MemoryRepairState : public State {
public:
	MemoryRepairState();
	MemoryRepairState(bool isBossVictory);  // Boss战胜利专用构造函数
	~MemoryRepairState();

	void onEnter(App& app) override;
	void onExit(App& app) override;
	void handleEvent(App& app, const SDL_Event& e) override;
	void update(App& app, float dt) override;
	void render(App& app) override;
    
    // 公共枚举和静态方法
    enum class BackHintType { Unknown, KnownTribe, KnownCost };
    static void setMapHintType(BackHintType hintType) { mapHintType_ = hintType; }
    
private:
    struct Candidate { 
        Card card; 
        SDL_Rect rect{0,0,0,0}; 
        BackHintType hint = BackHintType::Unknown; 
    };
	std::vector<Candidate> candidates_;
	int selected_ = -1;
	int screenW_ = 1280, screenH_ = 720;
	bool added_ = false;
    std::array<bool,3> flipped_{{false,false,false}};
    BackHintType sessionHint_ = BackHintType::Unknown; // 本次三选一的统一提示类型
    std::array<int, 3> sessionCostTypes_{{0, 1, 2}}; // 三个牌位的消耗类型：0=1墨滴, 1=2墨滴, 2=3墨滴, 3=魂骨
    bool isBossVictory_ = false; // 是否为Boss战胜利奖励
    Button* backButton_ = nullptr;
    Button* rerollButton_ = nullptr; // 重新抽卡按钮
    bool rerollUsed_ = false; // 是否已使用重新抽卡功能
    _TTF_Font* titleFont_ = nullptr;
    _TTF_Font* smallFont_ = nullptr;
    _TTF_Font* cardNameFont_ = nullptr;
    _TTF_Font* cardStatFont_ = nullptr;
    void buildCandidates();
	void layoutCandidates();
    
    // 静态变量用于存储地图传递的提示类型
    static BackHintType mapHintType_;
};

