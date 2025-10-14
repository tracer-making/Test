#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <string>

// 寻物人：从三件未知文物中选择一件破壁，获得奖励
class SeekerState : public State {
public:
	SeekerState();
	~SeekerState();

	void onEnter(App& app) override;
	void onExit(App& app) override;
	void handleEvent(App& app, const SDL_Event& e) override;
	void update(App& app, float dt) override;
	void render(App& app) override;

private:
	_TTF_Font* titleFont_ = nullptr;
	_TTF_Font* smallFont_ = nullptr;
	_TTF_Font* nameFont_ = nullptr;
	_TTF_Font* statFont_ = nullptr;
	SDL_Texture* titleTex_ = nullptr;
	Button* backButton_ = nullptr;
	Button* confirmButton_ = nullptr; // 破壁
	int screenW_ = 1600, screenH_ = 1000;
	
	// 状态切换
	bool pendingGoMapExplore_ = false;  // 返回地图探索

	struct Artifact {
		std::string title;      // 展示名称（未揭示时可为“未知文物”）
		int rarity = 0;         // 0 普通 / 1 稀有 / 2 传奇
		std::string rewardText; // 揭示后的奖励描述
		bool revealed = false;  // 是否已破壁揭示
		SDL_Rect rect{0,0,0,0}; // 布局矩形
	};
	std::vector<Artifact> artifacts_;
	int selectedIndex_ = -1;
	std::string message_;
	bool pendingBackToTest_ = false;

	// 生成三件未知文物（含权重）
	void generateArtifacts();
	// 布局三件文物的位置与大小
	void layoutArtifactsGrid();
};



