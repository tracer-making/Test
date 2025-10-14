#include "InkShopState.h"
#include "TestState.h"
#include "MapExploreState.h"
#include "../core/App.h"

InkShopState::InkShopState() = default;
InkShopState::~InkShopState() {
	if (titleTex_) SDL_DestroyTexture(titleTex_);
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (smallFont_) TTF_CloseFont(smallFont_);
	delete backButton_;
}

void InkShopState::onEnter(App& app) {
	int w,h; SDL_GetWindowSize(app.getWindow(), &w, &h); screenW_ = w; screenH_ = h;
	titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 56);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 18);
	if (titleFont_) {
		SDL_Color col{200,230,255,255};
		SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, u8"墨坊", col);
		if (s) { titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), s); SDL_FreeSurface(s);} 
	}

	backButton_ = new Button();
	if (backButton_) {
		backButton_->setRect({20,20,120,36});
		backButton_->setText(u8"返回测试");
		if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer());
		backButton_->setOnClick([this]() { pendingGoMapExplore_ = true; });
	}

	grantEntryGift();
	buildShopItems();
	layoutItems();
}

void InkShopState::onExit(App& app) {}

void InkShopState::handleEvent(App& app, const SDL_Event& e) {
	if (backButton_) backButton_->handleEvent(e);
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mx=e.button.x, my=e.button.y;
		for (auto& it : shopItems_) {
			if (mx>=it.rect.x && mx<=it.rect.x+it.rect.w && my>=it.rect.y && my<=it.rect.y+it.rect.h) {
				if (wenmai_ >= it.price) { wenmai_ -= it.price; ownedInks_.push_back(it.name); message_ = u8"已购买：" + it.name; }
				else { message_ = u8"文脉不足"; }
				break;
			}
		}
	}
}

void InkShopState::update(App& app, float dt) {
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

void InkShopState::render(App& app) {
	SDL_Renderer* r = app.getRenderer();
	SDL_SetRenderDrawColor(r, 18, 22, 32, 255);
	SDL_RenderClear(r);

	if (titleTex_) { int tw,th; SDL_QueryTexture(titleTex_,nullptr,nullptr,&tw,&th); SDL_Rect dst{ (screenW_-tw)/2, 60, tw, th }; SDL_RenderCopy(r, titleTex_, nullptr, &dst);} 
	if (backButton_) backButton_->render(r);

	// 文脉与持有之墨
	if (smallFont_) {
		std::string wm = std::string(u8"文脉：") + std::to_string(wenmai_);
		SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, wm.c_str(), col); if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{ 30, 100, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} 
		std::string own = u8"持有之墨："; for (size_t i=0;i<ownedInks_.size();++i) { if (i) own += u8"、"; own += ownedInks_[i]; }
		SDL_Surface* s2 = TTF_RenderUTF8_Blended_Wrapped(smallFont_, own.c_str(), col, screenW_-60); if (s2) { SDL_Texture* t2=SDL_CreateTextureFromSurface(r,s2); SDL_Rect d2{ 30, 130, s2->w, s2->h }; SDL_RenderCopy(r,t2,nullptr,&d2); SDL_DestroyTexture(t2); SDL_FreeSurface(s2);} 
	}

	// 物品
	for (const auto& it : shopItems_) {
		SDL_SetRenderDrawColor(r, 235,230,220,230); SDL_RenderFillRect(r, &it.rect);
		SDL_SetRenderDrawColor(r, 60,50,40,220); SDL_RenderDrawRect(r, &it.rect);
		if (smallFont_) {
			std::string line = it.name + "  -  " + std::to_string(it.price) + u8" 文脉";
			SDL_Color col{50,40,30,255}; SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, line.c_str(), col, it.rect.w-20); if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{ it.rect.x+10, it.rect.y+10, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} 
		}
	}

	if (!message_.empty() && smallFont_) {
		SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, message_.c_str(), col, screenW_-40); if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{20, screenH_-s->h-20, s->w, s->h}; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} 
	}
}

void InkShopState::grantEntryGift() {
	ownedInks_.push_back(u8"寻常之墨");
	message_ = u8"入店赠予：寻常之墨";
}

void InkShopState::buildShopItems() {
	shopItems_.clear();
	shopItems_.push_back({u8"墨·浓", 10});
	shopItems_.push_back({u8"墨·清", 8});
	shopItems_.push_back({u8"墨·古香", 12});
	shopItems_.push_back({u8"墨·灵光", 15});
}

void InkShopState::layoutItems() {
	int w = 320, h = 80; int gap = 14; int cols = 2; int totalW = cols*w + (cols-1)*gap; int x0 = (screenW_-totalW)/2; int y0 = 220; 
	for (int i=0;i<(int)shopItems_.size(); ++i) { int r=i/cols, c=i%cols; shopItems_[i].rect = { x0 + c*(w+gap), y0 + r*(h+gap), w, h }; }
}


