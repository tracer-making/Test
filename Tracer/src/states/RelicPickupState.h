#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>

// 墨宝拾遗：拥有道具不满三个时，随机补齐至三个
class RelicPickupState : public State {
public:
	RelicPickupState();
	~RelicPickupState();

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
	int screenW_ = 1280, screenH_ = 720;
	
	// 状态切换
	bool pendingGoMapExplore_ = false;  // 返回地图探索

	std::vector<std::string> ownedRelics_;
	std::string message_;
	bool pendingBackToTest_ = false;

	void ensureThreeRelics();
};


