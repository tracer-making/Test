#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include "../core/Deck.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>

class BarterState : public State {
public:
	BarterState();
	~BarterState();

	void onEnter(App& app) override;
	void onExit(App& app) override;
	void handleEvent(App& app, const SDL_Event& e) override;
	void update(App& app, float dt) override;
	void render(App& app) override;

private:
	_TTF_Font* titleFont_ = nullptr;
	_TTF_Font* smallFont_ = nullptr;
	_TTF_Font* nameFont_  = nullptr;
	_TTF_Font* statFont_  = nullptr;
	SDL_Texture* titleTex_ = nullptr;
	Button* backButton_ = nullptr;
	Button* confirmButton_ = nullptr;
	int screenW_ = 1280, screenH_ = 720;
	std::string message_;
	bool pendingBackToTest_ = false;

	// 左侧：牌库卡选择
	std::vector<SDL_Rect> libraryRects_;
	int selectedLibraryIndex_ = -1;
	// 右侧：三选一报价
	struct Offer { Card card; SDL_Rect rect{0,0,0,0}; };
	std::vector<Offer> offers_;
	int selectedOfferIndex_ = -1;

	void ensureDemoLibraryIfEmpty();
	void layoutGrids();
	void generateOffers();
};


