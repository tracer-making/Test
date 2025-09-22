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
	int enemyHealth_ = 100;
	
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
		bool isMovedToDeath = false; // 是否因移动而死亡（不获得魂骨）
		int moveDirection = 0; // 移动方向，1=右，-1=左，0=无方向
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
	
	// 固定玩家牌堆
	std::vector<std::string> playerDeck_;  // 玩家牌堆内容（卡牌ID列表）
	
	// 印记系统 - 特殊攻击方式
	enum class AttackType {
		Normal,         // 普通攻击（对位）
		Double,         // 双向攻击（斜对位）
		Triple,         // 三向攻击（对位+斜对位）
		Twice,          // 双重攻击（对位两次）
		DoubleTwice,    // 双向双重攻击（斜对位各攻击两次）
		TripleTwice     // 三向双重攻击（对位+斜对位各攻击两次）
	};
	
	// 获取卡牌的攻击类型
	AttackType getCardAttackType(const Card& card);
	
	// 检查卡牌是否有特定印记
	bool hasMark(const Card& card, const std::string& mark);
	
	// 特殊攻击执行函数
	void executeSpecialAttack(int attackerIndex, int targetCol, bool isPlayerAttacking);
	void executeDiagonalAttack(int attackerIndex, int targetCol, bool isPlayerAttacking, int damage);
	void executeTripleAttack(int attackerIndex, int targetCol, bool isPlayerAttacking, int damage);
	void executeTwiceAttack(int attackerIndex, int targetCol, bool isPlayerAttacking, int damage);
	void executeNormalAttack(int attackerIndex, int targetCol, bool isPlayerAttacking, int damage);
	void attackTarget(int attackerIndex, int targetIndex, int damage);
	
	// 特殊攻击目标设置函数
	void setupDiagonalTargets(int attackerIndex, int targetCol, bool isPlayerAttacking);
	void setupTripleTargets(int attackerIndex, int targetCol, bool isPlayerAttacking);
	void setupTwiceTargets(int attackerIndex, int targetCol, bool isPlayerAttacking);
	void setupDoubleTwiceTargets(int attackerIndex, int targetCol, bool isPlayerAttacking);
	void setupTripleTwiceTargets(int attackerIndex, int targetCol, bool isPlayerAttacking);
	
	// 横冲直撞相关方法
	void startRushing(int cardIndex);
	void updateRushing(float dt);
	void executeRushing();
	bool checkRushingCanMove(int currentRow, int currentCol, int direction);
	
	// 蛮力冲撞相关方法
	void startBruteForce(int cardIndex);
	void updateBruteForce(float dt);
	void executeBruteForce();
	
	// 推动检测辅助方法
	struct PushResult {
		bool canPush;
		std::vector<int> cardsToPush;  // 被推动的卡牌索引
	};
	PushResult checkCanPush(int currentRow, int currentCol, int direction);
	
	
	// 被推动卡牌动画方法
	void startPushedAnimation(const std::vector<int>& cardIndices, const std::vector<int>& directions);
	void updatePushedAnimation(float dt);
	void executePushedAnimation();
	
	
	// 墨尺和砚台
	SDL_Rect inkRulerRect_;    // 墨尺（显示墨量）
	SDL_Rect inkStoneRect_;     // 砚台（墨池状态）
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
	_TTF_Font* enemyFont_ = nullptr;     // 敌人区域专用字体
	
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
	
	// 回合制系统
	bool hasDrawnThisTurn_ = false;      // 本回合是否已抽牌
	bool mustDrawThisTurn_ = false;      // 本回合是否必须抽牌（第二回合开始）
	
	// 战斗系统
	int totalDamageDealt_ = 0;          // 本回合造成的总伤害
	bool showDamage_ = false;           // 是否显示伤害
	float damageDisplayTime_ = 0.0f;   // 伤害显示时间
	float damageDisplayDuration_ = 0.5f; // 伤害显示持续时间
	
	// 魂骨系统
	int boneCount_ = 0;                 // 魂骨数量
	std::vector<bool> previousCardStates_; // 上一帧的卡牌存活状态
	
	// 上帝模式系统
	bool godMode_ = false;             // 是否处于上帝模式
	bool enemyTurnStarted_ = false;    // 敌方回合是否已开始
	
	// 卡牌摧毁动画系统
	bool isDestroyAnimating_ = false;  // 是否正在播放摧毁动画
	float destroyAnimTime_ = 0.0f;    // 摧毁动画时间
	float destroyAnimDuration_ = 0.5f; // 摧毁动画持续时间
	std::vector<int> cardsToDestroy_;  // 待摧毁的卡牌索引
	
	// 攻击动画系统
	bool isAttackAnimating_ = false;   // 是否正在播放攻击动画
	float attackAnimTime_ = 0.0f;     // 攻击动画时间
	float attackAnimDuration_ = 0.9f; // 攻击动画持续时间（稍微快一点）
	std::vector<int> attackingCards_;  // 正在攻击的卡牌索引
	int currentAttackingIndex_ = 0;   // 当前攻击的卡牌索引
	bool isPlayerAttacking_ = true;   // 是否玩家攻击阶段
	bool hasAttacked_ = false;        // 当前卡牌是否已经攻击过
	
	// 特殊攻击动画系统
	AttackType currentAttackType_ = AttackType::Normal;
	int specialAttackTargets_[3] = {-1, -1, -1}; // 最多3个目标（三向攻击）
	int currentTargetIndex_ = 0;
	bool isSpecialAttackAnimating_ = false;
	
	// 横冲直撞系统
	bool isRushing_ = false;           // 是否正在横冲直撞
	int rushingCardIndex_ = -1;       // 正在横冲直撞的卡牌索引
	int rushingDirection_ = 1;         // 横冲直撞方向（1=右，-1=左）
	float rushingAnimTime_ = 0.0f;    // 横冲直撞动画时间
	float rushingAnimDuration_ = 0.5f; // 横冲直撞动画持续时间
	bool isRushingShaking_ = false;    // 是否正在摇晃动画（改变方向时）
	
	// 蛮力冲撞系统
	bool isBruteForcing_ = false;     // 是否正在蛮力冲撞
	int bruteForceCardIndex_ = -1;   // 正在蛮力冲撞的卡牌索引
	int bruteForceDirection_ = 1;     // 蛮力冲撞方向（1=右，-1=左）
	float bruteForceAnimTime_ = 0.0f; // 蛮力冲撞动画时间
	float bruteForceAnimDuration_ = 0.6f; // 蛮力冲撞动画持续时间
	bool isBruteForceShaking_ = false; // 是否正在摇晃动画（改变方向时）
	
	
	// 被推动卡牌动画系统
	bool isPushedAnimating_ = false;  // 是否正在播放被推动动画
	std::vector<int> pushedCardIndices_; // 被推动的卡牌索引
	std::vector<int> pushedDirections_; // 被推动的方向
	float pushedAnimTime_ = 0.0f;     // 被推动动画时间
	float pushedAnimDuration_ = 0.6f; // 被推动动画持续时间
	
	// 移动卡牌队列系统
	std::vector<int> pendingMovementCards_; // 待处理的移动卡牌（按位置排序）
	bool isProcessingMovementQueue_ = false; // 是否正在处理移动队列
	
	
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
	
	// 移动卡牌队列处理方法
	void processNextMovement();
	void onMovementComplete();
	
};
