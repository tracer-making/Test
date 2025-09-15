#include "SeekerState.h"
#include "TestState.h"
#include "../core/App.h"
#include <random>

SeekerState::SeekerState() = default;
SeekerState::~SeekerState() {
	if (titleTex_) SDL_DestroyTexture(titleTex_);
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (smallFont_) TTF_CloseFont(smallFont_);
	delete backButton_;
	delete confirmButton_;
}

void SeekerState::onEnter(App& app) {
	int w,h; SDL_GetWindowSize(app.getWindow(), &w, &h); screenW_ = w; screenH_ = h;
	titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 56);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 18);
	if (titleFont_) { SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, u8"寻物人：选择一张手牌破壁", col); if (s) { titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), s); SDL_FreeSurface(s);} }

	backButton_ = new Button(); if (backButton_) { backButton_->setRect({20,20,120,36}); backButton_->setText(u8"返回测试"); if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer()); backButton_->setOnClick([this]() { pendingBackToTest_ = true; }); }
	confirmButton_ = new Button(); if (confirmButton_) { confirmButton_->setRect({screenW_-160,20,140,36}); confirmButton_->setText(u8"破壁"); if (smallFont_) confirmButton_->setFont(smallFont_, app.getRenderer()); confirmButton_->setOnClick([this]() {
		if (selectedIndex_ < 0 || selectedIndex_ >= (int)DeckStore::instance().hand().size()) { message_ = u8"请先选择一张手牌"; return; }
		// 移除选中卡作为代价
		auto& store = DeckStore::instance(); Card sacrificed = store.hand()[selectedIndex_]; store.discard().push_back(sacrificed); store.hand().erase(store.hand().begin()+selectedIndex_); selectedIndex_=-1; layoutHandGrid();
		// 50% 传奇之墨，50% 传奇卡
		std::random_device rd; std::mt19937 gen(rd()); std::uniform_int_distribution<int> b(0,1);
		if (b(gen)==0) { message_ = u8"寻得：传奇之墨（占位）"; }
		else { Card c; c.id = "L"+std::to_string((int)store.library().size()+1); c.name = u8"传奇·天书"; c.attack = 5; c.health = 7; store.library().push_back(c); message_ = u8"寻得：传奇卡牌『传奇·天书』(加入牌库)"; }
	}); }

	// 若无手牌，准备示例
	if (DeckStore::instance().hand().empty() && DeckStore::instance().library().empty()) {
		for (int i=0;i<6;++i) { Card c; c.id = "S"+std::to_string(i+1); c.name = "手牌"+std::to_string(i+1); c.attack = 1+i%3; c.health = 3+i%4; DeckStore::instance().addToLibrary(c); }
		DeckStore::instance().drawToHand(6);
	}

	layoutHandGrid();
}

void SeekerState::onExit(App& app) {}

void SeekerState::handleEvent(App& app, const SDL_Event& e) {
	if (backButton_) backButton_->handleEvent(e);
	if (confirmButton_) confirmButton_->handleEvent(e);
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mx=e.button.x, my=e.button.y; for (size_t i=0;i<cardRects_.size(); ++i) { const SDL_Rect& rc = cardRects_[i]; if (mx>=rc.x && mx<=rc.x+rc.w && my>=rc.y && my<=rc.y+rc.h) { selectedIndex_=(int)i; break; } }
	}
}

void SeekerState::update(App& app, float dt) { if (pendingBackToTest_) { pendingBackToTest_ = false; app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState()))); return; } }

void SeekerState::render(App& app) {
	SDL_Renderer* r = app.getRenderer(); SDL_SetRenderDrawColor(r, 18,22,32,255); SDL_RenderClear(r);
	if (titleTex_) { int tw,th; SDL_QueryTexture(titleTex_,nullptr,nullptr,&tw,&th); SDL_Rect d{ (screenW_-tw)/2, 60, tw, th }; SDL_RenderCopy(r,titleTex_,nullptr,&d); }
	if (backButton_) backButton_->render(r); if (confirmButton_) confirmButton_->render(r);
	// 手牌
	auto& hand = DeckStore::instance().hand();
	for (size_t i=0;i<hand.size() && i<cardRects_.size(); ++i) {
		const auto& c = hand[i]; const SDL_Rect& rc = cardRects_[i];
		SDL_SetRenderDrawColor(r, 235,230,220,230); SDL_RenderFillRect(r,&rc);
		if ((int)i==selectedIndex_) { SDL_SetRenderDrawColor(r, 255,215,0,255); for (int t=0;t<3;++t){ SDL_Rect rr{ rc.x - t, rc.y - t, rc.w + 2*t, rc.h + 2*t}; SDL_RenderDrawRect(r,&rr);} }
		else { SDL_SetRenderDrawColor(r, 60,50,40,220); SDL_RenderDrawRect(r,&rc); }
		if (smallFont_) { std::string line = c.name + "  " + std::to_string(c.attack) + "/" + std::to_string(c.health); SDL_Color col{50,40,30,255}; SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, line.c_str(), col, rc.w-12); if (s){ SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{ rc.x+6, rc.y+6, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} }
	}
	if (!message_.empty() && smallFont_) { SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, message_.c_str(), col, screenW_-40); if (s){ SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{20, screenH_-s->h-20, s->w, s->h}; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} }
}

void SeekerState::layoutHandGrid() {
	int n = (int)DeckStore::instance().hand().size(); cardRects_.assign(n, SDL_Rect{0,0,0,0}); if (n<=0) return;
	int marginX=40, topY=160, bottom=40; int availW = SDL_max(200, screenW_-marginX*2); int availH = SDL_max(160, screenH_-topY-bottom);
	const float aspect=2.0f/3.0f; int gap=12; int bestCols=1,bestW=0,bestH=0,bestRows=n;
	for (int cols = SDL_min(n,8); cols>=1; --cols) { int rows=(n+cols-1)/cols; int cw=(availW-(cols-1)*gap)/cols; if (cw<=20) continue; int ch=(int)(cw/aspect); int totalH=rows*ch+(rows-1)*gap; if (totalH<=availH && cw>bestW) { bestW=cw; bestH=ch; bestCols=cols; bestRows=rows; } }
	if (bestW==0) { int cols=SDL_min(n, SDL_max(1, availW/80)); int rows=(n+cols-1)/cols; int cw=(availW-(cols-1)*gap)/cols; int ch=(int)(cw/aspect); float s=(float)availH/(rows*ch+(rows-1)*gap); bestW=SDL_max(24,(int)(cw*s)); bestH=SDL_max(36,(int)(ch*s)); bestCols=cols; bestRows=rows; }
	int totalW = bestCols*bestW+(bestCols-1)*gap; int startX=(screenW_-totalW)/2; int startY=topY + (availH - (bestRows*bestH + (bestRows-1)*gap))/2; startY = SDL_max(topY, startY);
	for (int i=0;i<n;++i) { int r=i/bestCols, c=i%bestCols; cardRects_[i] = { startX + c*(bestW+gap), startY + r*(bestH+gap), bestW, bestH }; }
}


