#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>
#include "../core/Deck.h"
#include <unordered_map>

class HeritageState : public State {
public:
	HeritageState();
	~HeritageState();

	void onEnter(App& app) override;
	void onExit(App& app) override;
	void handleEvent(App& app, const SDL_Event& e) override;
	void update(App& app, float dt) override;
	void render(App& app) override;

private:
	// 仅缓存渲染用矩形
	std::vector<SDL_Rect> cardRects_;

	// UI / 字体
	_TTF_Font* titleFont_ = nullptr;
	_TTF_Font* smallFont_ = nullptr;
	_TTF_Font* cardNameFont_ = nullptr;  // 卡牌名称字体
	_TTF_Font* cardStatFont_ = nullptr;  // 卡牌属性字体
	SDL_Texture* titleTex_ = nullptr;
	Button* backButton_ = nullptr;
	Button* confirmButton_ = nullptr;  // 动态创建的确认按钮

	// 屏幕
	int screenW_ = 1280;
	int screenH_ = 720;
	
	// 状态切换
	bool pendingGoMapExplore_ = false;  // 返回地图探索

	// 数据选择状态
	Card selectedSourceCard_;  // 被献祭的卡
	Card selectedTargetCard_;  // 接受传承的卡
	bool hasSourceCard_ = false;
	bool hasTargetCard_ = false;
	std::string message_;

	bool pendingBackToTest_ = false;
	
	// 手牌区状态
	bool showingHandCards_ = false;
	bool selectingSource_ = false;  // true=选择被献祭卡, false=选择接受传承卡
	std::vector<Card> availableCards_;
	std::vector<SDL_Rect> handCardRects_;
	int selectedHandCardIndex_ = -1;
	
	// 中间+按钮
	Button* plusButton_ = nullptr;
	
	// 悬停动画
	float leftSlotHover_ = 0.0f;   // 左边牌位悬停动画值
	float rightSlotHover_ = 0.0f;  // 右边牌位悬停动画值
	float plusButtonHover_ = 0.0f; // +按钮悬停动画值
	std::vector<float> handCardHovers_; // 手牌区卡牌悬停动画值
	
	// 传承动画
	bool isAnimating_ = false;     // 是否正在播放传承动画
	float animationTime_ = 0.0f;   // 动画时间
	float animationDuration_ = 2.0f; // 动画持续时间
	bool sourceCardDestroyed_ = false; // 源卡是否已被摧毁
	bool targetCardEnhanced_ = false;  // 目标卡是否已被强化

	// 持续漂浮动画
	float hoverTime_ = 0.0f;

	// 方法
	void layoutCardSlots();
	void performInheritance();
	void showHandCards(bool isSource);
	void hideHandCards();
	void layoutHandCards();
	bool canBeSacrificed(const Card& card);
	bool canReceiveInheritance(const Card& card);
	void renderMainInterface(App& app);
	void renderHandCardArea(App& app);
	void renderCardInSlot(App& app, const SDL_Rect& slot, const Card& card);
	void layoutGrid();
	
	// 获取战斗中的待更新数值
	std::unordered_map<std::string, std::pair<int, int>> getPendingCardUpdates();
};


