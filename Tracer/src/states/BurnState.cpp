#include "BurnState.h"
#include "MapExploreState.h"
#include "../core/App.h"
#include "../core/ItemStore.h"
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
        backButton_->setText(u8"返回地图");
		if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer());
        backButton_->setOnClick([this]() { pendingGoMapExplore_ = true; });
	}

    burnButton_ = new Button();
	if (burnButton_) {
        burnButton_->setRect({screenW_/2-20, screenH_/2 + 140, 40, 40});
        burnButton_->setText(u8"-");
		if (smallFont_) burnButton_->setFont(smallFont_, app.getRenderer());
		burnButton_->setOnClick([this]() {
            auto& lib = DeckStore::instance().library();
            if (selectedLibIndex_ >= 0 && selectedLibIndex_ < (int)libIndices_.size()) {
                int li = libIndices_[selectedLibIndex_];
                if (li >= 0 && li < (int)lib.size()) {
                    // 动画并删除
                    animActive_ = true; animTime_ = 0.0f; animRect_ = slotRect_;
                    // 记录全局增益：基础+1；若焚毁"玄牡(xuanmu)"则+8；毛皮卡牌不增加魂骨
                    const Card& destroyed = lib[li];
                    if (destroyed.id == std::string("xuanmu")) {
                        ItemStore::instance().extraInitialBones += 8;
                    } else if (destroyed.id == "tuopi_mao" || destroyed.id == "langpi" || destroyed.id == "jinang_mao") {
                        // 毛皮卡牌：可以焚书但不增加魂骨
                        // 不增加魂骨数量
                    } else {
                        ItemStore::instance().extraInitialBones += 1;
                    }
                    lib.erase(lib.begin() + li);
                    // 清空牌位选择，刷新索引，避免渲染残留
                    selectedLibIndex_ = -1;
                    selecting_ = false;
                    buildSelectionGrid();
                }
            } else {
                message_ = u8"请先点击中央牌位选择一张牌";
            }
		});
	}

    layoutUI();
}

void BurnState::onExit(App& app) {}

void BurnState::handleEvent(App& app, const SDL_Event& e) {
	// 处理按钮事件（只在上帝模式下）
	if (backButton_ && App::isGodMode()) backButton_->handleEvent(e);
    if (burnButton_) burnButton_->handleEvent(e);
    
    if (e.type == SDL_MOUSEMOTION) {
        int mx = e.motion.x, my = e.motion.y;
        
        // 处理印记悬停
        // 检查中央牌位的卡牌
        if (!animActive_ && selectedLibIndex_ >= 0 && selectedLibIndex_ < (int)libIndices_.size()) {
            auto& lib = DeckStore::instance().library();
            int li = libIndices_[selectedLibIndex_];
            if (li >= 0 && li < (int)lib.size()) {
                if (mx >= slotRect_.x && mx <= slotRect_.x + slotRect_.w && 
                    my >= slotRect_.y && my <= slotRect_.y + slotRect_.h) {
                    CardRenderer::handleMarkHover(lib[li], slotRect_, mx, my, statFont_);
                    return;
                }
            }
        }
        
        // 检查选择网格中的卡牌
        if (selecting_) {
            auto& lib = DeckStore::instance().library();
            for (size_t i = 0; i < libRects_.size() && i < libIndices_.size(); ++i) {
                int li = libIndices_[i];
                if (li >= 0 && li < (int)lib.size()) {
                    const Card& c = lib[li];
                    const SDL_Rect& rc = libRects_[i];
                    if (mx >= rc.x && mx <= rc.x + rc.w && my >= rc.y && my <= rc.y + rc.h) {
                        CardRenderer::handleMarkHover(c, rc, mx, my, statFont_);
                        return;
                    }
                }
            }
        }
        
        // 如果没有悬停在任何印记上，隐藏提示
        App::hideMarkTooltip();
    }
    else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx=e.button.x, my=e.button.y;
        // 点击中央牌位：打开选择
        if (mx>=slotRect_.x && mx<=slotRect_.x+slotRect_.w && my>=slotRect_.y && my<=slotRect_.y+slotRect_.h) {
            selecting_ = true; buildSelectionGrid();
        }
        // 在选择网格中选择
        if (selecting_) {
            for (size_t i=0;i<libRects_.size(); ++i) { const SDL_Rect& rc = libRects_[i]; if (mx>=rc.x && mx<=rc.x+rc.w && my>=rc.y && my<=rc.y+rc.h) { selectedLibIndex_ = (int)i; selecting_ = false; break; } }
        }
    }
    // 处理印记右键点击
    else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
        int mx = e.button.x, my = e.button.y;
        
        // 检查中央牌位的卡牌
        if (!animActive_ && selectedLibIndex_ >= 0 && selectedLibIndex_ < (int)libIndices_.size()) {
            auto& lib = DeckStore::instance().library();
            int li = libIndices_[selectedLibIndex_];
            if (li >= 0 && li < (int)lib.size()) {
                if (mx >= slotRect_.x && mx <= slotRect_.x + slotRect_.w && 
                    my >= slotRect_.y && my <= slotRect_.y + slotRect_.h) {
                    CardRenderer::handleMarkClick(lib[li], slotRect_, mx, my, statFont_);
                    if (App::isMarkTooltipVisible()) {
                        return;
                    }
                }
            }
        }
        
        // 检查选择网格中的卡牌
        if (selecting_) {
            auto& lib = DeckStore::instance().library();
            for (size_t i = 0; i < libRects_.size() && i < libIndices_.size(); ++i) {
                int li = libIndices_[i];
                if (li >= 0 && li < (int)lib.size()) {
                    const Card& c = lib[li];
                    const SDL_Rect& rc = libRects_[i];
                    if (mx >= rc.x && mx <= rc.x + rc.w && my >= rc.y && my <= rc.y + rc.h) {
                        CardRenderer::handleMarkClick(c, rc, mx, my, statFont_);
                        if (App::isMarkTooltipVisible()) {
                            return;
                        }
                    }
                }
            }
        }
    }
}

void BurnState::update(App& app, float dt) {
    if (animActive_) { animTime_ += dt; if (animTime_ >= animDuration_) { animActive_ = false; pendingGoMapExplore_ = true; } }
    if (pendingGoMapExplore_) { pendingGoMapExplore_ = false; app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState()))); }
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

    // 中央牌位
    SDL_SetRenderDrawColor(r, 235,230,220,230); SDL_RenderFillRect(r, &slotRect_);
    SDL_SetRenderDrawColor(r, 60,50,40,220); SDL_RenderDrawRect(r, &slotRect_);
    if (smallFont_) { SDL_Color col{90,80,70,200}; SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, u8"放入待焚书的卡", col); if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{ slotRect_.x + (slotRect_.w - s->w)/2, slotRect_.y + slotRect_.h/2 - s->h/2, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} }

    // 显示已选择的卡（动画期间不再显示，避免残留）
    if (!animActive_ && selectedLibIndex_ >= 0 && selectedLibIndex_ < (int)libIndices_.size()) {
        auto& lib = DeckStore::instance().library(); int li = libIndices_[selectedLibIndex_]; if (li>=0 && li<(int)lib.size()) { CardRenderer::renderCard(app, lib[li], slotRect_, nameFont_, statFont_, false); }
    }

    // 选择网格
    if (selecting_) {
        auto& lib = DeckStore::instance().library();
        for (size_t i=0;i<libRects_.size() && i<libIndices_.size(); ++i) {
            int li = libIndices_[i]; if (li<0 || li>=(int)lib.size()) continue; const Card& c = lib[li]; const SDL_Rect& rc = libRects_[i];
            CardRenderer::renderCard(app, c, rc, nameFont_, statFont_, (int)i==selectedLibIndex_);
        }
    }

    // 返回按钮（只在上帝模式下显示）
    if (backButton_ && App::isGodMode()) backButton_->render(r);
    if (burnButton_) burnButton_->render(r);

	if (!message_.empty() && smallFont_) {
		SDL_Color mcol{250, 220, 90, 255};
		SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, message_.c_str(), mcol);
		if (s) { SDL_Texture* t = SDL_CreateTextureFromSurface(r, s); SDL_Rect d{ screenW_/2 - s->w/2, screenH_-150, s->w, s->h }; SDL_RenderCopy(r, t, nullptr, &d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} 
	}

    // 焚毁动画（缩小+上移+淡出，红橙色调）
    if (animActive_) {
        float p = animTime_ / animDuration_;
        if (p > 1.0f) p = 1.0f;
        float alpha = 1.0f - p;
        float scale = 1.0f - 0.5f * p;
        SDL_Rect rc = animRect_;
        rc.x += (int)(rc.w * (1.0f - scale) * 0.5f);
        rc.y += (int)(rc.h * (1.0f - scale) * 0.5f) - (int)(80 * p);
        rc.w = (int)(rc.w * scale);
        rc.h = (int)(rc.h * scale);

        // 火焰色块
        SDL_SetRenderDrawColor(r, 255, 120, 40, (Uint8)(220 * alpha));
        SDL_RenderFillRect(r, &rc);
        SDL_SetRenderDrawColor(r, 255, 200, 80, (Uint8)(220 * alpha));
        SDL_RenderDrawRect(r, &rc);
        // 简单火花
        SDL_SetRenderDrawColor(r, 255, 180, 60, (Uint8)(180 * alpha));
        for (int i=0;i<12;++i) {
            int fx = rc.x + (rand()%rc.w);
            int fy = rc.y + (rand()%rc.h);
            SDL_RenderDrawPoint(r, fx, fy);
        }
    }
    
    // 渲染全局印记提示
    CardRenderer::renderGlobalMarkTooltip(app, statFont_);
}

void BurnState::layoutUI() {
    // 中央牌位
    int w=150,h=210; slotRect_ = { (screenW_-w)/2, (screenH_-h)/2 - 30, w, h };
    buildSelectionGrid();
}

void BurnState::buildSelectionGrid() {
    auto& lib = DeckStore::instance().library();
    libIndices_.clear(); libRects_.clear(); for (int i=0;i<(int)lib.size(); ++i) libIndices_.push_back(i);
    // 放在底部的缩略图网格
    int marginX=40, bottom=40; int availW = SDL_max(200, screenW_-marginX*2);
    int gap=8; int thumbW=100, thumbH=150; int cols = SDL_max(1, availW/(thumbW+gap)); cols = SDL_min(cols, 6);
    int n=(int)libIndices_.size(); int rows = SDL_max(1, (n+cols-1)/cols);
    int totalW = cols*thumbW + (cols-1)*gap; int startX=(screenW_-totalW)/2; int totalH = rows*thumbH+(rows-1)*gap; int y = screenH_-bottom-totalH;
    for (int i=0;i<n; ++i) { int r=i/cols, c=i%cols; SDL_Rect rc{ startX + c*(thumbW+gap), y + r*(thumbH+gap), thumbW, thumbH }; libRects_.push_back(rc); }
}
