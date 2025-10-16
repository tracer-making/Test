#pragma once

#include "../core/State.h"
#include <SDL.h>

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
};

