#pragma once

#include "../core/State.h"
#include "../core/Card.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include "../ui/Button.h"
#include <vector>
#include <string>
#include <array>

// 记忆修复：最初始占位版本（无交互与逻辑）
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
    enum class BackHintType { Unknown, KnownTribe, KnownCost };
    struct Candidate { 
        Card card; 
        SDL_Rect rect{0,0,0,0}; 
        BackHintType hint = BackHintType::Unknown; 
    };
	std::vector<Candidate> candidates_;
	int selected_ = -1;
	int screenW_ = 1280, screenH_ = 720;
	bool added_ = false;
    std::array<bool,3> flipped_{{false,false,false}};
    BackHintType sessionHint_ = BackHintType::Unknown; // 本次三选一的统一提示类型
    Button* backButton_ = nullptr;
    _TTF_Font* titleFont_ = nullptr;
    _TTF_Font* smallFont_ = nullptr;
    _TTF_Font* cardNameFont_ = nullptr;
    _TTF_Font* cardStatFont_ = nullptr;
	void buildCandidates();
	void layoutCandidates();
};

