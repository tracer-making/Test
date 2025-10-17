#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include "../core/Deck.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <string>
#include <unordered_map>

// 淬炼：选择一张手牌，执行 +1 攻 或 +2 血
class TemperState : public State {
public:
	TemperState();
	~TemperState();

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
    Button* confirmButton_ = nullptr; // “+”按钮
	int screenW_ = 1280, screenH_ = 720;
	
	// 状态切换
	bool pendingGoMapExplore_ = false;  // 返回地图探索

    // 中央牌位与选择
    SDL_Rect slotRect_ {0,0,0,0};
    bool addAttackMode_ = true; // true=+1攻, false=+2血（进入时随机）
    bool selecting_ = false; // 是否在选择界面（全局牌库）
    std::vector<int> libIndices_; // 显示的全局牌库索引
    std::vector<SDL_Rect> libRects_;
    int selectedLibIndex_ = -1; // 选中的全局牌库索引
    bool locked_ = false; // 按下“+”后锁定，不可再更换
    bool firstSuccessDone_ = false; // 第一次成功完成后允许点击牌确认并返回

    // 动画控制（第二次淬炼完成后的过场）
    enum class AnimType { None, Destroy, Return };
    bool animActive_ = false;
    AnimType animType_ = AnimType::None;
    float animTime_ = 0.0f;
    float animDuration_ = 0.8f;
	std::string message_;
	bool pendingBackToTest_ = false;
    // 本次会话内每张卡的淬炼次数（按实例ID统计）
    std::unordered_map<std::string, int> temperCountByInstance_;

    void layoutUIRects();
    void openSelection();
    void buildSelectionGrid();
    void applyTemper();
};


