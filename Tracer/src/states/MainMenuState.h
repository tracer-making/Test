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
#include <vector>

class Button;

class MainMenuState : public State {
public:
	MainMenuState();
	~MainMenuState();

	void onEnter(App& app) override;
	void handleEvent(App& app, const SDL_Event& e) override;
	void update(App& app, float) override;
	void render(App& app) override;

	void startBattle(App& app);

private:
	// 状态切换
	bool pendingGoMapExplore_ = false;
	std::array<Button*, 5> buttons_ {nullptr, nullptr, nullptr, nullptr, nullptr}; // Changed from 4 to 5
	int screenW_ = 1280;
	int screenH_ = 720;

	// 字体
	_TTF_Font* font_ = nullptr; // 标题大字体
	_TTF_Font* smallFont_ = nullptr; // 小字体用于按钮和数据流

	// 标题纹理
	SDL_Texture* titleTex_ = nullptr;
	int titleW_ = 0;
	int titleH_ = 0;

	// 科技感数据流（乱码字符）
	struct DataStream {
		int x;
		float y;
		float speed;
		std::string characters; // 字符序列
		std::vector<Uint8> alphas; // 每个字符的透明度
	};
	std::vector<DataStream> streams_;

	// 装饰性花纹
	struct Decoration {
		int x, y, radius;
		Uint8 alpha;
		bool isArc; // true=弧线, false=圆形
	};
	std::vector<Decoration> decorations_;

	// 闪烁星光
	struct Star {
		int x, y;
		float brightness; // 0.0-1.0
		float twinkleSpeed;
	};
	std::vector<Star> stars_;
};


