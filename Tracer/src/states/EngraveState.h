#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>
#include "../core/EngraveStore.h"

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
    struct Choice { std::string title; std::string desc; bool isYi = false; std::string value; SDL_Rect rect{0,0,0,0}; };

	_TTF_Font* titleFont_ = nullptr;
	_TTF_Font* smallFont_ = nullptr;
	_TTF_Font* choiceFont_ = nullptr;
	SDL_Texture* titleTex_ = nullptr;
	Button* backButton_ = nullptr;
	int screenW_ = 1280, screenH_ = 720;
	
	// 状态切换
	bool pendingGoMapExplore_ = false;  // 返回地图探索

	std::vector<Choice> choices_; // 3项
	int selected_ = -1;
	std::string result_;
	bool pendingBackToTest_ = false;

	// 选择后动画
	bool hasChosen_ = false;
	bool isAnimating_ = false;
	float animTime_ = 0.0f;
	float animDuration_ = 0.8f;

	// 组合面板选择索引（仅当已有至少一条意境绑定时显示面板）
	int selectedYi_ = -1;
	int selectedJing_ = -1;
	bool showCombinePanel_ = false;

	// 组合完成提示
	bool showCombineHint_ = false;
	float hintTime_ = 0.0f;
	float hintDuration_ = 3.0f;
	std::string combineHintText_;

	void buildRandomChoices();
	void layoutChoices();
};


