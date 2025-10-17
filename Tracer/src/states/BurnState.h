#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include "../core/Deck.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>

class BurnState : public State {
public:
	BurnState();
	~BurnState();

	void onEnter(App& app) override;
	void onExit(App& app) override;
	void handleEvent(App& app, const SDL_Event& e) override;
	void update(App& app, float dt) override;
	void render(App& app) override;

private:
	_TTF_Font* titleFont_ = nullptr;
	_TTF_Font* smallFont_ = nullptr;
	_TTF_Font* nameFont_  = nullptr;
	_TTF_Font* statFont_  = nullptr;
	SDL_Texture* titleTex_ = nullptr;
    Button* backButton_ = nullptr;
    Button* burnButton_ = nullptr;
	int screenW_ = 1600, screenH_ = 1000;
	std::string message_;
    bool pendingGoMapExplore_ = false;

    // 中央牌位与选择
    SDL_Rect slotRect_{0,0,0,0};
    bool selecting_ = false;
    std::vector<int> libIndices_;
    std::vector<SDL_Rect> libRects_;
    int selectedLibIndex_ = -1; // index in libIndices_

    // 动画
    bool animActive_ = false;
    float animTime_ = 0.0f;
    float animDuration_ = 0.8f;
    SDL_Rect animRect_{0,0,0,0};

    void layoutUI();
    void buildSelectionGrid();
};
