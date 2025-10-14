#include "MemoryRepairState.h"
#include "TestState.h"
#include "MapExploreState.h"
#include "../core/App.h"
#include <random>

MemoryRepairState::MemoryRepairState() = default;
MemoryRepairState::~MemoryRepairState() {
	if (titleTex_) SDL_DestroyTexture(titleTex_);
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (smallFont_) TTF_CloseFont(smallFont_);
	delete backButton_;
	delete confirmButton_;
}

void MemoryRepairState::onEnter(App& app) {
	int w,h; SDL_GetWindowSize(app.getWindow(), &w, &h); screenW_ = w; screenH_ = h;
	titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 56);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 18);
	if (titleFont_) {
		SDL_Color col{200,230,255,255};
		SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, u8"记忆修复（三选一）", col);
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
		confirmButton_->setText(u8"确认获取");
		if (smallFont_) confirmButton_->setFont(smallFont_, app.getRenderer());
		confirmButton_->setOnClick([this]() {
			if (selected_ != -1) {
				DeckStore::instance().library().push_back(candidates_[selected_].card);
				message_ = u8"已获取：" + candidates_[selected_].card.name;
			}
		});
	}

	buildCandidates();
	layoutCandidates();
}

void MemoryRepairState::onExit(App& app) {}

void MemoryRepairState::handleEvent(App& app, const SDL_Event& e) {
	if (backButton_) backButton_->handleEvent(e);
	if (confirmButton_) confirmButton_->handleEvent(e);
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mx=e.button.x, my=e.button.y;
		for (int i=0;i<(int)candidates_.size();++i) {
			const auto& c = candidates_[i];
			if (mx>=c.rect.x && mx<=c.rect.x+c.rect.w && my>=c.rect.y && my<=c.rect.y+c.rect.h) { selected_ = i; break; }
		}
	}
}

void MemoryRepairState::update(App& app, float dt) {
	if (pendingBackToTest_) { pendingBackToTest_ = false; app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState()))); return; }
	if (pendingGoMapExplore_) { pendingGoMapExplore_ = false; app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState()))); return; }
}

void MemoryRepairState::render(App& app) {
	SDL_Renderer* r = app.getRenderer();
	SDL_SetRenderDrawColor(r, 18, 22, 32, 255);
	SDL_RenderClear(r);

	if (titleTex_) { int tw,th; SDL_QueryTexture(titleTex_,nullptr,nullptr,&tw,&th); SDL_Rect dst{ (screenW_-tw)/2, 60, tw, th }; SDL_RenderCopy(r, titleTex_, nullptr, &dst);} 
	if (backButton_) backButton_->render(r);
	if (confirmButton_) confirmButton_->render(r);

	for (int i=0;i<(int)candidates_.size(); ++i) {
		auto& c = candidates_[i];
		SDL_SetRenderDrawColor(r, 235,230,220,230); SDL_RenderFillRect(r, &c.rect);
		if (i==selected_) { SDL_SetRenderDrawColor(r, 255,215,0,255); for (int t=0;t<3;++t) { SDL_Rect rr{ c.rect.x - t, c.rect.y - t, c.rect.w + 2*t, c.rect.h + 2*t }; SDL_RenderDrawRect(r, &rr); } }
		else { SDL_SetRenderDrawColor(r, 60,50,40,220); SDL_RenderDrawRect(r, &c.rect); }
		if (smallFont_) {
			std::string line = c.card.name + "  " + std::to_string(c.card.attack) + "/" + std::to_string(c.card.health);
			SDL_Color col{50,40,30,255}; SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, line.c_str(), col, c.rect.w-20); if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{ c.rect.x+10, c.rect.y+10, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} 
		}
	}

	if (!message_.empty() && smallFont_) { SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, message_.c_str(), col, screenW_-40); if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{20, screenH_-s->h-20, s->w, s->h}; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} }
}

void MemoryRepairState::buildCandidates() {
	candidates_.clear();
	static const char* names[] = {"残简·青锋","残简·素心","残简·游龙","残简·止水","残简·秋砚","残简·墨影"};
	std::random_device rd; std::mt19937 g(rd()); std::uniform_int_distribution<int> nx(0,5); std::uniform_int_distribution<int> atk(1,5); std::uniform_int_distribution<int> hp(2,7);
	for (int i=0;i<3;++i) {
		Card c; c.id = "MR"+std::to_string(i); c.name = names[nx(g)]; c.attack = atk(g); c.health = hp(g);
		candidates_.push_back({c});
	}
}

void MemoryRepairState::layoutCandidates() {
	int w = 280, h = 120; int gap = 24; int totalW = 3*w + 2*gap; int x0 = (screenW_-totalW)/2; int y = 200;
	for (int i=0;i<3;++i) { candidates_[i].rect = { x0 + i*(w+gap), y, w, h }; }
}


