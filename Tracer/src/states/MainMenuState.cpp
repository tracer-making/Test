#include "MainMenuState.h"
#include "../core/App.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>

MainMenuState::MainMenuState() = default;
MainMenuState::~MainMenuState() = default;

void MainMenuState::onEnter(App& app) {
	int w, h; SDL_GetWindowSize(app.getWindow(), &w, &h); screenW_ = w; screenH_ = h;

	// 加载中文字体（请确保文件存在）
	font_ = TTF_OpenFont("assets/fonts/simkai.ttf", 28);
	if (!font_) { SDL_Log("TTF_OpenFont failed: %s", TTF_GetError()); }

	std::vector<Button*> owned;
	owned.reserve(4);
	for (int i = 0; i < 4; ++i) owned.push_back(new Button());
	buttons_[0] = owned[0];
	buttons_[1] = owned[1];
	buttons_[2] = owned[2];
	buttons_[3] = owned[3];

	int bw = 260, bh = 56;
	int cx = screenW_ / 2 - bw / 2;
	int cy = screenH_ / 2 - (bh * 4 + 18 * 3) / 2;
	const char* labels[4] = {u8"开始游戏", u8"设置", u8"藏馆", u8"退出"};
	for (int i = 0; i < 4; ++i) {
		SDL_Rect r { cx, cy + i * (bh + 18), bw, bh };
		buttons_[i]->setRect(r);
		buttons_[i]->setText(labels[i]);
		if (font_) buttons_[i]->setFont(font_, app.getRenderer());
	}
	buttons_[0]->setOnClick([&app]() { SDL_Log("Start clicked"); });
	buttons_[1]->setOnClick([]() { SDL_Log("Settings clicked"); });
	buttons_[2]->setOnClick([]() { SDL_Log("Collection clicked"); });
	buttons_[3]->setOnClick([&app]() { SDL_Event quit; quit.type = SDL_QUIT; SDL_PushEvent(&quit); });
}

void MainMenuState::onExit(App& app) {
	for (auto* b : buttons_) { delete b; }
	buttons_ = {nullptr, nullptr, nullptr, nullptr};
	if (font_) { TTF_CloseFont(font_); font_ = nullptr; }
}

void MainMenuState::handleEvent(App& app, const SDL_Event& e) {
	for (auto* b : buttons_) if (b) b->handleEvent(e);
}

void MainMenuState::update(App& app, float) {
}

void MainMenuState::render(App& app) {
	SDL_Renderer* r = app.getRenderer();
	// 背景
	SDL_SetRenderDrawColor(r, 20, 22, 26, 255);
	SDL_Rect bg {0,0,screenW_,screenH_};
	SDL_RenderFillRect(r, &bg);

	// 标题占位
	SDL_SetRenderDrawColor(r, 230, 230, 240, 255);
	SDL_Rect title { screenW_/2 - 240, screenH_/2 - 200, 480, 64 };
	SDL_RenderDrawRect(r, &title);

	for (auto* b : buttons_) if (b) b->render(r);
}


