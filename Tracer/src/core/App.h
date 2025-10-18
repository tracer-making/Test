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
};


