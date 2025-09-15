#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include "../core/Deck.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>

// 记忆修复：简单版三选一获得卡牌，预留模式标签（随机/种族/指定消耗）
class MemoryRepairState : public State {
public:
	MemoryRepairState();
	~MemoryRepairState();

	void onEnter(App& app) override;
	void onExit(App& app) override;
	void handleEvent(App& app, const SDL_Event& e) override;
	void update(App& app, float dt) override;
	void render(App& app) override;

private:
	struct Candidate { Card card; SDL_Rect rect{0,0,0,0}; };

	_TTF_Font* titleFont_ = nullptr;
	_TTF_Font* smallFont_ = nullptr;
	SDL_Texture* titleTex_ = nullptr;
	Button* backButton_ = nullptr;
	Button* confirmButton_ = nullptr;
	int screenW_ = 1280, screenH_ = 720;

	// 模式标签（占位）
	int mode_ = 0; // 0随机 1种族 2指定消耗

	std::vector<Candidate> candidates_;
	int selected_ = -1;
	std::string message_;
	bool pendingBackToTest_ = false;

	void buildCandidates();
	void layoutCandidates();
};


