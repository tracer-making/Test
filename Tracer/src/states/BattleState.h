#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <vector>
#include <array>

struct Card;

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
	// 阴阳池相关
	int yinYangBalance_ = 50; // 0-100，平衡值
	SDL_Rect yinYangPoolRect_; // 阴阳池位置和大小

	// 卡牌槽 (3x4网格)
	static constexpr int BOARD_ROWS = 3;
	static constexpr int BOARD_COLS = 4;
	static constexpr int TOTAL_SLOTS = BOARD_ROWS * BOARD_COLS;
	std::array<SDL_Rect, TOTAL_SLOTS> cardSlots_; // 卡牌槽位置

	// 铃铛
	SDL_Rect bellRect_; // 铃铛位置

	// 手牌区
	int currentHandCards_ = 5; // 当前手牌数量，会动态变化
	std::vector<SDL_Rect> handCardRects_; // 手牌位置

	// 牌堆
	SDL_Rect drawPileRect_;    // 抽牌堆
	SDL_Rect discardPileRect_; // 弃牌堆

	// 水墨风格UI颜色
	SDL_Color backgroundColor_ = {245, 240, 230, 255}; // 宣纸色背景
	SDL_Color slotColor_ = {200, 190, 180, 120};       // 淡墨卡牌槽
	SDL_Color slotBorderColor_ = {60, 50, 40, 180};    // 深墨边框
	SDL_Color yinYangColor_ = {40, 35, 30, 220};       // 墨色阴阳鱼
	SDL_Color bellColor_ = {80, 70, 60, 200};          // 深墨铃铛
	SDL_Color handCardColor_ = {220, 210, 200, 150};   // 浅墨手牌

	// 屏幕尺寸
	int screenW_ = 1280;
	int screenH_ = 720;

	// 返回测试按钮
	Button* backToTestButton_ = nullptr;
	bool pendingGoTest_ = false;

	// 水墨装饰参数 - 预计算避免闪烁
	struct InkDot {
		int x, y, size;
	};
	struct StainEffect {
		int centerX, centerY, maxRadius;
	};
	struct CalligraphyLine {
		int startX, endX, y, offset;
	};
	struct CloudDecoration {
		int centerX, centerY, size;
		std::vector<std::pair<int, int>> points;
	};

	std::vector<InkDot> inkDots_;
	std::vector<StainEffect> stainEffects_;
	std::vector<CalligraphyLine> calligraphyLines_;
	std::vector<CloudDecoration> cloudDecorations_;
	std::vector<std::pair<int, int>> titleDecorationPoints_;
	std::vector<std::pair<int, int>> cornerDecorationPoints_;
};
