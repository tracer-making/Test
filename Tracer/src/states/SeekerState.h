#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include "../core/Deck.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <string>

// 寻物人：选择手牌一张，破壁后获得传奇之墨或传奇卡
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
	SDL_Texture* titleTex_ = nullptr;
	Button* backButton_ = nullptr;
	Button* confirmButton_ = nullptr; // 破壁
	int screenW_ = 1280, screenH_ = 720;

	std::vector<SDL_Rect> cardRects_;
	int selectedIndex_ = -1;
	std::string message_;
	bool pendingBackToTest_ = false;

	void layoutHandGrid();
};


