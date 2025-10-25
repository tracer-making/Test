#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include "../core/Card.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <string>

// 寻物人：中心三张牌背，点击翻面。一张为金羊毛，其余两张为全牌库随机并附加两个印记
class SeekerState : public State {
public:
	SeekerState();
	~SeekerState();

	void onEnter(App& app) override;
	void onExit(App& app) override;
	void handleEvent(App& app, const SDL_Event& e) override;
	void update(App& app, float dt) override;
	void render(App& app) override;

private:
	_TTF_Font* titleFont_ = nullptr;
	_TTF_Font* smallFont_ = nullptr;
	_TTF_Font* nameFont_ = nullptr;
	_TTF_Font* statFont_ = nullptr;
	SDL_Texture* titleTex_ = nullptr;
	Button* backButton_ = nullptr;
	Button* tutorialButton_ = nullptr;
	int screenW_ = 1600, screenH_ = 1000;

	// 状态切换
	bool pendingGoMapExplore_ = false;  // 返回地图探索

	struct Entry {
		Card card;
		SDL_Rect rect{0,0,0,0};
		bool revealed = false;
	};
	std::vector<Entry> entries_;
	std::string message_;

	// 获取动画
	bool animActive_ = false;
	float animTime_ = 0.0f;
	float animDuration_ = 0.8f;
	SDL_Rect animRect_{0,0,0,0};
	int pickedIndex_ = -1;

	void buildEntries();
	void layoutEntries();
	
	// 教程系统
	void startTutorial();
};
