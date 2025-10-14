#include "EngraveState.h"
#include "TestState.h"
#include "MapExploreState.h"
#include "../core/App.h"
#include <random>

EngraveState::EngraveState() = default;
EngraveState::~EngraveState() {
	if (titleTex_) SDL_DestroyTexture(titleTex_);
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (smallFont_) TTF_CloseFont(smallFont_);
	delete backButton_;
	delete confirmButton_;
}

void EngraveState::onEnter(App& app) {
	int w,h; SDL_GetWindowSize(app.getWindow(), &w, &h); screenW_ = w; screenH_ = h;
	titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 56);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 18);
	if (titleFont_) {
		SDL_Color col{200,230,255,255};
		SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, u8"意境刻画：三选一", col);
		if (s) { titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), s); SDL_FreeSurface(s);} 
	}

	backButton_ = new Button();
	if (backButton_) {
		backButton_->setRect({20,20,120,36});
		backButton_->setText(u8"返回测试");
		if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer());
		backButton_->setOnClick([this]() { pendingGoMapExplore_ = true; });
	}

	confirmButton_ = new Button();
	if (confirmButton_) {
		confirmButton_->setRect({screenW_ - 160, 20, 140, 36});
		confirmButton_->setText(u8"确认选择");
		if (smallFont_) confirmButton_->setFont(smallFont_, app.getRenderer());
		confirmButton_->setOnClick([this]() {
			if (selected_ != -1) {
				result_ = choices_[selected_].title + " - " + choices_[selected_].desc;
			}
		});
	}

	buildRandomChoices();
	layoutChoices();
}

void EngraveState::onExit(App& app) {}

void EngraveState::handleEvent(App& app, const SDL_Event& e) {
	if (backButton_) backButton_->handleEvent(e);
	if (confirmButton_) confirmButton_->handleEvent(e);
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mx=e.button.x, my=e.button.y;
		for (int i=0;i<(int)choices_.size();++i) {
			const auto& c = choices_[i];
			if (mx>=c.rect.x && mx<=c.rect.x+c.rect.w && my>=c.rect.y && my<=c.rect.y+c.rect.h) { selected_ = i; break; }
		}
	}
}

void EngraveState::update(App& app, float dt) {
	if (pendingBackToTest_) {
		pendingBackToTest_ = false;
		app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState())));
		return;
	}
	if (pendingGoMapExplore_) {
		pendingGoMapExplore_ = false;
		app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState())));
		return;
	}
}

void EngraveState::render(App& app) {
	SDL_Renderer* r = app.getRenderer();
	SDL_SetRenderDrawColor(r, 18, 22, 32, 255);
	SDL_RenderClear(r);

	if (titleTex_) { int tw,th; SDL_QueryTexture(titleTex_,nullptr,nullptr,&tw,&th); SDL_Rect dst{ (screenW_-tw)/2, 60, tw, th }; SDL_RenderCopy(r, titleTex_, nullptr, &dst);} 
	if (backButton_) backButton_->render(r);
	if (confirmButton_) confirmButton_->render(r);

	for (int i=0;i<(int)choices_.size(); ++i) {
		auto& c = choices_[i];
		SDL_SetRenderDrawColor(r, 235,230,220,230);
		SDL_RenderFillRect(r, &c.rect);
		if (i==selected_) {
			SDL_SetRenderDrawColor(r, 0, 220, 180, 255);
			for (int t=0;t<3;++t) { SDL_Rect rr{ c.rect.x - t, c.rect.y - t, c.rect.w + 2*t, c.rect.h + 2*t }; SDL_RenderDrawRect(r, &rr); }
		} else {
			SDL_SetRenderDrawColor(r, 60,50,40,220);
			SDL_RenderDrawRect(r, &c.rect);
		}
		if (smallFont_) {
			SDL_Color tcol{50,40,30,255}; SDL_Surface* ts = TTF_RenderUTF8_Blended(smallFont_, c.title.c_str(), tcol); if (ts) { SDL_Texture* tt=SDL_CreateTextureFromSurface(r,ts); SDL_Rect td{ c.rect.x+12, c.rect.y+10, ts->w, ts->h }; SDL_RenderCopy(r,tt,nullptr,&td); SDL_DestroyTexture(tt); SDL_FreeSurface(ts);} 
			SDL_Color dcol{80,90,120,220}; SDL_Surface* ds = TTF_RenderUTF8_Blended_Wrapped(smallFont_, c.desc.c_str(), dcol, c.rect.w-24); if (ds) { SDL_Texture* dt=SDL_CreateTextureFromSurface(r,ds); SDL_Rect dd{ c.rect.x+12, c.rect.y+40, ds->w, ds->h }; SDL_RenderCopy(r,dt,nullptr,&dd); SDL_DestroyTexture(dt); SDL_FreeSurface(ds);} 
		}
	}

	if (!result_.empty() && smallFont_) {
		SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, result_.c_str(), col, screenW_-40); if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{20, screenH_-s->h-20, s->w, s->h}; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} 
	}
}

void EngraveState::buildRandomChoices() {
	choices_.clear();
	static const char* yi[] = {"锐意","清风","博学","坚韧","迅捷","灵思"};
	static const char* jing[] = {"剑意","书卷","墨魂","游侠","方士","器匠"};
	std::random_device rd; std::mt19937 g(rd()); std::uniform_int_distribution<int> b(0,1); std::uniform_int_distribution<int> ix(0,5);
	for (int i=0;i<3;++i) {
		bool isYi = b(g)==1;
		Choice c;
		if (isYi) { c.title = std::string("意·") + yi[ix(g)]; c.desc = "获得印记，赋予特殊效果"; }
		else { c.title = std::string("境·") + jing[ix(g)]; c.desc = "改变卡牌或自身的类型加成"; }
		choices_.push_back(c);
	}
}

void EngraveState::layoutChoices() {
	int w = 280, h = 200; int gap = 28; int totalW = 3*w + 2*gap; int x0 = (screenW_-totalW)/2; int y = 180;
	for (int i=0;i<3;++i) { choices_[i].rect = { x0 + i*(w+gap), y, w, h }; }
}


