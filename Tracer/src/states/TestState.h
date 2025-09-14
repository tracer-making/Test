#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <array>
#include <vector>

class TestState : public State {
public:
	TestState();
	~TestState();

	void onEnter(App& app) override;
	void handleEvent(App& app, const SDL_Event& e) override;
	void update(App& app, float dt) override;
	void render(App& app) override;

private:
	// 返回主菜单按钮
	Button* backButton_ = nullptr;
	
	// 测试按钮数组
	std::vector<Button*> testButtons_;

	// 字体
	_TTF_Font* font_ = nullptr;
	_TTF_Font* smallFont_ = nullptr;
	
	// 屏幕尺寸
	int screenW_ = 1280;
	int screenH_ = 720;
	
	// 标题
	SDL_Texture* titleTex_ = nullptr;
	int titleW_ = 0;
	int titleH_ = 0;
};
