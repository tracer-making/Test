#include "TemperState.h"
#include "TestState.h"
#include "MapExploreState.h"
#include "../core/App.h"
#include "../ui/CardRenderer.h"
#include <random>

TemperState::TemperState() = default;
TemperState::~TemperState() {
	if (titleTex_) SDL_DestroyTexture(titleTex_);
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (smallFont_) TTF_CloseFont(smallFont_);
	delete backButton_;
	delete confirmButton_;
}

void TemperState::onEnter(App& app) {
	int w,h; SDL_GetWindowSize(app.getWindow(), &w, &h); screenW_ = w; screenH_ = h;
	titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 56);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 18);
	if (titleFont_) { SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, u8"淬炼", col); if (s) { titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), s); SDL_FreeSurface(s);} }

	backButton_ = new Button(); if (backButton_) { backButton_->setRect({20,20,120,36}); backButton_->setText(u8"返回地图"); if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer()); backButton_->setOnClick([this]() { pendingGoMapExplore_ = true; }); }
	confirmButton_ = new Button(); if (confirmButton_) { confirmButton_->setRect({screenW_/2 - 20, screenH_/2 + 160, 40, 40}); confirmButton_->setText(u8"+"); if (smallFont_) confirmButton_->setFont(smallFont_, app.getRenderer()); confirmButton_->setOnClick([this]() { applyTemper(); }); }

	// 进入时随机方案：+1攻 或 +2血（概率各50%）
	{
		std::random_device rd; std::mt19937 g(rd()); std::uniform_int_distribution<int> d(0,1);
		addAttackMode_ = (d(g) == 0);
	}

	layoutUIRects();
}

void TemperState::onExit(App& app) {}

void TemperState::handleEvent(App& app, const SDL_Event& e) {
    if (backButton_) backButton_->handleEvent(e);
    if (confirmButton_) confirmButton_->handleEvent(e);
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx=e.button.x, my=e.button.y;
        // 点击中央牌位：若已完成首次成功，则直接返回；否则（未锁定）打开选择
        if (mx>=slotRect_.x && mx<=slotRect_.x+slotRect_.w && my>=slotRect_.y && my<=slotRect_.y+slotRect_.h) {
            if (firstSuccessDone_) {
                // 第一次淬炼成功后：点击牌位播放返回动画，并在动画结束后返回地图
                if (!animActive_) { animActive_ = true; animType_ = AnimType::Return; animTime_ = 0.0f; }
                return;
            }
            if (!locked_) {
                openSelection();
            }
        }
		// 选择界面中：点选某张卡
        if (selecting_ && !locked_) {
            for (size_t i=0;i<libRects_.size(); ++i) {
                const SDL_Rect& rc = libRects_[i];
                if (mx>=rc.x && mx<=rc.x+rc.w && my>=rc.y && my<=rc.y+rc.h) {
                    selectedLibIndex_ = (int)i;
                    selecting_ = false; // 选择后立即收起下方界面
                    break;
                }
            }
        }
	}
}

void TemperState::update(App& app, float dt) {
	if (pendingBackToTest_) { pendingBackToTest_ = false; app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState()))); return; }
    if (animActive_) {
        animTime_ += dt;
        if (animTime_ >= animDuration_) {
            animActive_ = false; animType_ = AnimType::None; animTime_ = 0.0f;
            pendingGoMapExplore_ = true;
        }
    }
    if (pendingGoMapExplore_) { pendingGoMapExplore_ = false; app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState()))); return; }
}

void TemperState::render(App& app) {
	SDL_Renderer* r = app.getRenderer(); SDL_SetRenderDrawColor(r, 18,22,32,255); SDL_RenderClear(r);
    if (titleTex_) { int tw,th; SDL_QueryTexture(titleTex_,nullptr,nullptr,&tw,&th); SDL_Rect d{ (screenW_-tw)/2, 60, tw, th }; SDL_RenderCopy(r,titleTex_,nullptr,&d); }
    if (backButton_) backButton_->render(r); if (confirmButton_) confirmButton_->render(r);

    // 中央牌位：若已选择，则显示该牌；否则显示提示底板
    if (selectedLibIndex_ >= 0 && selectedLibIndex_ < (int)libIndices_.size()) {
        auto& lib = DeckStore::instance().library();
        int idx = libIndices_[selectedLibIndex_];
        if (idx >= 0 && idx < (int)lib.size()) {
            CardRenderer::renderCard(app, lib[idx], slotRect_, smallFont_, smallFont_, false);
        }
    } else {
        SDL_SetRenderDrawColor(r, 235,230,220,230); SDL_RenderFillRect(r, &slotRect_);
        SDL_SetRenderDrawColor(r, 60,50,40,220); SDL_RenderDrawRect(r, &slotRect_);
        if (smallFont_) {
            SDL_Color col{80,70,60,255};
            const char* text = addAttackMode_ ? u8"+1 攻击" : u8"+2 生命";
            SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, text, col);
            if (s) { SDL_Texture* t = SDL_CreateTextureFromSurface(r, s); SDL_Rect d{ slotRect_.x + (slotRect_.w - s->w)/2, slotRect_.y + (slotRect_.h - s->h)/2, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} 
        }
    }

    // 选择界面：显示全局牌库网格（仅在选择模式下显示）
    if (selecting_ && !locked_) {
        auto& lib = DeckStore::instance().library();
        for (size_t i=0;i<libIndices_.size() && i<libRects_.size(); ++i) {
            int idx = libIndices_[i]; if (idx < 0 || idx >= (int)lib.size()) continue;
            const Card& c = lib[idx]; const SDL_Rect& rc = libRects_[i];
            // 使用卡面渲染，整体视觉与文脉传承一致（小尺寸）
            CardRenderer::renderCard(app, c, rc, smallFont_, smallFont_, false);
            // 选中高亮边框
            SDL_SetRenderDrawColor(r, (int)i==selectedLibIndex_?0:60, (int)i==selectedLibIndex_?220:50, (int)i==selectedLibIndex_?180:40, 220);
            SDL_RenderDrawRect(r, &rc);
        }
    }

    // 第二次淬炼完成后的过场动画
    if (animActive_) {
        float progress = animTime_ / animDuration_;
        float alpha = 1.0f - progress;
        SDL_Rect rect = slotRect_;
        if (animType_ == AnimType::Destroy) {
            // 摧毁：缩小+淡出
            float scale = 1.0f - 0.5f * progress;
            rect.x += (int)(rect.w * (1.0f - scale) * 0.5f);
            rect.y += (int)(rect.h * (1.0f - scale) * 0.5f);
            rect.w = (int)(rect.w * scale);
            rect.h = (int)(rect.h * scale);
            SDL_SetRenderDrawColor(r, 200, 40, 40, (Uint8)(220 * alpha));
            SDL_RenderFillRect(r, &rect);
        } else if (animType_ == AnimType::Return) {
            // 返回：上移+淡出
            int moveY = (int)(-120 * progress);
            rect.y += moveY;
            SDL_SetRenderDrawColor(r, 80, 200, 120, (Uint8)(220 * alpha));
            SDL_RenderFillRect(r, &rect);
        }
        SDL_SetRenderDrawColor(r, 240, 240, 240, (Uint8)(255 * alpha));
        SDL_RenderDrawRect(r, &rect);
    }

    if (!message_.empty() && smallFont_) { SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, message_.c_str(), col, screenW_-40); if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{20, screenH_-s->h-20, s->w, s->h}; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} }
}

void TemperState::layoutUIRects() {
    // 中央空白牌位（进一步缩小）
    int w = 130, h = 195; slotRect_ = { (screenW_-w)/2, (screenH_-h)/2 - 40, w, h };
    // 选择网格（屏幕底部区域）
    buildSelectionGrid();
    // “+”按钮居中到牌位下方
    if (confirmButton_) {
        confirmButton_->setRect({ slotRect_.x + slotRect_.w/2 - 20, slotRect_.y + slotRect_.h + 12, 40, 40 });
    }
}

void TemperState::applyTemper() {
    auto& lib = DeckStore::instance().library();
    if (selectedLibIndex_ < 0 || selectedLibIndex_ >= (int)libIndices_.size()) { message_ = u8"未放置目标卡，请点击上方牌位选择"; return; }
    int libIndex = libIndices_[selectedLibIndex_]; if (libIndex < 0 || libIndex >= (int)lib.size()) { message_ = u8"选择无效"; return; }

    Card& card = lib[libIndex];
    int& count = temperCountByInstance_[card.instanceId];
    bool success = true;
    if (count == 0) {
        success = true; // 第一次必定成功
    } else if (count == 1) {
        // 第二次50%失败
        std::random_device rd; std::mt19937 gen(rd()); std::uniform_real_distribution<float> d(0.0f, 1.0f);
        success = (d(gen) >= 0.5f);
    } else {
        // 第三次及以后：同样50%失败（可按需扩展）
        std::random_device rd; std::mt19937 gen(rd()); std::uniform_real_distribution<float> d(0.0f, 1.0f);
        success = (d(gen) >= 0.5f);
    }

    if (success) {
        if (addAttackMode_) card.attack += 1; else card.health += 2;
        message_ = addAttackMode_ ? u8"淬炼成功：攻击+1" : u8"淬炼成功：生命+2";
        if (!firstSuccessDone_) firstSuccessDone_ = true; // 标记已完成第一次成功
    } else {
        // 失败则从牌库中删除这张牌
        lib.erase(lib.begin() + libIndex);
        selectedLibIndex_ = -1;
        buildSelectionGrid();
        message_ = u8"淬炼失败：卡牌被摧毁";
    }
    count += 1;
    // 按下“+”后锁定并隐藏下方列表，不允许再更换
    locked_ = true;
    // 若是第二次（或之后）完成，触发动画并返回地图
    if (count >= 2) {
        animActive_ = true;
        animType_ = success ? AnimType::Return : AnimType::Destroy;
        animTime_ = 0.0f;
    }
}

void TemperState::openSelection() {
    selecting_ = true; selectedLibIndex_ = -1; buildSelectionGrid();
}

void TemperState::buildSelectionGrid() {
    auto& lib = DeckStore::instance().library();
    libIndices_.clear(); libRects_.clear();
    for (int i=0;i<(int)lib.size(); ++i) libIndices_.push_back(i);
    // 固定较小缩略图尺寸（与文脉传承更接近）
    int marginX=30; int availW = SDL_max(200, screenW_-marginX*2);
    int gap=6; int thumbW=96; int thumbH=144; // 更小 2:3
    int cols = SDL_max(1, availW / (thumbW + gap));
    cols = SDL_min(cols, 5); // 列数限制，避免过宽
    int n=(int)libIndices_.size();
    int rows = SDL_max(1, (n + cols - 1) / cols);
    int totalW = cols*thumbW + (cols-1)*gap; int startX = (screenW_-totalW)/2;
    int totalH = rows*thumbH + (rows-1)*gap;
    // 将下方区域整体在牌位下方的剩余空间内垂直居中，且水平居中
    int availableTop = slotRect_.y + slotRect_.h + 30;
    int remainingH = screenH_ - availableTop - 40; // 底部留 40
    int y = availableTop + SDL_max(0, (remainingH - totalH)/2);
    for (int i=0;i<n; ++i) { int r=i/cols, c=i%cols; SDL_Rect rc{ startX + c*(thumbW+gap), y + r*(thumbH+gap), thumbW, thumbH }; libRects_.push_back(rc); }
}


