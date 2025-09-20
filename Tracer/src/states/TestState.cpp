#include "TestState.h"
#include "MainMenuState.h"
#include "BattleState.h"
#include "MapExploreState.h"
#include "DeckState.h"
#include "HeritageState.h"
#include "EngraveState.h"
#include "InkShopState.h"
#include "MemoryRepairState.h"
#include "RelicPickupState.h"
#include "TemperState.h"
#include "SeekerState.h"
#include "BurnState.h"
#include "BarterState.h"
#include "CardBrowserState.h"
#include "../core/App.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <memory>

TestState::TestState() = default;
TestState::~TestState() {
	if (font_) TTF_CloseFont(font_);
	if (smallFont_) TTF_CloseFont(smallFont_);
	if (titleTex_) SDL_DestroyTexture(titleTex_);
	
	for (auto* button : testButtons_) {
		delete button;
	}
	delete backButton_;
}

void TestState::onEnter(App& app) {
	SDL_Log("TestState::onEnter - Start");
	
	// 设置窗口尺寸（适中尺寸）
	screenW_ = 1600;
	screenH_ = 1000;
	SDL_SetWindowSize(app.getWindow(), screenW_, screenH_);
	SDL_Log("Screen size: %d x %d", screenW_, screenH_);

	// 加载字体
	font_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 80);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 20);
	if (!font_ || !smallFont_) {
		SDL_Log("TTF_OpenFont failed: %s", TTF_GetError());
		return;
	}
	else {
		SDL_Log("Fonts loaded successfully");
		SDL_Color titleCol{ 200, 230, 255, 255 };
		SDL_Surface* ts = TTF_RenderUTF8_Blended(font_, u8"功能测试界面", titleCol);
		if (ts) {
			titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), ts);
			titleW_ = ts->w;
			titleH_ = ts->h;
			SDL_FreeSurface(ts);
			SDL_Log("Title texture created: %d x %d", titleW_, titleH_);
		}
	}

	// 创建返回按钮（右上角）
	backButton_ = new Button();
	if (!backButton_) {
		SDL_Log("Failed to create back button");
		return;
	}
	SDL_Log("Back button created");
	
	int backButtonSize = 40;
	SDL_Rect backButtonRect{ screenW_ - backButtonSize - 20, 20, backButtonSize, backButtonSize };
	backButton_->setRect(backButtonRect);
	backButton_->setText(u8"X");
	if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer());
	backButton_->setOnClick([&app]() {
		app.setState(std::unique_ptr<State>(static_cast<State*>(new MainMenuState())));
		});
	SDL_Log("Back button setup complete");

	// 按钮标签：保留前三个入口，其余替换为功能测试入口
	std::vector<std::string> buttonLabels = {
		u8"主菜单",
		u8"战斗界面",
		u8"地图探索",
		u8"文脉传承",
		u8"意境刻画",
		u8"诗剑之争/意境之斗",
		u8"墨坊",
		u8"记忆修复",
		u8"墨宝拾遗",
		u8"淬炼",
		u8"寻物人",
		u8"以物易物",
		u8"焚书",
		u8"文心试炼",
		u8"合卷",
		u8"卡牌图鉴",
		u8"牌库"
	};
	SDL_Log("Button labels initialized: %zu buttons", buttonLabels.size());

	// 创建测试按钮网格
	int buttonWidth = 200;
	int buttonHeight = 45;
	int buttonMargin = 20;
	int gridColumns = 2;
	int gridRows = static_cast<int>((buttonLabels.size() + gridColumns - 1) / gridColumns);
	
	int gridWidth = gridColumns * buttonWidth + (gridColumns - 1) * buttonMargin;
	int gridHeight = gridRows * buttonHeight + (gridRows - 1) * buttonMargin;
	int gridX = (screenW_ - gridWidth) / 2;
	int gridY = (screenH_ - gridHeight) / 2 + 60;
	SDL_Log("Grid layout: %d columns, %d rows, position: %d, %d", gridColumns, gridRows, gridX, gridY);

	for (size_t i = 0; i < buttonLabels.size(); ++i) {
		int row = static_cast<int>(i) / gridColumns;
		int col = i % gridColumns;
		
		SDL_Rect buttonRect{
			gridX + col * (buttonWidth + buttonMargin),
			gridY + row * (buttonHeight + buttonMargin),
			buttonWidth,
			buttonHeight
		};

		Button* button = new Button();
		if (!button) {
			SDL_Log("Failed to create button %zu", i);
			continue;
		}
		
		button->setRect(buttonRect);
		button->setText(buttonLabels[i]);
		if (smallFont_) button->setFont(smallFont_, app.getRenderer());
		
		// 不在此处设置回调，统一在 handleEvent 中处理

		testButtons_.push_back(button);
		SDL_Log("Button %zu created and added", i);
	}
	
	SDL_Log("TestState::onEnter - Complete, %zu test buttons created", testButtons_.size());
}

void TestState::handleEvent(App& app, const SDL_Event& e) {
	// 处理返回按钮事件
	if (backButton_) backButton_->handleEvent(e);
	
	// 处理测试按钮事件
	for (auto* button : testButtons_) {
		if (button) button->handleEvent(e);
	}

	// 特殊处理测试按钮点击（避免lambda生命周期问题）
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mx = e.button.x, my = e.button.y;
		
		// 检查每个测试按钮
		for (size_t i = 0; i < testButtons_.size(); ++i) {
			if (testButtons_[i]) {
				const SDL_Rect& rect = testButtons_[i]->getRect();
				if (mx >= rect.x && mx <= rect.x + rect.w &&
					my >= rect.y && my <= rect.y + rect.h) {
					// 只记录待切换目标，在 update 中执行
					switch (i) {
					case 0: pendingTarget_ = 0; break; // 主菜单
					case 1: pendingTarget_ = 1; break; // 战斗
					case 2: pendingTarget_ = 2; break; // 地图
					case 3: pendingTarget_ = 3; break; // 文脉传承
					case 4: pendingTarget_ = 4; break; // 意境刻画
					case 16: pendingTarget_ = 16; break; // 牌库
					case 6: pendingTarget_ = 6; break; // 墨坊
					case 7: pendingTarget_ = 7; break; // 记忆修复
					case 8: pendingTarget_ = 8; break; // 墨宝拾遗
					case 9: pendingTarget_ = 9; break; // 淬炼
					case 10: pendingTarget_ = 10; break; // 寻物人
        case 11: pendingTarget_ = 11; break; // 以物易物
        case 12: pendingTarget_ = 12; break; // 焚书
        case 13: pendingTarget_ = 13; break; // 文心试炼
        case 14: pendingTarget_ = 14; break; // 合卷
        case 15: pendingTarget_ = 15; break; // 卡牌图鉴
					default: break;
					}
				}
			}
		}
	}
}

void TestState::update(App& app, float dt) {
	// 延迟切换状态，避免在事件处理中销毁当前对象
	if (pendingTarget_ != -1) {
		int t = pendingTarget_;
		pendingTarget_ = -1;
		switch (t) {
		case 0:
			app.setState(std::unique_ptr<State>(static_cast<State*>(new MainMenuState())));
			break;
		case 1:
			app.setState(std::unique_ptr<State>(static_cast<State*>(new BattleState())));
			break;
		case 2:
			app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState())));
			break;
		case 3:
			app.setState(std::unique_ptr<State>(static_cast<State*>(new HeritageState())));
			break;
		case 4:
			app.setState(std::unique_ptr<State>(static_cast<State*>(new EngraveState())));
			break;
		case 6:
			app.setState(std::unique_ptr<State>(static_cast<State*>(new InkShopState())));
			break;
		case 7:
			app.setState(std::unique_ptr<State>(static_cast<State*>(new MemoryRepairState())));
			break;
		case 8:
			app.setState(std::unique_ptr<State>(static_cast<State*>(new RelicPickupState())));
			break;
		case 9:
			app.setState(std::unique_ptr<State>(static_cast<State*>(new TemperState())));
			break;
		case 10:
			app.setState(std::unique_ptr<State>(static_cast<State*>(new SeekerState())));
			break;
		case 11:
			app.setState(std::unique_ptr<State>(static_cast<State*>(new BarterState())));
			break;
		case 12:
			app.setState(std::unique_ptr<State>(static_cast<State*>(new BurnState())));
			break;
		case 13:
			// 文心试炼 - 暂未实现
			break;
		case 14:
			// 合卷 - 暂未实现
			break;
		case 15:
			app.setState(std::unique_ptr<State>(static_cast<State*>(new CardBrowserState())));
			break;
		case 16:
			app.setState(std::unique_ptr<State>(static_cast<State*>(new DeckState())));
			break;
		default:
			break;
		}
	}
}

void TestState::render(App& app) {
	SDL_Renderer* r = app.getRenderer();
	
	// 深色背景
	SDL_SetRenderDrawColor(r, 15, 20, 30, 255);
	SDL_Rect bg{ 0, 0, screenW_, screenH_ };
	SDL_RenderFillRect(r, &bg);

	// 渲染标题
	if (titleTex_) {
		SDL_Rect dst{ screenW_ / 2 - titleW_ / 2, 80, titleW_, titleH_ };
		SDL_RenderCopy(r, titleTex_, nullptr, &dst);
	}

	// 渲染返回按钮
	if (backButton_) backButton_->render(r);
	
	// 渲染测试按钮
	for (auto* button : testButtons_) {
		if (button) button->render(r);
	}
}
