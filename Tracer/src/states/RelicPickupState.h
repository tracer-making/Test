#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>

// 墨宝拾遗：拥有道具不满三个时，随机补齐至三个
class RelicPickupState : public State {
public:
	RelicPickupState();
	~RelicPickupState();

	void onEnter(App& app) override;
	void onExit(App& app) override;
	void handleEvent(App& app, const SDL_Event& e) override;
	void update(App& app, float dt) override;
	void render(App& app) override;

private:
	_TTF_Font* titleFont_ = nullptr;
	_TTF_Font* smallFont_ = nullptr;
	SDL_Texture* titleTex_ = nullptr;
	Button* backButton_ = nullptr;
	int screenW_ = 1280, screenH_ = 720;
	static constexpr int MAX_ITEMS = 3;
	
	// 状态切换
	bool pendingGoMapExplore_ = false;  // 返回地图探索

	struct Item { std::string id; std::string name; std::string description; };
	std::vector<Item> playerItems_;               // 模拟玩家已有道具（最多3件）
	struct ItemCandidate { Item item; SDL_Rect rect{0,0,0,0}; bool hovered=false; bool picked=false; };
	std::vector<ItemCandidate> candidates_;       // 本次可拾取的候选道具
	bool spawnCard_ = false;                      // 是否改为生成书林署丞
	SDL_Rect cardRect_{0,0,0,0};                 // 书林署丞位置
	std::string message_;
	
	// 拾取动画
	struct PickupAnim {
		bool active = false;
		float time = 0.0f;
		float duration = 0.8f;
		SDL_Rect rect{0,0,0,0};
		std::string itemName;
	};
	PickupAnim pickupAnim_;
	
	bool pendingBackToTest_ = false;

	void setupPickupContent();
	void layoutCandidates();
};


