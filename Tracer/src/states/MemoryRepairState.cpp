#include "MemoryRepairState.h"
#include "../core/App.h"

MemoryRepairState::MemoryRepairState() = default;
MemoryRepairState::~MemoryRepairState() = default;

void MemoryRepairState::onEnter(App& app) {}
void MemoryRepairState::onExit(App& app) {}
void MemoryRepairState::handleEvent(App& app, const SDL_Event& e) {}
void MemoryRepairState::update(App& app, float dt) {}
void MemoryRepairState::render(App& app) {
	SDL_Renderer* r = app.getRenderer();
	SDL_SetRenderDrawColor(r, 18, 22, 32, 255);
	SDL_RenderClear(r);
}
