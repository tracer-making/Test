#pragma once

#include "../core/State.h"
#include <SDL.h>
#if __has_include(<SDL2/SDL_ttf.h>)
#include <SDL2/SDL_ttf.h>
#else
#include <SDL_ttf.h>
#endif
#include <array>
#include <string>

class Button;

class MainMenuState : public State {
public:
	MainMenuState();
	~MainMenuState();

	void onEnter(App& app) override;
	void onExit(App& app) override;
	void handleEvent(App& app, const SDL_Event& e) override;
	void update(App& app, float deltaSeconds) override;
	void render(App& app) override;

private:
	std::array<Button*, 4> buttons_ {nullptr, nullptr, nullptr, nullptr};
	int screenW_ = 1280;
	int screenH_ = 720;
	_TTF_Font* font_ = nullptr;
};


