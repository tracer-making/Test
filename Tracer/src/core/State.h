#pragma once

#include <SDL.h>

class App;

class State {
public:
	virtual ~State() = default;
	virtual void onEnter(App& app) {}
	virtual void onExit(App& app) {}
	virtual void handleEvent(App& app, const SDL_Event& e) = 0;
	virtual void update(App& app, float deltaSeconds) = 0;
	virtual void render(App& app) = 0;
};


