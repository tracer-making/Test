#include "App.h"
#include "State.h"
#include <SDL.h>
#if __has_include(<SDL2/SDL_ttf.h>)
#include <SDL2/SDL_ttf.h>
#else
#include <SDL_ttf.h>
#endif
#include <memory>
#include "Cards.h"
#include "Deck.h"

// 定义静态成员变量
bool App::godMode_ = false;
bool App::temperBlessing_ = false;
std::string App::selectedInitialDeck_ = "";
int App::remainingCandles_ = 2;

App::App() = default;
App::~App() { shutdown(); }

bool App::init(const char* title, int width, int height) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
		SDL_Log("SDL_Init Error: %s", SDL_GetError());
		return false;
	}

	if (TTF_Init() != 0) {
		SDL_Log("TTF_Init Error: %s", TTF_GetError());
		return false;
	}

	window_ = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
	if (!window_) {
		SDL_Log("CreateWindow Error: %s", SDL_GetError());
		return false;
	}

	renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer_) {
		SDL_Log("CreateRenderer Error: %s", SDL_GetError());
		return false;
	}

	// 加载内置卡牌数据库
	CardDB::instance().loadBuiltinCards();
	
	// 注意：不在这里初始化玩家牌堆，等用户选择牌组后再初始化

	running_ = true;
	return true;
}

void App::shutdown() {
	state_.reset();
	if (renderer_) { SDL_DestroyRenderer(renderer_); renderer_ = nullptr; }
	if (window_) { SDL_DestroyWindow(window_); window_ = nullptr; }
	TTF_Quit();
	SDL_Quit();
}

void App::run() {
	Uint64 prev = SDL_GetPerformanceCounter();
	while (running_) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) { running_ = false; }
			if (state_) state_->handleEvent(*this, e);
		}

		Uint64 now = SDL_GetPerformanceCounter();
		double delta = (double)(now - prev) / (double)SDL_GetPerformanceFrequency();
		prev = now;

		if (state_) state_->update(*this, static_cast<float>(delta));

		SDL_SetRenderDrawColor(renderer_, 10, 10, 12, 255);
		SDL_RenderClear(renderer_);
		if (state_) state_->render(*this);
		SDL_RenderPresent(renderer_);
	}
}

void App::setState(std::unique_ptr<State> nextState) {
	if (state_) state_->onExit(*this);
	state_ = std::move(nextState);
	if (state_) state_->onEnter(*this);
}


