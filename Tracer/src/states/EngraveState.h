#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>

// 意境刻画：三选一（随机生成“意/印记”或“境/类型”）
class EngraveState : public State {
public:
	EngraveState();
	~EngraveState();

	void onEnter(App& app) override;
	void onExit(App& app) override;
	void handleEvent(App& app, const SDL_Event& e) override;
	void update(App& app, float dt) override;
	void render(App& app) override;

private:
	struct Choice { std::string title; std::string desc; SDL_Rect rect{0,0,0,0}; };

	_TTF_Font* titleFont_ = nullptr;
	_TTF_Font* smallFont_ = nullptr;
	SDL_Texture* titleTex_ = nullptr;
	Button* backButton_ = nullptr;
	Button* confirmButton_ = nullptr;
	int screenW_ = 1280, screenH_ = 720;

	std::vector<Choice> choices_; // 3项
	int selected_ = -1;
	std::string result_;
	bool pendingBackToTest_ = false;

	void buildRandomChoices();
	void layoutChoices();
};


