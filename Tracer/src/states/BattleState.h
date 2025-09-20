#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include "../ui/CardRenderer.h"
#include "../core/Card.h"
#include "../core/Deck.h"
#include <SDL.h>
#include <vector>
#include <array>

class BattleState : public State {
public:
	BattleState();
	~BattleState();

	void onEnter(App& app) override;
	void onExit(App& app) override;
	void handleEvent(App& app, const SDL_Event& e) override;
	void update(App& app, float dt) override;
	void render(App& app) override;

private:
	// 游戏状态
	enum class GamePhase {
		PlayerTurn,    // 玩家回合
		EnemyTurn,    // 敌人回合
		GameOver      // 游戏结束
	};
	GamePhase currentPhase_ = GamePhase::PlayerTurn;
	
	// 回合信息
	int currentTurn_ = 1;
	int playerHealth_ = 20;
	int enemyHealth_ = 20;
	
	// 战场区域 (3x4网格)
	static constexpr int BATTLEFIELD_ROWS = 3;
	static constexpr int BATTLEFIELD_COLS = 4;
	static constexpr int TOTAL_BATTLEFIELD_SLOTS = BATTLEFIELD_ROWS * BATTLEFIELD_COLS;
	
	struct BattlefieldCard {
		Card card;
		SDL_Rect rect;
		bool isPlayer = true;  // true=玩家，false=敌人
		int health = 0;       // 当前生命值
		bool isAlive = true;   // 是否存活
		bool isSacrificed = false; // 是否被献祭
	};
	std::array<BattlefieldCard, TOTAL_BATTLEFIELD_SLOTS> battlefield_;
	
	// 手牌区
	std::vector<Card> handCards_;
	std::vector<SDL_Rect> handCardRects_;
	int selectedHandCard_ = -1;
	
	// 墨锭牌堆和玩家牌堆
	SDL_Rect inkPileRect_;      // 墨锭牌堆
	SDL_Rect playerPileRect_;   // 玩家牌堆
	int inkPileCount_ = 20;
	int playerPileCount_ = 20;
	
	// 墨尺和砚台
	SDL_Rect inkRulerRect_;    // 墨尺（显示墨量）
	SDL_Rect inkStoneRect_;     // 砚台（墨池状态）
	int currentInk_ = 0;         // 当前墨量
	int maxInk_ = 10;           // 最大墨量
	
	// UI区域
	SDL_Rect battlefieldRect_;     // 战场区域
	SDL_Rect handAreaRect_;        // 手牌区域
	SDL_Rect enemyAreaRect_;       // 敌人区域
	SDL_Rect cardInfoRect_;        // 卡牌介绍区域
	SDL_Rect itemAreaRect_;        // 道具区域
	
	// 字体
	_TTF_Font* titleFont_ = nullptr;
	_TTF_Font* cardNameFont_ = nullptr;
	_TTF_Font* cardStatFont_ = nullptr;
	_TTF_Font* infoFont_ = nullptr;
	
	// 按钮
	Button* backButton_ = nullptr;
	Button* endTurnButton_ = nullptr;
	
	// 屏幕尺寸（适中尺寸）
	int screenW_ = 1600;
	int screenH_ = 1000;
	
	// 状态
	bool pendingBackToTest_ = false;
	std::string statusMessage_;
	
	// 鼠标悬停状态
	int hoveredBattlefieldIndex_ = -1;
	int hoveredHandCardIndex_ = -1;
	
	// 手牌动画状态
	std::vector<float> handCardScales_;  // 手牌缩放比例
	std::vector<float> handCardAnimTime_; // 手牌动画时间
	std::vector<float> handCardFloatY_;  // 手牌上下漂浮偏移
	std::vector<float> handCardFloatTime_; // 手牌漂浮动画时间
	float animationSpeed_ = 5.0f;        // 动画速度
	float hoverScale_ = 1.1f;            // 悬停时缩放
	float clickScale_ = 1.2f;            // 点击时缩放
	float floatAmplitude_ = 8.0f;        // 漂浮幅度
	float floatSpeed_ = 3.0f;            // 漂浮速度
	
	// 献祭系统
	bool isSacrificing_ = false;         // 是否处于献祭模式
	int sacrificeTargetCost_ = 0;        // 目标献祭消耗
	int currentSacrificeInk_ = 0;        // 当前献祭获得的墨量
	std::vector<int> sacrificeCandidates_; // 可献祭的卡牌索引
	
	// 献祭动画系统
	bool isSacrificeAnimating_ = false;  // 是否正在播放献祭动画
	bool showSacrificeInk_ = false;      // 是否显示献墨量（延时显示）
	float sacrificeAnimTime_ = 0.0f;     // 献祭动画时间
	float sacrificeAnimDuration_ = 1.0f; // 献祭动画持续时间
	std::vector<int> cardsToDestroy_;   // 待摧毁的卡牌索引
	
	// 私有方法
	void initializeBattle();
	void layoutUI();
	void layoutBattlefield();
	void layoutHandCards();
	// drawCard函数已移除，现在通过点击牌堆来抽牌
	void playCard(int handIndex, int battlefieldIndex);
	void endTurn();
	void enemyTurn();
	void checkGameOver();
	void renderBattlefield(App& app);
	void renderHandCards(App& app);
	void renderUI(App& app);
};
