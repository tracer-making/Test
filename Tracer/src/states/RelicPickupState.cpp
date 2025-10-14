#include "RelicPickupState.h"
#include "TestState.h"
#include "MapExploreState.h"
#include "../core/App.h"
#include <random>

RelicPickupState::RelicPickupState() = default;
RelicPickupState::~RelicPickupState() {
	if (titleTex_) SDL_DestroyTexture(titleTex_);
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (smallFont_) TTF_CloseFont(smallFont_);
	delete backButton_;
}

void RelicPickupState::onEnter(App& app) {
	int w,h; SDL_GetWindowSize(app.getWindow(), &w, &h); screenW_ = w; screenH_ = h;
	titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 56);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 18);
	if (titleFont_) { SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, u8"墨宝拾遗", col); if (s) { titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), s); SDL_FreeSurface(s);} }

	backButton_ = new Button();
	if (backButton_) { backButton_->setRect({20,20,120,36}); backButton_->setText(u8"返回地图"); if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer()); backButton_->setOnClick([this]() { pendingGoMapExplore_ = true; }); }

	ensureThreeRelics();
}

void RelicPickupState::onExit(App& app) {}

void RelicPickupState::handleEvent(App& app, const SDL_Event& e) {
	if (backButton_) backButton_->handleEvent(e);
}

void RelicPickupState::update(App& app, float dt) {
	if (pendingBackToTest_) { pendingBackToTest_ = false; app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState()))); return; }
	if (pendingGoMapExplore_) { pendingGoMapExplore_ = false; app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState()))); return; }
}

void RelicPickupState::render(App& app) {
	SDL_Renderer* r = app.getRenderer(); SDL_SetRenderDrawColor(r, 18,22,32,255); SDL_RenderClear(r);
	if (titleTex_) { int tw,th; SDL_QueryTexture(titleTex_,nullptr,nullptr,&tw,&th); SDL_Rect d{ (screenW_-tw)/2, 60, tw, th }; SDL_RenderCopy(r,titleTex_,nullptr,&d); }
	if (backButton_) backButton_->render(r);

	// 展示道具
	int y = 160; if (smallFont_) {
		SDL_Color col{200,230,255,255};
		for (const auto& name : ownedRelics_) {
			SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, name.c_str(), col);
			if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{ 40, y, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s); y += s->h + 10; }
		}
	}

	if (!message_.empty() && smallFont_) { SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, message_.c_str(), col, screenW_-40); if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{20, screenH_-s->h-20, s->w, s->h}; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} }
}

void RelicPickupState::ensureThreeRelics() {
	// 示例当前持有为空
	// 随机补至3
	static const char* pool[] = {"缚符","玉坠","折扇","古印","阴阳佩","铜钱符","香篆","刻章"};
	std::random_device rd; std::mt19937 g(rd()); std::uniform_int_distribution<int> ix(0, (int)(sizeof(pool)/sizeof(pool[0]))-1);
	while ((int)ownedRelics_.size() < 3) {
		std::string cand = pool[ix(g)];
		bool exists=false; for (const auto& n : ownedRelics_) if (n==cand) { exists=true; break; }
		if (!exists) ownedRelics_.push_back(cand);
	}
	message_ = u8"拾遗成功：道具已达3件";
}


