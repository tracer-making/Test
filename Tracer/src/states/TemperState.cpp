#include "TemperState.h"
#include "TestState.h"
#include "MapExploreState.h"
#include "../core/App.h"
#include <random>

TemperState::TemperState() = default;
TemperState::~TemperState() {
	if (titleTex_) SDL_DestroyTexture(titleTex_);
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (smallFont_) TTF_CloseFont(smallFont_);
	delete backButton_;
	delete atkButton_;
	delete hpButton_;
}

void TemperState::onEnter(App& app) {
	int w,h; SDL_GetWindowSize(app.getWindow(), &w, &h); screenW_ = w; screenH_ = h;
	titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 56);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 18);
	if (titleFont_) { SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, u8"淬炼：选择一张手牌", col); if (s) { titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), s); SDL_FreeSurface(s);} }

	backButton_ = new Button(); if (backButton_) { backButton_->setRect({20,20,120,36}); backButton_->setText(u8"返回地图"); if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer()); backButton_->setOnClick([this]() { pendingGoMapExplore_ = true; }); }
	atkButton_ = new Button(); if (atkButton_) { atkButton_->setRect({screenW_/2 - 140, 100, 120, 36}); atkButton_->setText(u8"+1 攻击"); if (smallFont_) atkButton_->setFont(smallFont_, app.getRenderer()); atkButton_->setOnClick([this]() { applyTemper(true); }); }
	hpButton_ = new Button(); if (hpButton_) { hpButton_->setRect({screenW_/2 + 20, 100, 120, 36}); hpButton_->setText(u8"+2 生命"); if (smallFont_) hpButton_->setFont(smallFont_, app.getRenderer()); hpButton_->setOnClick([this]() { applyTemper(false); }); }

	// 使用玩家的实际牌堆
	auto& store = DeckStore::instance();
	if (store.hand().empty()) {
		if (store.library().empty()) {
			// 如果牌库也为空，初始化玩家牌堆
			store.initializePlayerDeck();
		} else {
			// 从牌库抽取一些卡牌到手牌用于淬炼
			int drawCount = std::min(6, (int)store.library().size());
			store.drawToHand(drawCount);
		}
	}

	layoutHandGrid();
}

void TemperState::onExit(App& app) {}

void TemperState::handleEvent(App& app, const SDL_Event& e) {
	if (backButton_) backButton_->handleEvent(e);
	if (atkButton_) atkButton_->handleEvent(e);
	if (hpButton_) hpButton_->handleEvent(e);
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mx=e.button.x, my=e.button.y; for (size_t i=0;i<cardRects_.size(); ++i) { const SDL_Rect& rc = cardRects_[i]; if (mx>=rc.x && mx<=rc.x+rc.w && my>=rc.y && my<=rc.y+rc.h) { selectedIndex_ = (int)i; break; } }
	}
}

void TemperState::update(App& app, float dt) {
	if (pendingBackToTest_) { pendingBackToTest_ = false; app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState()))); return; }
	if (pendingGoMapExplore_) { pendingGoMapExplore_ = false; app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState()))); return; }
}

void TemperState::render(App& app) {
	SDL_Renderer* r = app.getRenderer(); SDL_SetRenderDrawColor(r, 18,22,32,255); SDL_RenderClear(r);
	if (titleTex_) { int tw,th; SDL_QueryTexture(titleTex_,nullptr,nullptr,&tw,&th); SDL_Rect d{ (screenW_-tw)/2, 60, tw, th }; SDL_RenderCopy(r,titleTex_,nullptr,&d); }
	if (backButton_) backButton_->render(r); if (atkButton_) atkButton_->render(r); if (hpButton_) hpButton_->render(r);

	// 手牌渲染（水墨风格与牌库一致）
	auto& hand = DeckStore::instance().hand();
	if (hand.empty() && smallFont_) {
		SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, u8"当前无手牌可淬炼（已为你准备示例卡，返回重进试试）", col, screenW_-40); if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{20, screenH_/2 - s->h/2, s->w, s->h}; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} 
	}
	for (size_t i=0;i<hand.size() && i<cardRects_.size(); ++i) {
		const auto& c = hand[i]; const SDL_Rect& rc = cardRects_[i];
		// 纸面底色
		SDL_SetRenderDrawColor(r, 235,230,220,230); SDL_RenderFillRect(r, &rc);
		// 边框（深墨，选中加粗多层）
		bool sel = ((int)i==selectedIndex_);
		if (sel) SDL_SetRenderDrawColor(r, 0,220,180,255); else SDL_SetRenderDrawColor(r, 60,50,40,220);
		SDL_RenderDrawRect(r, &rc);
		if (sel) { for (int t=1;t<=3;++t){ SDL_Rect rr{ rc.x - t, rc.y - t, rc.w + 2*t, rc.h + 2*t }; SDL_RenderDrawRect(r,&rr);} }
		// 角落小装饰点
		SDL_SetRenderDrawColor(r, 120,110,100,150);
		SDL_Rect dots[4]={{rc.x+4,rc.y+4,2,2},{rc.x+rc.w-6,rc.y+4,2,2},{rc.x+4,rc.y+rc.h-6,2,2},{rc.x+rc.w-6,rc.y+rc.h-6,2,2}};
		for (const auto& d : dots) SDL_RenderFillRect(r,&d);

		if (smallFont_) {
			// 名称（顶部居中，缩放）
			SDL_Color nameCol{50,40,30,255}; SDL_Surface* ns = TTF_RenderUTF8_Blended(smallFont_, c.name.c_str(), nameCol);
			if (ns) {
				SDL_Texture* nt = SDL_CreateTextureFromSurface(r, ns);
				int nameH = SDL_max(12, (int)(rc.h * 0.16f)); float nscale = (float)nameH / (float)ns->h; int nW = (int)(ns->w * nscale);
				SDL_Rect nd{ rc.x + (rc.w - nW)/2, rc.y + (int)(rc.h*0.05f), nW, nameH };
				SDL_RenderCopy(r, nt, nullptr, &nd);
				SDL_DestroyTexture(nt);
				// 分割线
				SDL_SetRenderDrawColor(r, 80,70,60,220);
				int lineY = nd.y + nd.h + SDL_max(2, (int)(rc.h*0.015f)); int thick = SDL_max(1, (int)(rc.h*0.007f));
				for (int k=0;k<thick;++k) SDL_RenderDrawLine(r, rc.x+6, lineY+k, rc.x+rc.w-6, lineY+k);
				SDL_FreeSurface(ns);
			}

			// 攻击/生命（底部左右，缩放；HP深红）
			int statH = SDL_max(12, (int)(rc.h * 0.18f)); int margin = SDL_max(6, (int)(rc.h * 0.035f)); char buf[32];
			// 攻击
			SDL_Color atkCol{80,50,40,255}; snprintf(buf, sizeof(buf), "%d", c.attack); SDL_Surface* sa = TTF_RenderUTF8_Blended(smallFont_, buf, atkCol);
			if (sa) { SDL_Texture* ta = SDL_CreateTextureFromSurface(r, sa); float sc=(float)statH/(float)sa->h; int w=(int)(sa->w*sc); SDL_Rect ad{ rc.x + margin, rc.y + rc.h - statH - margin, w, statH }; SDL_RenderCopy(r, ta, nullptr, &ad); SDL_DestroyTexture(ta); SDL_FreeSurface(sa);} 
			// 生命（深红）
			SDL_Color hpCol{160,30,40,255}; snprintf(buf, sizeof(buf), "%d", c.health); SDL_Surface* sh = TTF_RenderUTF8_Blended(smallFont_, buf, hpCol);
			if (sh) { SDL_Texture* th = SDL_CreateTextureFromSurface(r, sh); float sc=(float)statH/(float)sh->h; int w=(int)(sh->w*sc); SDL_Rect hd{ rc.x + rc.w - w - margin, rc.y + rc.h - statH - margin, w, statH }; SDL_RenderCopy(r, th, nullptr, &hd); SDL_DestroyTexture(th); SDL_FreeSurface(sh);} 

			// 撕卡概率显示（位于底部数值之上）
			TemperInfo info = temperMap_[c.id];
			std::string probText;
			if (info.successes < 2) probText = u8"撕卡概率：0%（前两次保底）";
			else {
				int pct = (int)((info.failProb > 0.0f ? info.failProb : 0.30f) * 100.0f + 0.5f);
				probText = u8"撕卡概率：" + std::to_string(pct) + "%";
			}
			SDL_Color prCol{100,90,80,220}; SDL_Surface* ps = TTF_RenderUTF8_Blended_Wrapped(smallFont_, probText.c_str(), prCol, rc.w - 12);
			if (ps) {
				SDL_Texture* pt = SDL_CreateTextureFromSurface(r, ps);
				int pH = SDL_max(10, (int)(rc.h * 0.10f)); float pS = (float)pH / (float)ps->h; int pW = (int)(ps->w * pS);
				int py = rc.y + rc.h - statH - margin - pH - 4;
				SDL_Rect pd{ rc.x + 6, py, pW, pH };
				SDL_RenderCopy(r, pt, nullptr, &pd);
				SDL_DestroyTexture(pt); SDL_FreeSurface(ps);
			}
		}
	}

	if (!message_.empty() && smallFont_) { SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, message_.c_str(), col, screenW_-40); if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{20, screenH_-s->h-20, s->w, s->h}; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} }
}

void TemperState::layoutHandGrid() {
	int n = (int)DeckStore::instance().hand().size(); cardRects_.assign(n, SDL_Rect{0,0,0,0}); if (n<=0) return;
	int marginX=40, topY=160, bottom=40; int availW = SDL_max(200, screenW_-marginX*2); int availH = SDL_max(160, screenH_-topY-bottom);
	const float aspect=2.0f/3.0f; int gap=12; int bestCols=1,bestW=0,bestH=0,bestRows=n;
	for (int cols = SDL_min(n,8); cols>=1; --cols) { int rows=(n+cols-1)/cols; int cw=(availW-(cols-1)*gap)/cols; if (cw<=20) continue; int ch=(int)(cw/aspect); int totalH=rows*ch+(rows-1)*gap; if (totalH<=availH && cw>bestW) { bestW=cw; bestH=ch; bestCols=cols; bestRows=rows; } }
	if (bestW==0) { int cols=SDL_min(n, SDL_max(1, availW/80)); int rows=(n+cols-1)/cols; int cw=(availW-(cols-1)*gap)/cols; int ch=(int)(cw/aspect); float s=(float)availH/(rows*ch+(rows-1)*gap); bestW=SDL_max(24,(int)(cw*s)); bestH=SDL_max(36,(int)(ch*s)); bestCols=cols; bestRows=rows; }
	int totalW = bestCols*bestW+(bestCols-1)*gap; int startX=(screenW_-totalW)/2; int startY=topY + (availH - (bestRows*bestH + (bestRows-1)*gap))/2; startY = SDL_max(topY, startY);
	for (int i=0;i<n;++i) { int r=i/bestCols, c=i%bestCols; cardRects_[i] = { startX + c*(bestW+gap), startY + r*(bestH+gap), bestW, bestH }; }
}

void TemperState::applyTemper(bool addAttack) {
	auto& hand = DeckStore::instance().hand();
	if (selectedIndex_ < 0 || selectedIndex_ >= (int)hand.size()) { message_ = u8"请先选择一张手牌"; return; }
	Card& card = hand[selectedIndex_];
	TemperInfo& info = temperMap_[card.id];
	// 前两次必定成功；两次后启用撕卡概率，且每次成功后概率提高
	bool mustSuccess = (info.successes < 2);
	bool success = true;
	if (!mustSuccess) {
		// 失败概率 failProb，初始 0.30，每次成功后+0.10，上限0.6
		if (info.failProb <= 0.0f) info.failProb = 0.30f;
		std::random_device rd; std::mt19937 gen(rd()); std::uniform_real_distribution<float> d(0.0f, 1.0f);
		float r = d(gen);
		success = (r > info.failProb);
	}
	if (success) {
		if (addAttack) card.attack += 1; else card.health += 2;
		info.successes += 1;
		if (!mustSuccess) { info.failProb = std::min(0.6f, info.failProb + 0.10f); }
		message_ = addAttack ? u8"强化成功：攻击+1" : u8"强化成功：生命+2";
	} else {
		// 撕卡：移除选中的手牌
		DeckStore::instance().discard().push_back(card); // 也可直接丢弃
		hand.erase(hand.begin() + selectedIndex_);
		selectedIndex_ = -1;
		layoutHandGrid();
		message_ = u8"强化失败，卡牌被撕毁";
	}
}


