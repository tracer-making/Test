#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>
#include "../core/Deck.h"

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
	SDL_Texture* titleTex_ = nullptr;
	Button* backButton_ = nullptr;
	Button* confirmButton_ = nullptr;

	// 屏幕
	int screenW_ = 1280;
	int screenH_ = 720;

	// 数据选择状态
	int selectedSource_ = -1;
	int selectedTarget_ = -1;
	std::string message_;

	// 手牌数量滑动条（2..10）
	int handCount_ = 6;
	const int handMin_ = 2;
	const int handMax_ = 10;
	SDL_Rect sliderTrack_{0,0,0,0};
	SDL_Rect sliderThumb_{0,0,0,0};
	bool sliderDragging_ = false;
	bool pendingBackToTest_ = false;

	// 方法
	void layoutGrid();
	void layoutSlider();
	void updateSliderFromMouse(int mx);
	void performInheritance();
};


