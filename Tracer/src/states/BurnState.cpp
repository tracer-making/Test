#include "BurnState.h"
#include "TestState.h"
#include "../core/App.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include "../ui/CardRenderer.h"

BurnState::BurnState() = default;
BurnState::~BurnState() {
	if (titleTex_) SDL_DestroyTexture(titleTex_);
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (smallFont_) TTF_CloseFont(smallFont_);
	if (nameFont_) TTF_CloseFont(nameFont_);
	if (statFont_) TTF_CloseFont(statFont_);
	delete backButton_;
	delete burnButton_;
}

void BurnState::onEnter(App& app) {
	// 设置窗口尺寸（适中尺寸）
	screenW_ = 1600;
	screenH_ = 1000;
	SDL_SetWindowSize(app.getWindow(), screenW_, screenH_);
	titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 64);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 16);
	nameFont_  = TTF_OpenFont("assets/fonts/Sanji.ttf", 22);
	statFont_  = TTF_OpenFont("assets/fonts/Sanji.ttf", 24);
	if (titleFont_) {
		SDL_Color col{200,230,255,255};
		SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, u8"焚书", col);
		if (s) { titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), s); SDL_FreeSurface(s);} 
	}

	backButton_ = new Button();
	if (backButton_) {
		backButton_->setRect({20,20,120,36});
		backButton_->setText(u8"返回测试");
		if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer());
		backButton_->setOnClick([this]() { pendingBackToTest_ = true; });
	}

	burnButton_ = new Button();
	if (burnButton_) {
		burnButton_->setRect({screenW_/2-60, screenH_-100, 120, 40});
		burnButton_->setText(u8"焚毁之仪");
		if (smallFont_) burnButton_->setFont(smallFont_, app.getRenderer());
		burnButton_->setOnClick([this]() {
			auto& lib = DeckStore::instance().library();
			if (selectedIndex_ >= 0 && selectedIndex_ < (int)lib.size()) {
				// 摧毁所选卡：从牌库移除
				lib.erase(lib.begin() + selectedIndex_);
				message_ = u8"焚毁成功：卡牌已从牌组移除";
				selectedIndex_ = -1;
				layoutGridFromLibrary();
			} else {
				message_ = u8"请先从牌组中选择一张卡牌";
			}
		});
	}

	// 如果牌库为空，注入一些示例卡牌以便演示
	auto& lib = DeckStore::instance().library();
	if (lib.empty()) {
		Card c1; c1.id="demo_sword"; c1.name=u8"残锋"; c1.attack=3; c1.health=2;
		Card c2; c2.id="demo_poem";  c2.name=u8"柳絮"; c2.attack=1; c2.health=4;
		Card c3; c3.id="demo_ink";   c3.name=u8"墨痕"; c3.attack=2; c3.health=3;
		Card c4; c4.id="demo_seal";  c4.name=u8"旧印"; c4.attack=1; c4.health=5;
		Card c5; c5.id="demo_scroll";c5.name=u8"残卷"; c5.attack=2; c5.health=2;
		lib.push_back(c1); lib.push_back(c2); lib.push_back(c3); lib.push_back(c4); lib.push_back(c5);
		message_ = u8"提示：牌库为空，已注入示例卡用于焚书演示";
	}

	layoutGridFromLibrary();
}

void BurnState::onExit(App& app) {}

void BurnState::handleEvent(App& app, const SDL_Event& e) {
	if (backButton_) backButton_->handleEvent(e);
	if (burnButton_) burnButton_->handleEvent(e);

	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mx = e.button.x, my = e.button.y;
		auto& lib = DeckStore::instance().library();
		for (size_t i=0; i<cardRects_.size() && i<lib.size(); ++i) {
			const SDL_Rect& rc = cardRects_[i];
			if (mx>=rc.x && mx<=rc.x+rc.w && my>=rc.y && my<=rc.y+rc.h) {
				selectedIndex_ = (int)i;
				break;
			}
		}
	}
}

void BurnState::update(App& app, float dt) {
	if (pendingBackToTest_) {
		pendingBackToTest_ = false;
		app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState())));
	}
}

void BurnState::render(App& app) {
	SDL_Renderer* r = app.getRenderer();
	SDL_SetRenderDrawColor(r, 20, 24, 34, 255);
	SDL_RenderClear(r);
	
	// 水墨背景点
	SDL_SetRenderDrawColor(r, 80, 70, 60, 60);
	srand(1314);
	for (int i=0;i<400;++i) {
		int x = rand()%screenW_;
		int y = rand()%screenH_;
		SDL_RenderDrawPoint(r, x, y);
	}

	if (titleTex_) {
		int tw,th; SDL_QueryTexture(titleTex_,nullptr,nullptr,&tw,&th);
		SDL_Rect dst{ (screenW_-tw)/2, 80, tw, th };
		SDL_RenderCopy(r, titleTex_, nullptr, &dst);
	}

	if (smallFont_) {
		SDL_Color col{210, 200, 180, 255};
		SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, u8"从牌组中选择一张卡牌焚毁，以火净化", col);
		if (s) { SDL_Texture* t = SDL_CreateTextureFromSurface(r, s); SDL_Rect d{ screenW_/2 - s->w/2, 150, s->w, s->h }; SDL_RenderCopy(r, t, nullptr, &d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} 
	}

    // 渲染牌组（库）为统一卡面
	auto& lib2 = DeckStore::instance().library();
	for (size_t i=0; i<cardRects_.size() && i<lib2.size(); ++i) {
		SDL_Rect rect = cardRects_[i];
		const Card& card = lib2[i];

		if (selectedIndex_ == (int)i) {
			SDL_SetRenderDrawColor(r, 255, 120, 40, 180);
			SDL_Rect hl{ rect.x-3, rect.y-3, rect.w+6, rect.h+6};
			SDL_RenderFillRect(r, &hl);
		}

        CardRenderer::renderCard(app, card, rect, nameFont_, statFont_, selectedIndex_ == (int)i);
	}

	if (backButton_) backButton_->render(r);
	if (burnButton_) burnButton_->render(r);

	if (!message_.empty() && smallFont_) {
		SDL_Color mcol{250, 220, 90, 255};
		SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, message_.c_str(), mcol);
		if (s) { SDL_Texture* t = SDL_CreateTextureFromSurface(r, s); SDL_Rect d{ screenW_/2 - s->w/2, screenH_-150, s->w, s->h }; SDL_RenderCopy(r, t, nullptr, &d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} 
	}
}

void BurnState::layoutGridFromLibrary() {
	auto& lib = DeckStore::instance().library();
	int n = (int)lib.size();
	cardRects_.clear(); cardRects_.resize(n);
	if (n<=0) return;
	int marginX = 40; int topY = 200; int bottomMargin = 110;
	int availableW = SDL_max(200, screenW_ - marginX * 2);
	int availableH = SDL_max(160, screenH_ - topY - bottomMargin);
	const float aspect = 2.0f/3.0f; int gap = 12;
	int bestCols = SDL_min(n, 5), bestCardW=0, bestCardH=0, bestRows=n;
	for (int cols = SDL_min(n,10); cols>=1; --cols) {
		int rows = (n + cols - 1) / cols;
		int cw = (availableW - (cols - 1) * gap) / cols; if (cw<=20) continue; int ch = (int)(cw / aspect);
		int totalH = rows * ch + (rows - 1) * gap;
		if (totalH <= availableH && cw > bestCardW) { bestCardW=cw; bestCardH=ch; bestCols=cols; bestRows=rows; }
	}
	if (bestCardW==0) { int cols = SDL_min(n, SDL_max(1, availableW/80)); int rows=(n+cols-1)/cols; int cw=(availableW-(cols-1)*gap)/cols; int ch=(int)(cw/aspect); int totalH = rows*ch+(rows-1)*gap; if (totalH>availableH) { float sc=(float)availableH/(float)totalH; cw=SDL_max(24,(int)(cw*sc)); ch=SDL_max(36,(int)(ch*sc)); } bestCols=cols; bestRows=rows; bestCardW=cw; bestCardH=ch; }
	int totalW = bestCols * bestCardW + (bestCols - 1)*gap;
	int startX = (screenW_ - totalW)/2;
	int startY = topY + (availableH - (bestRows*bestCardH + (bestRows-1)*gap))/2; startY = SDL_max(topY, startY);
	for (int i=0;i<n;++i) { int r=i/bestCols, c=i%bestCols; cardRects_[i] = { startX + c*(bestCardW+gap), startY + r*(bestCardH+gap), bestCardW, bestCardH }; }
}
