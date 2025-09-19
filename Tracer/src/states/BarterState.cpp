#include "BarterState.h"
#include "TestState.h"
#include "../core/App.h"
#include <SDL.h>
#include <SDL_ttf.h>

BarterState::BarterState() = default;
BarterState::~BarterState() {
	if (titleTex_) SDL_DestroyTexture(titleTex_);
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (smallFont_) TTF_CloseFont(smallFont_);
	if (nameFont_) TTF_CloseFont(nameFont_);
	if (statFont_) TTF_CloseFont(statFont_);
	delete backButton_;
	delete confirmButton_;
}

void BarterState::onEnter(App& app) {
	int w,h; SDL_GetWindowSize(app.getWindow(), &w, &h); screenW_ = w; screenH_ = h;
	titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 64);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 16);
	nameFont_  = TTF_OpenFont("assets/fonts/Sanji.ttf", 22);
	statFont_  = TTF_OpenFont("assets/fonts/Sanji.ttf", 24);
	if (titleFont_) { SDL_Color col{200,230,255,255}; SDL_Surface* s=TTF_RenderUTF8_Blended(titleFont_, u8"以物易物", col); if (s){ titleTex_=SDL_CreateTextureFromSurface(app.getRenderer(), s); SDL_FreeSurface(s);} }

	backButton_ = new Button(); if (backButton_) { backButton_->setRect({20,20,120,36}); backButton_->setText(u8"返回测试"); if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer()); backButton_->setOnClick([this](){ pendingBackToTest_ = true; }); }
	confirmButton_ = new Button(); if (confirmButton_) { confirmButton_->setRect({screenW_/2-60, screenH_-100, 120, 40}); confirmButton_->setText(u8"成交"); if (smallFont_) confirmButton_->setFont(smallFont_, app.getRenderer()); confirmButton_->setOnClick([this](){
		auto& lib = DeckStore::instance().library();
		if (selectedLibraryIndex_>=0 && selectedLibraryIndex_<(int)lib.size() && selectedOfferIndex_>=0 && selectedOfferIndex_<(int)offers_.size()) {
			Card incoming = offers_[selectedOfferIndex_].card;
			lib.erase(lib.begin()+selectedLibraryIndex_);
			lib.push_back(incoming);
			message_ = u8"交易完成！已替换卡牌";
			selectedLibraryIndex_ = -1; selectedOfferIndex_ = -1; generateOffers(); layoutGrids();
		} else {
			message_ = u8"请先选择要交换的牌与报价";
		}
	}); }

	ensureDemoLibraryIfEmpty();
	generateOffers();
	layoutGrids();
}

void BarterState::onExit(App& app) {}

void BarterState::handleEvent(App& app, const SDL_Event& e) {
	if (backButton_) backButton_->handleEvent(e);
	if (confirmButton_) confirmButton_->handleEvent(e);
	if (e.type==SDL_MOUSEBUTTONDOWN && e.button.button==SDL_BUTTON_LEFT) {
		int mx=e.button.x, my=e.button.y;
		auto& lib = DeckStore::instance().library();
		for (size_t i=0;i<libraryRects_.size() && i<lib.size(); ++i) {
			const SDL_Rect& rc = libraryRects_[i];
			if (mx>=rc.x && mx<=rc.x+rc.w && my>=rc.y && my<=rc.y+rc.h) { selectedLibraryIndex_=(int)i; }
		}
		for (size_t i=0;i<offers_.size(); ++i) {
			const SDL_Rect& rc = offers_[i].rect;
			if (mx>=rc.x && mx<=rc.x+rc.w && my>=rc.y && my<=rc.y+rc.h) { selectedOfferIndex_=(int)i; }
		}
	}
}

void BarterState::update(App& app, float dt) {
	if (pendingBackToTest_) { pendingBackToTest_=false; app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState()))); }
}

void BarterState::render(App& app) {
	SDL_Renderer* r = app.getRenderer();
	SDL_SetRenderDrawColor(r, 20, 24, 34, 255);
	SDL_RenderClear(r);
	SDL_SetRenderDrawColor(r, 80, 70, 60, 60); srand(4243); for (int i=0;i<400;++i){ SDL_RenderDrawPoint(r, rand()%screenW_, rand()%screenH_);} 
	if (titleTex_) { int tw,th; SDL_QueryTexture(titleTex_,nullptr,nullptr,&tw,&th); SDL_Rect dst{ (screenW_-tw)/2, 80, tw, th }; SDL_RenderCopy(r, titleTex_, nullptr, &dst);} 

	// 左侧：牌库
	auto& lib = DeckStore::instance().library();
	for (size_t i=0;i<libraryRects_.size() && i<lib.size(); ++i) {
		SDL_Rect rect = libraryRects_[i]; const Card& card = lib[i];
		if (selectedLibraryIndex_==(int)i) { SDL_SetRenderDrawColor(r, 120, 180, 255, 120); SDL_Rect hl{rect.x-3,rect.y-3,rect.w+6,rect.h+6}; SDL_RenderFillRect(r,&hl);} 
		SDL_SetRenderDrawColor(r, 235,230,220,230); SDL_RenderFillRect(r,&rect);
		SDL_SetRenderDrawColor(r, 60,50,40,220); SDL_RenderDrawRect(r,&rect);
		if (nameFont_) { SDL_Color nameCol{50,40,30,255}; SDL_Surface* s=TTF_RenderUTF8_Blended(nameFont_, card.name.c_str(), nameCol); if (s){ SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); int h=SDL_max(12,(int)(rect.h*0.16f)); float sc=(float)h/(float)s->h; int wsc=(int)(s->w*sc); int nx=rect.x+(rect.w-wsc)/2; SDL_Rect nd{nx, rect.y+(int)(rect.h*0.06f), wsc, h}; SDL_RenderCopy(r,t,nullptr,&nd); SDL_DestroyTexture(t); SDL_FreeSurface(s);} }
		if (statFont_) { char buf[32]; int h=SDL_max(12,(int)(rect.h*0.18f)); int m=SDL_max(6,(int)(rect.h*0.035f)); SDL_Color col{80,50,40,255};
			snprintf(buf,sizeof(buf),"%d", card.attack); SDL_Surface* sa=TTF_RenderUTF8_Blended(statFont_, buf, col); if (sa){ SDL_Texture* ta=SDL_CreateTextureFromSurface(r,sa); float sc=(float)h/(float)sa->h; int wsc=(int)(sa->w*sc); SDL_Rect ad{rect.x+m, rect.y+rect.h-h-m, wsc, h}; SDL_RenderCopy(r,ta,nullptr,&ad); SDL_DestroyTexture(ta); SDL_FreeSurface(sa);} 
			snprintf(buf,sizeof(buf),"%d", card.health); SDL_Color hp{160,30,40,255}; SDL_Surface* sh=TTF_RenderUTF8_Blended(statFont_, buf, hp); if (sh){ SDL_Texture* th=SDL_CreateTextureFromSurface(r,sh); float sc=(float)h/(float)sh->h; int wsc=(int)(sh->w*sc); SDL_Rect hd{rect.x+rect.w-wsc-m, rect.y+rect.h-h-m, wsc, h}; SDL_RenderCopy(r,th,nullptr,&hd); SDL_DestroyTexture(th); SDL_FreeSurface(sh);} }
	}

	// 右侧：三选一报价
	for (size_t i=0;i<offers_.size(); ++i) {
		SDL_Rect rect = offers_[i].rect; const Card& card = offers_[i].card;
		if (selectedOfferIndex_==(int)i) { SDL_SetRenderDrawColor(r, 255, 220, 120, 140); SDL_Rect hl{rect.x-3,rect.y-3,rect.w+6,rect.h+6}; SDL_RenderFillRect(r,&hl);} 
		SDL_SetRenderDrawColor(r, 235,230,220,230); SDL_RenderFillRect(r,&rect);
		SDL_SetRenderDrawColor(r, 60,50,40,220); SDL_RenderDrawRect(r,&rect);
		if (nameFont_) { SDL_Color nameCol{50,40,30,255}; SDL_Surface* s=TTF_RenderUTF8_Blended(nameFont_, card.name.c_str(), nameCol); if (s){ SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); int h=SDL_max(12,(int)(rect.h*0.16f)); float sc=(float)h/(float)s->h; int wsc=(int)(s->w*sc); int nx=rect.x+(rect.w-wsc)/2; SDL_Rect nd{nx, rect.y+(int)(rect.h*0.06f), wsc, h}; SDL_RenderCopy(r,t,nullptr,&nd); SDL_DestroyTexture(t); SDL_FreeSurface(s);} }
		if (statFont_) { char buf[32]; int h=SDL_max(12,(int)(rect.h*0.18f)); int m=SDL_max(6,(int)(rect.h*0.035f)); SDL_Color col{80,50,40,255};
			snprintf(buf,sizeof(buf),"%d", card.attack); SDL_Surface* sa=TTF_RenderUTF8_Blended(statFont_, buf, col); if (sa){ SDL_Texture* ta=SDL_CreateTextureFromSurface(r,sa); float sc=(float)h/(float)sa->h; int wsc=(int)(sa->w*sc); SDL_Rect ad{rect.x+m, rect.y+rect.h-h-m, wsc, h}; SDL_RenderCopy(r,ta,nullptr,&ad); SDL_DestroyTexture(ta); SDL_FreeSurface(sa);} 
			snprintf(buf,sizeof(buf),"%d", card.health); SDL_Color hp{160,30,40,255}; SDL_Surface* sh=TTF_RenderUTF8_Blended(statFont_, buf, hp); if (sh){ SDL_Texture* th=SDL_CreateTextureFromSurface(r,sh); float sc=(float)h/(float)sh->h; int wsc=(int)(sh->w*sc); SDL_Rect hd{rect.x+rect.w-wsc-m, rect.y+rect.h-h-m, wsc, h}; SDL_RenderCopy(r,th,nullptr,&hd); SDL_DestroyTexture(th); SDL_FreeSurface(sh);} }
	}

	if (backButton_) backButton_->render(r);
	if (confirmButton_) confirmButton_->render(r);

	if (!message_.empty() && smallFont_) { SDL_Color mcol{250,220,90,255}; SDL_Surface* s=TTF_RenderUTF8_Blended(smallFont_, message_.c_str(), mcol); if (s){ SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{ screenW_/2 - s->w/2, screenH_-150, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} }
}

void BarterState::ensureDemoLibraryIfEmpty() {
	auto& lib = DeckStore::instance().library();
	if (lib.empty()) {
		Card c1; c1.id="demo_sword"; c1.name=u8"残锋"; c1.attack=3; c1.health=2;
		Card c2; c2.id="demo_poem";  c2.name=u8"柳絮"; c2.attack=1; c2.health=4;
		Card c3; c3.id="demo_ink";   c3.name=u8"墨痕"; c3.attack=2; c3.health=3;
		lib.push_back(c1); lib.push_back(c2); lib.push_back(c3);
		message_ = u8"提示：牌库为空，已注入示例卡用于以物易物演示";
	}
}

void BarterState::generateOffers() {
	offers_.clear(); offers_.resize(3);
	// 简单生成：基于随机攻防
	srand(7757);
	for (int i=0;i<3;++i) {
		Card c; c.id = std::string("offer_")+std::to_string(i);
		int a = 1 + rand()%5; int h = 1 + rand()%5;
		c.attack=a; c.health=h; c.name = std::string(u8"易物") + std::to_string(a) + ":" + std::to_string(h);
		offers_[i].card = c;
	}
}

void BarterState::layoutGrids() {
	// 左右两栏
	int margin = 40; int gap = 20;
	int leftW = (screenW_ - margin*2 - gap) * 2 / 3;
	int rightW = (screenW_ - margin*2 - gap) - leftW;
	int topY = 200; int bottom = 110;
	int leftH = SDL_max(160, screenH_ - topY - bottom);
	int rightH = leftH;
	const float aspect = 2.0f/3.0f;

	// 左：库网格自适应
	auto& lib = DeckStore::instance().library();
	int n = (int)lib.size(); libraryRects_.clear(); libraryRects_.resize(n);
	if (n>0) {
		int bestCols=1,bestW=0,bestH=0,bestRows=n; int cg=12;
		for (int cols = SDL_min(n,8); cols>=1; --cols) {
			int rows=(n+cols-1)/cols; int cw=(leftW-(cols-1)*cg)/cols; if (cw<=20) continue; int ch=(int)(cw/aspect); int totalH=rows*ch+(rows-1)*cg; if (totalH<=leftH && cw>bestW){bestW=cw;bestH=ch;bestCols=cols;bestRows=rows;}
		}
		if (bestW==0) { int cols=SDL_min(n, SDL_max(1,leftW/80)); int rows=(n+cols-1)/cols; int cw=(leftW-(cols-1)*cg)/cols; int ch=(int)(cw/aspect); int totalH=rows*ch+(rows-1)*cg; float sc = (float)leftH/(float)totalH; cw=SDL_max(24,(int)(cw*sc)); ch=SDL_max(36,(int)(ch*sc)); bestCols=cols; bestRows=rows; bestW=cw; bestH=ch; }
		int totalW = bestCols*bestW+(bestCols-1)*cg; int sx = margin + (leftW-totalW)/2; int sy = topY + (leftH - (bestRows*bestH+(bestRows-1)*cg))/2; sy=SDL_max(topY,sy);
		for (int i=0;i<n;++i){ int r=i/bestCols,c=i%bestCols; libraryRects_[i] = { sx + c*(bestW+cg), sy + r*(bestH+cg), bestW, bestH }; }
	}

	// 右：三报价纵向排列
	offers_.resize(3);
	int cardW = rightW; int cardH = (int)(cardW / aspect); int cg = 16; int totalH = 3*cardH + 2*cg; if (totalH>rightH){ float sc=(float)rightH/(float)totalH; cardW=(int)(cardW*sc); cardH=(int)(cardH*sc);} 
	int sx = margin + leftW + gap + (rightW-cardW)/2; int sy = topY + (rightH - (3*cardH + 2*cg))/2; sy = SDL_max(topY, sy);
	for (int i=0;i<3;++i) { offers_[i].rect = { sx, sy + i*(cardH+cg), cardW, cardH }; }
}


