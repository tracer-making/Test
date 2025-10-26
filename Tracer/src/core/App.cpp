#include "App.h"
#include "State.h"
#include "NarrativeManager.h"
#include <SDL.h>
#if __has_include(<SDL2/SDL_ttf.h>)
#include <SDL2/SDL_ttf.h>
#else
#include <SDL_ttf.h>
#endif
#include <memory>
#include "Cards.h"
#include "Deck.h"
#include "../states/MainMenuState.h"
#include "../states/DeckSelectState.h"
#include "../states/MapExploreState.h"
#include "../states/BattleState.h"
#include "../states/VictoryState.h"
#include "../states/MemoryRepairState.h"
#include "../states/HeritageState.h"
#include "../states/EngraveState.h"
#include "../states/InkWorkshopState.h"
#include "../states/InkGhostState.h"
#include "../states/BarterState.h"
#include "../states/RelicPickupState.h"
#include "../states/SeekerState.h"
#include "../states/TemperState.h"
#include "../states/BurnState.h"
#include "../states/CombineState.h"
#include "../states/InkShopState.h"

// 定义静态成员变量
bool App::godMode_ = false;
bool App::temperBlessing_ = false;
std::string App::selectedInitialDeck_ = "";
int App::remainingCandles_ = 2;
// 全局印记提示系统
bool App::showMarkTooltip_ = false;
std::string App::tooltipMarkName_ = "";
std::string App::tooltipDescription_ = "";
int App::tooltipMouseX_ = 0;
int App::tooltipMouseY_ = 0;

App::App() = default;
App::~App() { shutdown(); }

bool App::init(const char* title, int width, int height) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
		SDL_Log("SDL_Init Error: %s", SDL_GetError());
		return false;
	}

	if (TTF_Init() != 0) {
		SDL_Log("TTF_Init Error: %s", TTF_GetError());
		return false;
	}

	window_ = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
	if (!window_) {
		SDL_Log("CreateWindow Error: %s", SDL_GetError());
		return false;
	}

	renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer_) {
		SDL_Log("CreateRenderer Error: %s", SDL_GetError());
		return false;
	}

	// 加载内置卡牌数据库
	CardDB::instance().loadBuiltinCards();
	
	// 初始化叙事管理器
	initializeNarrativeTransitions();
	
	// 注意：不在这里初始化玩家牌堆，等用户选择牌组后再初始化

	running_ = true;
	return true;
}

void App::shutdown() {
	state_.reset();
	if (renderer_) { SDL_DestroyRenderer(renderer_); renderer_ = nullptr; }
	if (window_) { SDL_DestroyWindow(window_); window_ = nullptr; }
	TTF_Quit();
	SDL_Quit();
}

void App::run() {
	Uint64 prev = SDL_GetPerformanceCounter();
	while (running_) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) { running_ = false; }
			if (state_) state_->handleEvent(*this, e);
		}

		Uint64 now = SDL_GetPerformanceCounter();
		double delta = (double)(now - prev) / (double)SDL_GetPerformanceFrequency();
		prev = now;

		if (state_) state_->update(*this, static_cast<float>(delta));

		SDL_SetRenderDrawColor(renderer_, 10, 10, 12, 255);
		SDL_RenderClear(renderer_);
		if (state_) state_->render(*this);
		SDL_RenderPresent(renderer_);
	}
}

void App::setState(std::unique_ptr<State> nextState) {
	if (state_) state_->onExit(*this);
	state_ = std::move(nextState);
	if (state_) state_->onEnter(*this);
}

// 全局印记提示系统实现
void App::showMarkTooltip(const std::string& markName, const std::string& description, int x, int y) {
	tooltipMarkName_ = markName;
	tooltipDescription_ = description;
	tooltipMouseX_ = x;
	tooltipMouseY_ = y;
	showMarkTooltip_ = true;
}

void App::hideMarkTooltip() {
	showMarkTooltip_ = false;
}

void App::getMarkTooltipInfo(std::string& markName, std::string& description, int& x, int& y) {
	markName = tooltipMarkName_;
	description = tooltipDescription_;
	x = tooltipMouseX_;
	y = tooltipMouseY_;
}

void App::renderMarkTooltip() {
	if (!showMarkTooltip_) return;
	
	// 这里需要字体，但我们没有全局字体，所以让各个状态自己处理
	// 或者我们可以在这里实现一个简单的文本渲染
}

void App::initializeNarrativeTransitions() {
	// 主菜单 -> 牌组选择
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::MainMenuToDeckSelect,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<DeckSelectState>(); }
	);
	
	// 牌组选择 -> 地图探索
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::DeckSelectToMapExplore,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MapExploreState>(); }
	);
	
	// 地图探索 -> 战斗
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::MapExploreToBattle,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<BattleState>(); }
	);
	
	// 战斗 -> 地图探索
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::BattleToMapExplore,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MapExploreState>(); }
	);
	
	// 地图探索 -> 节点
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::MapExploreToNode,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MapExploreState>(); }
	);
	
	// 节点 -> 地图探索
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::NodeToMapExplore,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MapExploreState>(); }
	);
	
	// 战斗 -> 胜利
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::BattleToVictory,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<VictoryState>(); }
	);
	
	// 战斗 -> 记忆修复
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::BattleToMemoryRepair,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MemoryRepairState>(true); }
	);
	
	// 地图探索到各个节点
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::MapExploreToHeritage,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<HeritageState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::MapExploreToEngrave,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<EngraveState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::MapExploreToInkWorkshop,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<InkWorkshopState>(0); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::MapExploreToInkGhost,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<InkGhostState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::MapExploreToBarter,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<BarterState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::MapExploreToMemoryRepair,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MemoryRepairState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::MapExploreToRelicPickup,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<RelicPickupState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::MapExploreToSeeker,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<SeekerState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::MapExploreToTemper,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<TemperState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::MapExploreToBurn,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<BurnState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::MapExploreToCombine,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<CombineState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::MapExploreToInkShop,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<InkShopState>(); }
	);
	
	// 各个节点到地图探索
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::HeritageToMapExplore,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MapExploreState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::EngraveToMapExplore,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MapExploreState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::InkWorkshopToMapExplore,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MapExploreState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::InkGhostToMapExplore,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MapExploreState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::BarterToMapExplore,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MapExploreState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::MemoryRepairToMapExplore,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MapExploreState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::RelicPickupToMapExplore,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MapExploreState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::SeekerToMapExplore,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MapExploreState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::TemperToMapExplore,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MapExploreState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::BurnToMapExplore,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MapExploreState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::CombineToMapExplore,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MapExploreState>(); }
	);
	
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::InkShopToMapExplore,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MapExploreState>(); }
	);
	
	// 最终Boss胜利 -> 主菜单
	NarrativeManager::setNarrativeTransition(
		NarrativeManager::NarrativeType::FinalBossVictory,
		"assets/narrative/all_narratives.txt",
		[]() -> std::unique_ptr<State> { return std::make_unique<MainMenuState>(); }
	);
}


