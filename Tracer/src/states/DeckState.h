#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>

class DeckState : public State {
public:
	DeckState();
	~DeckState();

	void onEnter(App& app) override;
	void onExit(App& app) override;
	void handleEvent(App& app, const SDL_Event& e) override;
	void update(App& app, float dt) override;
	void render(App& app) override;


private:
	struct CardView {
		std::string name;
		int attack = 0;
		int health = 0;
		std::vector<std::string> marks;
		SDL_Rect rect{0,0,0,0};
	};

	int screenW_ = 1280;
	int screenH_ = 720;
	_TTF_Font* titleFont_ = nullptr;
	_TTF_Font* smallFont_ = nullptr;
	_TTF_Font* nameFont_ = nullptr;
	_TTF_Font* statFont_ = nullptr;
	SDL_Texture* titleTex_ = nullptr;
	Button* backButton_ = nullptr;

	std::vector<CardView> cards_;
	// 背景水墨点阵
	std::vector<std::pair<int,int>> bgDots_;



	void buildGlobalDeck();
	void layoutGrid();
};


