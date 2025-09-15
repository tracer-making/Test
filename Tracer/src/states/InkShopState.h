#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>

// 墨坊：进入赠送“寻常之墨”，可用文脉购买之墨
class InkShopState : public State {
public:
	InkShopState();
	~InkShopState();

	void onEnter(App& app) override;
	void onExit(App& app) override;
	void handleEvent(App& app, const SDL_Event& e) override;
	void update(App& app, float dt) override;
	void render(App& app) override;

private:
	struct InkItem { std::string name; int price = 0; SDL_Rect rect{0,0,0,0}; };

	_TTF_Font* titleFont_ = nullptr;
	_TTF_Font* smallFont_ = nullptr;
	SDL_Texture* titleTex_ = nullptr;
	Button* backButton_ = nullptr;
	int screenW_ = 1280, screenH_ = 720;

	// 数据
	int wenmai_ = 30; // 文脉余额（示例）
	std::vector<std::string> ownedInks_;
	std::vector<InkItem> shopItems_;
	std::string message_;
	bool pendingBackToTest_ = false;

	void grantEntryGift();
	void buildShopItems();
	void layoutItems();
};


