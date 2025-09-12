#pragma once

#include <SDL.h>
#include <memory>

class State;

class App {
public:
	App();
	~App();

	bool init(const char* title, int width, int height);
	void shutdown();

	void run();

	void setState(std::unique_ptr<State> nextState);

	SDL_Window* getWindow() const { return window_; }
	SDL_Renderer* getRenderer() const { return renderer_; }

private:
	SDL_Window* window_ = nullptr;
	SDL_Renderer* renderer_ = nullptr;
	bool running_ = false;
	std::unique_ptr<State> state_;
};


