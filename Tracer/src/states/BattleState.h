#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include "../ui/CardRenderer.h"
#include "../core/Card.h"
#include "../core/Deck.h"
#include <SDL.h>
#include <vector>
#include <array>
#include <iostream>

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

	// 上帝模式：锁血与墨尺
	bool lockPlayerHealth_ = false;
	int lockedPlayerHealthValue_ = 0;
	int lockedMeterNetValue_ = 0;
	float lockedMeterDisplayPos_ = 0.0f;
	int lockedMeterTarget_ = 0;

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
		int placedTurn = 0; // 放置（上场）时的回合数
		bool oneTurnGrowthApplied = false; // 一回合成长是否已触发
		bool isDiving = false; // 水袭印记：是否潜水状态
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

	// 道具系统
	struct Item {
		std::string id;           // 道具ID
		std::string name;         // 道具名称
		std::string description;  // 道具描述
		int count;               // 道具数量
		
		Item() : count(0) {}
		Item(const std::string& id, const std::string& name, const std::string& description, int count = 1)
			: id(id), name(name), description(description), count(count) {}
	};
	
	std::vector<Item> playerItems_;  // 玩家拥有的道具（最多14个）
	static constexpr int MAX_ITEMS = 3;  // 最大道具数量
	
	// 道具UI相关
	SDL_Rect itemSlots_[MAX_ITEMS];  // 道具槽位矩形
	bool isItemHovered_[MAX_ITEMS] = {false};  // 道具悬停状态
	
	// 风雅扇效果跟踪
	std::vector<int> fengyaShanAirstrikeSlots_;  // 记录获得风雅扇空袭效果的卡牌位置
	
	// 日晷效果跟踪
	bool riguiEffectActive_ = false;  // 日晷效果是否激活
	
	// 断因剑目标选择状态
	bool isSelectingTarget_ = false;  // 是否正在选择目标
	int selectedTargetIndex_ = -1;    // 选中的目标索引
	
	// 吞墨毫目标选择状态
	bool isSelectingTunmohaoTarget_ = false;  // 是否正在选择吞墨毫目标
	int selectedTunmohaoTargetIndex_ = -1;    // 选中的吞墨毫目标索引
	
	// 缚魂锁目标选择状态
	bool isSelectingFuhunsuoTarget_ = false;  // 是否正在选择缚魂锁目标
	int selectedFuhunsuoTargetIndex_ = -1;    // 选中的缚魂锁目标索引
	
	// 缚魂锁移动动画状态
	bool isFuhunsuoAnimating_ = false;  // 是否正在播放缚魂锁移动动画
	float fuhunsuoAnimTime_ = 0.0f;     // 缚魂锁动画时间
	float fuhunsuoAnimDuration_ = 1.0f; // 缚魂锁动画持续时间
	int fuhunsuoFromIndex_ = -1;        // 缚魂锁移动起始位置
	int fuhunsuoToIndex_ = -1;          // 缚魂锁移动目标位置
	SDL_Rect fuhunsuoFromRect_;         // 缚魂锁移动起始矩形
	SDL_Rect fuhunsuoToRect_;           // 缚魂锁移动目标矩形

	// 印记系统 - 特殊攻击方式
	enum class AttackType {
		Normal,         // 普通攻击（对位）
		Double,         // 双向攻击（斜对位）
		Triple,         // 三向攻击（对位+斜对位）
		Twice,          // 双重攻击（对位两次）
		DoubleTwice,    // 双向双重攻击（斜对位各攻击两次）
		TripleTwice,    // 三向双重攻击（对位+斜对位各攻击两次）
		AllDirection    // 全向打击（攻击对面全部卡牌）
	};

	// 获取卡牌的攻击类型
	AttackType getCardAttackType(const Card& card);

	// 检查卡牌是否有特定印记
	bool hasMark(const Card& card, const std::string& mark) const;
	
	// 检查多向攻击是否至少有一个方向可以攻击
	bool canMultiDirectionAttack(int attackerIndex, int targetCol, bool isPlayerAttacking);
	
	// 缚魂锁移动动画更新
	void updateFuhunsuoAnimation(float dt);
	
	// 随机印记效果：删去随机印记并添加任意一个印记
	void applyRandomMarkEffect(Card& card);

	// 计算用于展示的攻击力（受对位“臭臭/令人生厌”临时影响）
	int getDisplayAttackForIndex(int battlefieldIndex) const;

	// 水袭印记相关方法
	void updateWaterAttackMarks();
	void applyWaterAttackDiving();
	void applyWaterAttackSurfacing();
	
	// 水袭翻面状态
	std::array<bool, TOTAL_BATTLEFIELD_SLOTS> waterAttackFlipped_; // 每个卡牌的翻面状态

	// 护主印记：补位逻辑
	void applyGuardianForEnemyAttack();   // 敌方攻击前（敌人前进与成长之后）我方护主补位
	void applyGuardianForPlayerPlay(int justPlacedIndex); // 我方打牌后，敌方护主补位到其对位

	// 护主补位动画系统
	bool isGuardianAnimating_ = false;
	float guardianAnimTime_ = 0.0f;
	float guardianAnimDuration_ = 0.35f;
	std::vector<int> guardianFromIndices_;
	std::vector<int> guardianToIndices_;
	std::vector<Card> guardianCardsSnapshot_;
	std::vector<int> guardianHealthSnapshot_;
	std::array<bool, TOTAL_BATTLEFIELD_SLOTS> guardianMovingFrom_{}; // 动画期间隐藏源卡
	void scheduleGuardianMove(int fromIndex, int toIndex);
	void finalizeGuardianMoves();
	

	// 不死印记：待回手的卡（动画结束后统一加入手牌）
	std::vector<Card> pendingUndyingToHand_;
	
	// 反伤连锁深度限制（避免无限递归）
	int thornsChainDepth_ = 0;
	int thornsChainMax_ = 16;
	

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
	void setupAllDirectionTargets(int attackerIndex, int targetCol, bool isPlayerAttacking);

	// 横冲直撞相关方法
	void startRushing(int cardIndex);
	void updateRushing(float dt);
	void executeRushing();
	bool checkRushingCanMove(int currentRow, int currentCol, int direction);

	// 道具系统相关方法
	void addItem(const std::string& itemId, int count = 1);
	bool removeItem(const std::string& itemId, int count = 1);
	bool hasItem(const std::string& itemId) const;
	int getItemCount(const std::string& itemId) const;
	void useItem(const std::string& itemId);
	void initializeItems();  // 初始化所有道具定义

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
	
	// 一口之量印记：存储被献祭的卡牌信息
	struct SacrificedCard {
		Card card;
		int attack;
		int health;
	};
	std::vector<SacrificedCard> sacrificedCards_; // 被献祭的卡牌信息

	// 献祭动画系统
	bool isSacrificeAnimating_ = false;  // 是否正在播放献祭动画
	bool showSacrificeInk_ = false;      // 是否显示献墨量（延时显示）
	float sacrificeAnimTime_ = 0.0f;     // 献祭动画时间
	float sacrificeAnimDuration_ = 1.0f; // 献祭动画持续时间

	// 回合制系统
	bool hasDrawnThisTurn_ = false;      // 本回合是否已抽牌
	bool mustDrawThisTurn_ = false;      // 本回合是否必须抽牌（第二回合开始）

	// 墨尺指针与本轮伤害统计
	int meterPosition_ = 0;             // 指针位置，-5..5，负向偏向玩家一侧，正向偏向敌人一侧
	int playerDamageThisCycle_ = 0;      // 我方本轮（玩家攻+敌人攻比较周期）造成的伤害
	int enemyDamageThisCycle_ = 0;       // 敌方本轮造成的伤害
	int enemyHealthBeforePlayerAttack_ = 0; // 用于计算玩家阶段伤害
	int playerHealthBeforeEnemyTurn_ = 0;   // 用于计算敌方阶段伤害
	int lastPlayerDamage_ = 0;           // 上一次我方阶段造成的伤害
	int lastEnemyDamage_ = 0;            // 上一次敌方阶段造成的伤害
	int meterNet_ = 0;                  // 净伤平衡：玩家伤害累加，敌人伤害扣减（用于抵消），单位=刻度，范围最终夹在-5..5

	// 墨尺指针动画
	int meterTargetPos_ = 0;            // 目标刻度（-5..5）
	float meterDisplayPos_ = 0.0f;       // 当前显示刻度（可为小数，用于插值）
	float meterAnimTime_ = 0.0f;         // 动画计时
	float meterAnimDuration_ = 0.35f;    // 动画时长（秒）
	float meterStartPos_ = 0.0f;         // 动画起始刻度
	bool isMeterAnimating_ = false;      // 是否正在播放指针动画

	// 战斗系统
	int totalDamageDealt_ = 0;          // 本回合造成的总伤害
	bool showDamage_ = false;           // 是否显示伤害
	float damageDisplayTime_ = 0.0f;   // 伤害显示时间
	float damageDisplayDuration_ = 0.5f; // 伤害显示持续时间

	// 魂骨系统
	int boneCount_ = 0;                 // 魂骨数量
	std::vector<bool> previousCardStates_; // 上一帧的卡牌存活状态
	std::vector<bool> previousEnemyCardStates_; // 上一帧的敌方卡牌存活状态
	
	// 献祭之血印记：本回合献祭次数
	int sacrificeCountThisTurn_ = 0;   // 本回合献祭次数

	// 检索印记系统
	bool isSearchingDeck_ = false;      // 是否正在检索牌堆
	int selectedDeckCardIndex_ = -1;    // 选中的牌堆卡牌索引


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
	int specialAttackTargets_[8] = { -1, -1, -1, -1, -1, -1, -1, -1 }; // 最多8个目标（全向打击）
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

	bool hasSetInitialRushDirection_ = false; // 是否已设置初始冲刺方向
	bool hasSetInitialBruteForceDirection_ = false; // 是否已设置初始蛮力冲撞方向

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
	void renderDeckSelection(App& app);

	// 移动卡牌队列处理方法
	void processNextMovement();
	void onMovementComplete();

    // 掘墓人：回合结束时结算骨头增益（仅统计一方）
    // countEnemySide=false 统计我方；true 统计敌方
    void grantGravediggerBones(bool countEnemySide);

	// 敌人前进（第一行向第二行）- 动画版
	struct EnemyAdvanceStep {
		int fromIndex;
		int toIndex;
		SDL_Rect fromRect;
		SDL_Rect toRect;
	};
	std::vector<EnemyAdvanceStep> enemyAdvanceSteps_;
	bool isEnemyAdvancing_ = false;
	bool enemyAdvanceStartedThisFrame_ = false; // 启动当帧不执行落位，避免瞬移
	float enemyAdvanceAnimTime_ = 0.0f;
	float enemyAdvanceAnimDuration_ = 1.0f; // 延长动画时长，节奏更从容
	float enemyAdvanceOvershootScale_ = 0.6f; // 超冲量按两格中心垂直距离的比例
	float enemyAdvanceMinOvershootPx_ = 80.0f; // 超冲最小像素，确保位移感
	void prepareEnemyAdvanceSteps();
	bool startEnemyAdvanceIfAny();
	void updateEnemyAdvance(float dt);
	void executeEnemyAdvance();

	void advanceEnemiesFrontRow();

	// 一回合成长处理：保留解析与查找工具（可配合动画）
	bool parseOneTurnGrowthTarget(const Card& card, std::string& outTargetName);
	std::string findCardIdByName(const std::string& nameUtf8);

	// 成长动画相关（恢复变量，便于后续使用）
	struct GrowthStep {
		int index;
		bool willTransform;
		Card targetCard; // willTransform==true 时有效
		int addAttack;   // willTransform==false 时有效
		int addHealth;   // willTransform==false 时有效
	};
	std::vector<GrowthStep> pendingGrowth_;
	bool isGrowthAnimating_ = false;
	float growthAnimTime_ = 0.0f;
	float growthAnimDuration_ = 0.6f;
	// 成长触发标志：本次成长是否发生在玩家回合开始（抽牌前）
	bool growthAtTurnStart_ = false;
	// 回合开始成长调度（抽牌前），若有成长则启动动画并返回true
	bool scheduleTurnStartGrowth();
	// 敌人攻击前成长调度（在敌人向前移动之后，攻击之前），若有成长则启动动画并返回true
	bool scheduleEnemyPreAttackGrowth();
	// 标志：本次成长用于敌人攻击前
	bool growthForEnemyAttack_ = false;

};
