#pragma once

#include <SDL.h>
#include <memory>
#include <string>

class State;

class App {
public:
	App();
	~App();

	bool init(const char* title, int width, int height);
	void shutdown();

	void run();

	void setState(std::unique_ptr<State> nextState);

	SDL_Window* getWindow() const { return window_; }
	SDL_Renderer* getRenderer() const { return renderer_; }

	// 全局上帝模式
	static bool isGodMode() { return godMode_; }
	static void setGodMode(bool enabled) { godMode_ = enabled; }
	static void toggleGodMode() { godMode_ = !godMode_; }
	
	// 淬炼系统全局状态
	static bool hasTemperBlessing() { return temperBlessing_; }
	static void setTemperBlessing(bool enabled) { temperBlessing_ = enabled; }
	
	// 初始牌组选择
	static std::string getSelectedInitialDeck() { return selectedInitialDeck_; }
	static void setSelectedInitialDeck(const std::string& deckId) { selectedInitialDeck_ = deckId; }
	
	// 蜡烛系统（游戏生命值）
	static int getRemainingCandles() { return remainingCandles_; }
	static void setRemainingCandles(int count) { remainingCandles_ = count; }
	static void extinguishCandle() { 
		if (remainingCandles_ > 0) remainingCandles_--; 
	}
	static void restoreCandle() { 
		if (remainingCandles_ < 2) remainingCandles_++; 
	}
	static void resetCandles() { remainingCandles_ = 2; }
	
	// 全局印记提示系统
	static void showMarkTooltip(const std::string& markName, const std::string& description, int x, int y);
	static void hideMarkTooltip();
	static bool isMarkTooltipVisible() { return showMarkTooltip_; }
	static void getMarkTooltipInfo(std::string& markName, std::string& description, int& x, int& y);
	static void renderMarkTooltip();
	
	// 初始化叙事跳转
	static void initializeNarrativeTransitions();

	// 当前战斗位置信息（用于文本播放的 before/after 预设定位）
	static void setCurrentBattlePosition(int layer, int index) { currentBattleLayer_ = layer; currentBattleIndex_ = index; }
	static int getCurrentBattleLayer() { return currentBattleLayer_; }
	static int getCurrentBattleIndex() { return currentBattleIndex_; }

	// 全局战斗计数器（用于按顺序 1..4 周期索引）
	static int incrementBattleCounter() { return ++battleCounter_; }
	static int getBattleCounter() { return battleCounter_; }
	static void resetBattleCounter() { battleCounter_ = 0; }

private:
	SDL_Window* window_ = nullptr;
	SDL_Renderer* renderer_ = nullptr;
	bool running_ = false;
	std::unique_ptr<State> state_;
	
	// 全局上帝模式状态
	static bool godMode_;
	// 淬炼系统全局状态
	static bool temperBlessing_;
	// 初始牌组选择
	static std::string selectedInitialDeck_;
	// 蜡烛系统（游戏生命值）
	static int remainingCandles_;
	// 全局印记提示系统
	static bool showMarkTooltip_;
	static std::string tooltipMarkName_;
	static std::string tooltipDescription_;
	static int tooltipMouseX_;
	static int tooltipMouseY_;

	// 当前战斗位置信息
	static int currentBattleLayer_;
	static int currentBattleIndex_;
	static int battleCounter_;
};


