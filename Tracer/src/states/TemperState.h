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
	Button* atkButton_ = nullptr;
	Button* hpButton_ = nullptr;
	int screenW_ = 1280, screenH_ = 720;
	
	// 状态切换
	bool pendingGoMapExplore_ = false;  // 返回地图探索

	std::vector<SDL_Rect> cardRects_;
	int selectedIndex_ = -1;
	std::string message_;
	bool pendingBackToTest_ = false;
	struct TemperInfo { int successes = 0; float failProb = 0.0f; };
	std::unordered_map<std::string, TemperInfo> temperMap_;

	void layoutHandGrid();
	void applyTemper(bool addAttack);
};


