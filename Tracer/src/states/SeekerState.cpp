#include "SeekerState.h"
#include "MapExploreState.h"
#include "../core/App.h"
#include "../core/Deck.h"
#include "../core/Cards.h"
#include "../ui/CardRenderer.h"
#include "../core/TutorialTexts.h"
#include <algorithm>
#include <random>

SeekerState::SeekerState() = default;

SeekerState::~SeekerState() {
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (smallFont_) TTF_CloseFont(smallFont_);
	if (nameFont_) TTF_CloseFont(nameFont_);
	if (statFont_) TTF_CloseFont(statFont_);
	if (titleTex_) SDL_DestroyTexture(titleTex_);
	if (backButton_) delete backButton_;
	if (tutorialButton_) delete tutorialButton_;
}

void SeekerState::onEnter(App& app) {
	// 屏幕与字体
	screenW_ = 1600; screenH_ = 1000; SDL_SetWindowSize(app.getWindow(), screenW_, screenH_);
	titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 48);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 16);
	nameFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 20);
	statFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 18);
	if (titleFont_) { SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, u8"寻物人", col); if (s) { titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), s); SDL_FreeSurface(s);} }

	backButton_ = new Button(); if (backButton_) { backButton_->setRect({20,20,120,36}); backButton_->setText(u8"返回地图"); if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer()); backButton_->setOnClick([this]() { pendingGoMapExplore_ = true; }); }
	tutorialButton_ = new Button(); if (tutorialButton_) { tutorialButton_->setRect({screenW_ - 120, 20, 100, 35}); tutorialButton_->setText(u8"?"); if (smallFont_) tutorialButton_->setFont(smallFont_, app.getRenderer()); tutorialButton_->setOnClick([this]() { startTutorial(); }); }

	buildEntries();
	layoutEntries();
}

void SeekerState::onExit(App& app) {}

void SeekerState::handleEvent(App& app, const SDL_Event& e) {
	// 教程系统处理
	if (CardRenderer::isTutorialActive()) {
		if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
			CardRenderer::handleTutorialClick();
		}
		return;
	}
	
	// 处理按钮事件（只在上帝模式下）
	if (backButton_ && App::isGodMode()) backButton_->handleEvent(e);
	if (tutorialButton_ && App::isGodMode()) tutorialButton_->handleEvent(e);
	
	if (e.type == SDL_MOUSEMOTION) {
		int mx = e.motion.x, my = e.motion.y;
		
		// 处理印记悬停
		for (const auto& en : entries_) {
			if (en.revealed && mx >= en.rect.x && mx <= en.rect.x + en.rect.w && 
				my >= en.rect.y && my <= en.rect.y + en.rect.h) {
				CardRenderer::handleMarkHover(en.card, en.rect, mx, my, statFont_);
				return;
			}
		}
		
		// 如果没有悬停在任何印记上，隐藏提示
		App::hideMarkTooltip();
	}
	// 处理印记右键点击
	else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
		int mx = e.button.x, my = e.button.y;
		
		for (const auto& en : entries_) {
			if (en.revealed && mx >= en.rect.x && mx <= en.rect.x + en.rect.w && 
				my >= en.rect.y && my <= en.rect.y + en.rect.h) {
				CardRenderer::handleMarkClick(en.card, en.rect, mx, my, statFont_);
				if (App::isMarkTooltipVisible()) {
					return;
				}
			}
		}
	}
	else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mx=e.button.x, my=e.button.y;
		for (auto& en : entries_) {
			if (mx>=en.rect.x && mx<=en.rect.x+en.rect.w && my>=en.rect.y && my<=en.rect.y+en.rect.h) {
                if (!en.revealed && !animActive_) {
                    en.revealed = true;
                    // 加入全局牌库
                    DeckStore::instance().addToLibrary(en.card);
                    // 启动获取动画
                    animActive_ = true; animTime_ = 0.0f; animRect_ = en.rect; pickedIndex_ = (int)(&en - &entries_[0]);
                }
				break;
			}
		}
	}
}

void SeekerState::update(App& app, float dt) {
    // 更新教程系统
    CardRenderer::updateTutorial(dt);
    
    if (animActive_) {
        animTime_ += dt;
        if (animTime_ >= animDuration_) { animActive_ = false; pendingGoMapExplore_ = true; }
    }
    if (pendingGoMapExplore_) { pendingGoMapExplore_ = false; app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState()))); }
}

void SeekerState::render(App& app) {
	SDL_Renderer* r = app.getRenderer(); SDL_SetRenderDrawColor(r, 18,22,32,255); SDL_RenderClear(r);
	// 标题（已删除）
	// if (titleTex_) { int tw,th; SDL_QueryTexture(titleTex_,nullptr,nullptr,&tw,&th); SDL_Rect d{ (screenW_-tw)/2, 60, tw, th }; SDL_RenderCopy(r,titleTex_,nullptr,&d); }
	// 返回按钮（只在上帝模式下显示）
	if (backButton_ && App::isGodMode()) backButton_->render(r);
	
	// 教程按钮（始终显示）
	if (tutorialButton_) tutorialButton_->render(r);

	for (const auto& en : entries_) {
		if (en.revealed) {
			// 金羊皮淡金色渲染，参考墨坊
			if (en.card.id == "jinang_mao") {
				// 直接复用自定义金色渲染逻辑（内联一份简版以避免状态依赖）
				SDL_Renderer* r2 = app.getRenderer();
				SDL_SetRenderDrawColor(r2, 255, 235, 150, 255);
				SDL_RenderFillRect(r2, &en.rect);
				SDL_SetRenderDrawColor(r2, 255, 245, 180, 255);
				SDL_RenderDrawRect(r2, &en.rect);
				SDL_SetRenderDrawColor(r2, 255, 240, 160, 255);
				SDL_Rect inner{ en.rect.x+2,en.rect.y+2,en.rect.w-4,en.rect.h-4 }; SDL_RenderDrawRect(r2, &inner);
				if (nameFont_) { SDL_Color nameCol{160,100,60,255}; SDL_Surface* s = TTF_RenderUTF8_Blended(nameFont_, en.card.name.c_str(), nameCol); if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r2,s); int dh=SDL_max(12,(int)(en.rect.h*0.16f)); float sc=(float)dh/(float)s->h; int sw=(int)(s->w*sc); SDL_Rect nd{ en.rect.x+(en.rect.w-sw)/2, en.rect.y+(int)(en.rect.h*0.04f), sw, dh }; SDL_RenderCopy(r2,t,nullptr,&nd); SDL_DestroyTexture(t); SDL_FreeSurface(s);} }
				if (statFont_) { SDL_Color stCol{160,100,60,255}; int dh=SDL_max(12,(int)(en.rect.h*0.18f)); int mg=SDL_max(6,(int)(en.rect.h*0.035f)); std::string atk=std::to_string(en.card.attack), hp=std::to_string(en.card.health);
					SDL_Surface* sa=TTF_RenderUTF8_Blended(statFont_, atk.c_str(), stCol); if (sa){ SDL_Texture* ta=SDL_CreateTextureFromSurface(r2,sa); float sc=(float)dh/(float)sa->h; int sw=(int)(sa->w*sc); SDL_Rect ad{ en.rect.x+mg, en.rect.y+en.rect.h-dh-mg, sw, dh }; SDL_RenderCopy(r2,ta,nullptr,&ad); SDL_DestroyTexture(ta); SDL_FreeSurface(sa);} 
					SDL_Surface* sh=TTF_RenderUTF8_Blended(statFont_, hp.c_str(), stCol); if (sh){ SDL_Texture* th=SDL_CreateTextureFromSurface(r2,sh); float sc=(float)dh/(float)sh->h; int sw=(int)(sh->w*sc); SDL_Rect hd{ en.rect.x+en.rect.w-sw-mg, en.rect.y+en.rect.h-dh-mg, sw, dh }; SDL_RenderCopy(r2,th,nullptr,&hd); SDL_DestroyTexture(th); SDL_FreeSurface(sh);} }
			} else {
				CardRenderer::renderCard(app, en.card, en.rect, nameFont_, statFont_, false);
			}
		} else {
			// 牌背：简单底板
			SDL_SetRenderDrawColor(r, 235,230,220,230); SDL_RenderFillRect(r, &en.rect);
			SDL_SetRenderDrawColor(r, 60,50,40,220); SDL_RenderDrawRect(r, &en.rect);
			if (smallFont_) { SDL_Color col{90,80,70,200}; SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, u8"未知", col); if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{ en.rect.x + (en.rect.w - s->w)/2, en.rect.y + en.rect.h/2 - s->h/2, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} }
		}
	}

    // 获取动画：上移+淡出
    if (animActive_) {
        float p = animTime_ / animDuration_;
        float alpha = 1.0f - p;
        SDL_Rect rct = animRect_;
        rct.y -= (int)(120 * p);
        SDL_SetRenderDrawColor(r, 255, 255, 180, (Uint8)(220 * alpha));
        SDL_RenderFillRect(r, &rct);
        SDL_SetRenderDrawColor(r, 240, 240, 240, (Uint8)(255 * alpha));
        SDL_RenderDrawRect(r, &rct);
    }

	if (!message_.empty() && smallFont_) { SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, message_.c_str(), col, screenW_-40); if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{20, screenH_-s->h-20, s->w, s->h}; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} }
	
	// 渲染全局印记提示
	CardRenderer::renderGlobalMarkTooltip(app, statFont_);
	
	// 渲染教程系统
	CardRenderer::renderTutorial(r, smallFont_, screenW_, screenH_);
}

void SeekerState::buildEntries() {
	entries_.clear(); entries_.resize(3);
	// 随机一个位置放金羊毛
	std::random_device rd; std::mt19937 g(rd()); std::uniform_int_distribution<int> pick(0,2); int goldIx = pick(g);
	for (int i=0;i<3;++i) {
		if (i == goldIx) {
			entries_[i].card = CardDB::instance().make("jinang_mao");
		} else {
			// 全牌库随机一张可获取的卡牌
			auto all = CardDB::instance().allIds();
			std::vector<std::string> obtainableIds;
			
			// 过滤出介部的普通卡牌（obtainable == 1 && category == 介部），排除卷册螟蛉
			for (const auto& id : all) {
				Card c = CardDB::instance().make(id);
				if (c.obtainable == 1 && c.category == u8"介部" && c.id != "juance_mingling") {
					obtainableIds.push_back(id);
				}
			}
			
			if (!obtainableIds.empty()) {
				std::shuffle(obtainableIds.begin(), obtainableIds.end(), g);
				entries_[i].card = CardDB::instance().make(obtainableIds.front());
			}
			// 附加1个印记（复用战斗界面的随机印记候选池）
			std::vector<std::string> availableMarks = {
				u8"空袭", u8"水袭", u8"高跳", u8"护主", u8"领袖力量", u8"掘墓人",
				u8"双重攻击", u8"双向攻击", u8"三向攻击", u8"冲刺能手", u8"蛮力冲撞",
				u8"生生不息",  u8"不死印记", u8"优质祭品", u8"丰产之巢", u8"一回合成长",
				u8"内心之蜂", u8"滋生寄生虫", u8"断尾求生", u8"反伤", u8"死神之触",
				u8"臭臭", u8"蚁后", u8"一口之量", u8"坚硬之躯", u8"守护者",
				u8"兔窝", u8"筑坝师", u8"检索", u8"道具商", u8"食尸鬼", u8"骨王"
			};
			std::uniform_int_distribution<int> mi(0, (int)availableMarks.size()-1);
			int guard = 0;
			while (guard < 20) {
				++guard;
				std::string mk = availableMarks[mi(g)];
				bool dup=false; for (const auto& m : entries_[i].card.marks) if (m==mk) { dup=true; break; }
				if (!dup) { entries_[i].card.marks.push_back(mk); break; }
			}
		}
		
		// 设置所有卡牌为不可传承
		entries_[i].card.canInherit = false;
	}
}

void SeekerState::layoutEntries() {
	int w = 150, h = 210, gap = 30; int totalW = 3*w + 2*gap; int x0 = (screenW_-totalW)/2; int y = (screenH_-h)/2;
	for (int i=0;i<3;++i) entries_[i].rect = { x0 + i*(w+gap), y, w, h };
}

void SeekerState::startTutorial() {
    // 使用统一的教程文本
    std::vector<std::string> tutorialTexts = TutorialTexts::getSeekerTutorial();
    
    // 创建空的高亮区域（不使用高亮功能）
    std::vector<SDL_Rect> highlightRects = {
        {0, 0, 0, 0}, // 无高亮
        {0, 0, 0, 0}, // 无高亮
        {0, 0, 0, 0}, // 无高亮
        {0, 0, 0, 0}, // 无高亮
        {0, 0, 0, 0}  // 无高亮
    };
    
    // 启动教程
    CardRenderer::startTutorial(tutorialTexts, highlightRects);
}
