#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include "../core/Deck.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>

class BurnState : public State {
public:
	BurnState();
	~BurnState();

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
	Button* burnButton_ = nullptr;
	int screenW_ = 1280, screenH_ = 720;
	std::string message_;
	bool pendingBackToTest_ = false;

	std::vector<SDL_Rect> cardRects_;
	int selectedIndex_ = -1; // index in library

	void layoutGridFromLibrary();
};
