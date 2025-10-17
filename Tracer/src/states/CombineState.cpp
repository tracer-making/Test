#include "CombineState.h"
#include "MapExploreState.h"
#include "../core/App.h"
#include "../ui/CardRenderer.h"
#include <algorithm>
#include <unordered_map>
#include <cstdlib>
#include <ctime>

CombineState::CombineState() = default;
CombineState::~CombineState() {
    if (titleTex_) SDL_DestroyTexture(titleTex_);
    if (titleFont_) TTF_CloseFont(titleFont_);
    if (smallFont_) TTF_CloseFont(smallFont_);
    if (nameFont_) TTF_CloseFont(nameFont_);
    if (statFont_) TTF_CloseFont(statFont_);
    delete backButton_;
}

void CombineState::onEnter(App& app) {
    screenW_ = 1600; screenH_ = 1000; SDL_SetWindowSize(app.getWindow(), screenW_, screenH_);
    titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 56);
    smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 16);
    nameFont_  = TTF_OpenFont("assets/fonts/Sanji.ttf", 20);
    statFont_  = TTF_OpenFont("assets/fonts/Sanji.ttf", 18);
    if (titleFont_) { SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, u8"合卷", col); if (s) { titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), s); SDL_FreeSurface(s);} }

    backButton_ = new Button(); if (backButton_) { backButton_->setRect({20,20,120,36}); backButton_->setText(u8"返回地图"); if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer()); backButton_->setOnClick([this]() { pendingGoMapExplore_ = true; }); }
    combineButton_ = new Button(); if (combineButton_) { combineButton_->setRect({ screenW_/2 - 60, slotLeft_.y + slotLeft_.h + 12, 120, 36 }); combineButton_->setText(u8"合卷"); if (smallFont_) combineButton_->setFont(smallFont_, app.getRenderer()); combineButton_->setOnClick([this]() { combineSelectedPair(); }); }

    // 初始化随机数种子
    srand((unsigned int)time(nullptr));

    layoutUI();
    buildAllPairs();
    layoutPairsGrid();
}

void CombineState::onExit(App& app) {}

void CombineState::handleEvent(App& app, const SDL_Event& e) {
    if (backButton_) backButton_->handleEvent(e);
    if (combineButton_ && pairLibIndexA_ >= 0 && pairLibIndexB_ >= 0) combineButton_->handleEvent(e);
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx=e.button.x, my=e.button.y;
        // 点击上方任一牌位：打开选择（只有在没有单张卡牌模式时才能点击）
        if (!selecting_ || !pairs_.empty() || (pairs_.size() == 1 && pairs_[0].first != -1)) {
            if ((mx>=slotLeft_.x && mx<=slotLeft_.x+slotLeft_.w && my>=slotLeft_.y && my<=slotLeft_.y+slotLeft_.h) ||
                (mx>=slotRight_.x && mx<=slotRight_.x+slotRight_.w && my>=slotRight_.y && my<=slotRight_.y+slotRight_.h)) {
                selecting_ = true; 
                buildAllPairs(); 
                layoutPairsGrid();
                
                // 如果没有找到同名牌对，随机选择一张牌
                if (pairs_.empty()) {
                    auto& lib = DeckStore::instance().library();
                    if (!lib.empty()) {
                        int randomIndex = rand() % lib.size();
                        const Card& randomCard = lib[randomIndex];
                        
                        // 从CardDB获取原始数值版本
                        const Card* originalCard = CardDB::instance().find(randomCard.id);
                        if (originalCard) {
                            // 清空pairs_，添加一张原始卡牌
                            pairs_.clear();
                            pairRectsA_.clear();
                            pairRectsB_.clear();
                            
                            // 创建一个特殊的"单张卡牌"条目
                            pairs_.push_back({-1, -1}); // 特殊标记表示这是单张卡牌
                            
                            // 布局单张卡牌
                            int pw=120, ph=180;
                            int x = (screenW_ - pw) / 2;
                            int y = slotLeft_.y + slotLeft_.h + 60;
                            pairRectsA_.push_back({x, y, pw, ph});
                            pairRectsB_.push_back({0, 0, 0, 0}); // 空的右卡位
                            
                            // 存储原始卡牌用于显示
                            selectedPair_ = 0;
                            pairLibIndexA_ = randomIndex; // 存储随机卡牌的索引
                            pairLibIndexB_ = -1; // 没有第二张卡
                            
                            message_ = u8"未找到同名牌对，随机选择了一张卡牌";
                        }
                    }
                }
            }
        }
        // 点击下方任一组的任一张：放入上方并收起
        if (selecting_) {
            for (size_t i=0;i<pairRectsA_.size() && i<pairs_.size(); ++i) {
                const SDL_Rect& ra = pairRectsA_[i]; const SDL_Rect& rb = pairRectsB_[i];
                if ((mx>=ra.x && mx<=ra.x+ra.w && my>=ra.y && my<=ra.y+ra.h) || (mx>=rb.x && mx<=rb.x+rb.w && my>=rb.y && my<=rb.y+rb.h)) {
                    // 检查是否是单张卡牌（特殊标记）
                    if (pairs_[i].first == -1 && pairs_[i].second == -1) {
                        // 单张卡牌情况：启动获取动画并加入全局牌库
                        auto& lib = DeckStore::instance().library();
                        if (pairLibIndexA_ >= 0 && pairLibIndexA_ < (int)lib.size()) {
                            const Card& randomCard = lib[pairLibIndexA_];
                            const Card* originalCard = CardDB::instance().find(randomCard.id);
                            if (originalCard) {
                                // 将原始卡牌加入全局牌库
                                DeckStore::instance().addToLibrary(*originalCard);
                                
                                acquireAnim_.active = true;
                                acquireAnim_.time = 0.0f;
                                acquireAnim_.rect = ra;
                                acquireAnim_.card = *originalCard;
                                message_ = u8"获得卡牌：" + originalCard->name;
                            }
                        }
                        return;
                    } else {
                        // 正常同名牌对情况
                        selectedPair_ = (int)i; 
                        pairLibIndexA_ = pairs_[i].first; 
                        pairLibIndexB_ = pairs_[i].second; 
                        selecting_ = false; 
                        break;
                    }
                }
            }
        }
    }
}

void CombineState::update(App& app, float dt) {
    // 更新融合动画
    if (combineAnim_.active) {
        combineAnim_.time += dt;
        if (combineAnim_.time >= combineAnim_.duration) {
            // 动画结束，执行实际融合操作
            auto& lib = DeckStore::instance().library();
            DeckStore::instance().addToLibrary(combineAnim_.resultCard);
            
            // 删除原卡牌（注意索引顺序）
            if (pairLibIndexA_ > pairLibIndexB_) { 
                lib.erase(lib.begin() + pairLibIndexA_); 
                lib.erase(lib.begin() + pairLibIndexB_); 
            } else { 
                lib.erase(lib.begin() + pairLibIndexB_); 
                lib.erase(lib.begin() + pairLibIndexA_); 
            }
            
            combineAnim_.active = false;
            pairLibIndexA_ = pairLibIndexB_ = -1; 
            selectedPair_ = -1; 
            buildAllPairs(); 
            layoutPairsGrid();
            message_ = u8"合卷完成：已生成融合卡并移除原卡";
            pendingGoMapExplore_ = true;
        }
    }
    
    // 更新获取动画
    if (acquireAnim_.active) {
        acquireAnim_.time += dt;
        if (acquireAnim_.time >= acquireAnim_.duration) {
            acquireAnim_.active = false;
            pendingGoMapExplore_ = true;
        }
    }
    
    if (pendingGoMapExplore_) { pendingGoMapExplore_ = false; app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState()))); }
}

void CombineState::render(App& app) {
    SDL_Renderer* r = app.getRenderer(); SDL_SetRenderDrawColor(r, 18,22,32,255); SDL_RenderClear(r);
    if (titleTex_) { int tw,th; SDL_QueryTexture(titleTex_,nullptr,nullptr,&tw,&th); SDL_Rect d{ (screenW_-tw)/2, 60, tw, th }; SDL_RenderCopy(r,titleTex_,nullptr,&d); }
    if (backButton_) backButton_->render(r);
    if (combineButton_ && pairLibIndexA_ >= 0 && pairLibIndexB_ >= 0) combineButton_->render(r);

    // 渲染融合动画
    if (combineAnim_.active) {
        float progress = combineAnim_.time / combineAnim_.duration;
        float alpha = 1.0f - progress * 0.3f; // 稍微淡出
        
        // 计算动画位置：两张卡向中心移动并融合
        float moveProgress = std::min(progress * 2.0f, 1.0f); // 前一半时间移动
        float scaleProgress = std::max(0.0f, (progress - 0.5f) * 2.0f); // 后一半时间缩放
        
        SDL_Rect animLeft = combineAnim_.leftRect;
        SDL_Rect animRight = combineAnim_.rightRect;
        SDL_Rect animCenter = combineAnim_.centerRect;
        
        // 移动动画
        if (moveProgress < 1.0f) {
            float t = moveProgress;
            animLeft.x = (int)(combineAnim_.leftRect.x + (animCenter.x - combineAnim_.leftRect.x) * t);
            animRight.x = (int)(combineAnim_.rightRect.x + (animCenter.x - combineAnim_.rightRect.x) * t);
        }
        
        // 缩放动画
        if (scaleProgress > 0.0f) {
            float scale = 1.0f + scaleProgress * 0.3f; // 稍微放大
            int newW = (int)(animCenter.w * scale);
            int newH = (int)(animCenter.h * scale);
            animLeft = { animCenter.x + (animCenter.w - newW) / 2, animCenter.y + (animCenter.h - newH) / 2, newW, newH };
            animRight = animLeft; // 融合成一张
        }
        
        // 渲染动画中的卡牌
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 255, 255, 255, (Uint8)(255 * alpha));
        SDL_RenderFillRect(r, &animLeft);
        if (moveProgress < 1.0f) {
            SDL_RenderFillRect(r, &animRight);
        }
        
        // 渲染卡牌内容
        if (moveProgress < 1.0f) {
            // 动画期间显示原始卡牌，不显示融合后的数值
            auto& lib = DeckStore::instance().library();
            if (pairLibIndexA_ >= 0 && pairLibIndexA_ < (int)lib.size()) {
                CardRenderer::renderCard(app, lib[pairLibIndexA_], animLeft, nameFont_, statFont_, false);
            }
            if (pairLibIndexB_ >= 0 && pairLibIndexB_ < (int)lib.size()) {
                CardRenderer::renderCard(app, lib[pairLibIndexB_], animRight, nameFont_, statFont_, false);
            }
        } else {
            // 融合完成后显示结果卡牌
            CardRenderer::renderCard(app, combineAnim_.resultCard, animLeft, nameFont_, statFont_, false);
        }
        
        // 添加融合特效
        if (progress > 0.3f && progress < 0.7f) {
            SDL_SetRenderDrawColor(r, 255, 255, 100, (Uint8)(255 * (1.0f - std::abs(progress - 0.5f) * 2.0f)));
            SDL_RenderFillRect(r, &animCenter);
        }
        
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    } else {
        // 正常渲染上方两个牌位
        SDL_SetRenderDrawColor(r, 235,230,220,230); SDL_RenderFillRect(r, &slotLeft_); SDL_RenderFillRect(r, &slotRight_);
        SDL_SetRenderDrawColor(r, 60,50,40,220); SDL_RenderDrawRect(r, &slotLeft_); SDL_RenderDrawRect(r, &slotRight_);

        // 上方放置已选组（只有在非单张卡牌模式下才显示）
        if ((pairLibIndexA_>=0 || pairLibIndexB_>=0) && !(pairs_.size() == 1 && pairs_[0].first == -1)) {
            auto& lib = DeckStore::instance().library();
            if (pairLibIndexA_>=0 && pairLibIndexA_<(int)lib.size()) CardRenderer::renderCard(app, lib[pairLibIndexA_], slotLeft_, nameFont_, statFont_, false);
            if (pairLibIndexB_>=0 && pairLibIndexB_<(int)lib.size()) CardRenderer::renderCard(app, lib[pairLibIndexB_], slotRight_, nameFont_, statFont_, false);
        }
    }

    // 下方展示所有组：AA  BB  CC ... （组间留白），仅在选择模式显示
    if (selecting_) {
        auto& lib = DeckStore::instance().library();
        for (size_t i=0;i<pairs_.size() && i<pairRectsA_.size(); ++i) {
            const SDL_Rect& ra = pairRectsA_[i]; 
            int ia = pairs_[i].first; int ib = pairs_[i].second;
            
            // 检查是否是单张卡牌（特殊标记）
            if (ia == -1 && ib == -1) {
                // 显示原始数值版本的卡牌（如果不在获取动画中）
                if (!acquireAnim_.active && pairLibIndexA_ >= 0 && pairLibIndexA_ < (int)lib.size()) {
                    const Card& randomCard = lib[pairLibIndexA_];
                    const Card* originalCard = CardDB::instance().find(randomCard.id);
                    if (originalCard) {
                        CardRenderer::renderCard(app, *originalCard, ra, nameFont_, statFont_, true);
                    }
                }
            } else {
                // 正常同名牌对显示
                if (i < pairRectsB_.size()) {
                    const SDL_Rect& rb = pairRectsB_[i];
                    if (ia>=0 && ia<(int)lib.size()) CardRenderer::renderCard(app, lib[ia], ra, nameFont_, statFont_, (int)i==selectedPair_);
                    if (ib>=0 && ib<(int)lib.size()) CardRenderer::renderCard(app, lib[ib], rb, nameFont_, statFont_, (int)i==selectedPair_);
                }
            }
        }
    }

    // 渲染获取动画
    if (acquireAnim_.active) {
        float progress = acquireAnim_.time / acquireAnim_.duration;
        float alpha = 1.0f - progress * 0.3f; // 稍微淡出
        float scale = 1.0f + progress * 0.5f; // 放大效果
        float moveY = -progress * 50.0f; // 向上移动
        
        SDL_Rect animRect = acquireAnim_.rect;
        animRect.x = (int)(animRect.x + (animRect.w * (scale - 1.0f)) / 2);
        animRect.y = (int)(animRect.y + moveY);
        animRect.w = (int)(animRect.w * scale);
        animRect.h = (int)(animRect.h * scale);
        
        // 渲染动画中的卡牌
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 255, 255, 255, (Uint8)(255 * alpha));
        SDL_RenderFillRect(r, &animRect);
        
        CardRenderer::renderCard(app, acquireAnim_.card, animRect, nameFont_, statFont_, false);
        
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }

    if (!message_.empty() && smallFont_) { SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, message_.c_str(), col, screenW_-40); if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{20, screenH_-s->h-20, s->w, s->h}; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} }
}

void CombineState::layoutUI() {
    int w=150,h=210,gap=24; int totalW = 2*w + gap; int x0=(screenW_-totalW)/2; int y = (screenH_-h)/2 - 80;
    slotLeft_ = { x0, y, w, h}; slotRight_ = { x0 + w + gap, y, w, h };
    if (combineButton_) combineButton_->setRect({ screenW_/2 - 60, slotLeft_.y + slotLeft_.h + 12, 120, 36 });
}

void CombineState::buildAllPairs() {
    pairs_.clear(); pairRectsA_.clear(); pairRectsB_.clear(); selectedPair_ = -1; pairLibIndexA_ = pairLibIndexB_ = -1;
    auto& lib = DeckStore::instance().library();
    // 名称->索引列表
    std::unordered_map<std::string, std::vector<int>> mapNameToIdx;
    mapNameToIdx.reserve(lib.size());
    for (int i=0;i<(int)lib.size(); ++i) mapNameToIdx[lib[i].name].push_back(i);
    for (auto& kv : mapNameToIdx) {
        auto& vec = kv.second; if ((int)vec.size() < 2) continue;
        // 每两张组成一组（只取成对）
        for (size_t k=0; k+1<vec.size(); k+=2) {
            pairs_.push_back({ vec[k], vec[k+1] });
        }
    }
}

void CombineState::layoutPairsGrid() {
    pairRectsA_.clear(); pairRectsB_.clear();
    int pw=120, ph=180; int pgapIn=20; int groupGap=60; // A和B之间间距；组与组之间更大间距（加大）
    int totalW = 0; for (size_t i=0;i<pairs_.size(); ++i) totalW += (2*pw + pgapIn) + (i+1<pairs_.size()?groupGap:0);
    int x = (screenW_ - totalW)/2; int y = slotLeft_.y + slotLeft_.h + 60;
    for (size_t i=0;i<pairs_.size(); ++i) {
        SDL_Rect ra{ x, y, pw, ph }; SDL_Rect rb{ x + pw + pgapIn, y, pw, ph };
        pairRectsA_.push_back(ra); pairRectsB_.push_back(rb);
        x += (2*pw + pgapIn) + groupGap;
    }
}

void CombineState::combineSelectedPair() {
    if (pairLibIndexA_ < 0 || pairLibIndexB_ < 0) { message_ = u8"请先放置一组卡牌"; return; }
    auto& lib = DeckStore::instance().library();
    if (pairLibIndexA_ >= (int)lib.size() || pairLibIndexB_ >= (int)lib.size()) { message_ = u8"选择无效"; return; }
    const Card& a = lib[pairLibIndexA_];
    const Card& b = lib[pairLibIndexB_];
    Card c = a;
    c.attack = a.attack + b.attack;
    c.health = a.health + b.health;
    for (const auto& m : b.marks) {
        if (std::find(c.marks.begin(), c.marks.end(), m) == c.marks.end()) c.marks.push_back(m);
    }
    c.instanceId = CardDB::instance().make(a.id).instanceId;
    
    // 启动融合动画
    combineAnim_.active = true;
    combineAnim_.time = 0.0f;
    combineAnim_.leftRect = slotLeft_;
    combineAnim_.rightRect = slotRight_;
    combineAnim_.centerRect = { (slotLeft_.x + slotRight_.x) / 2, slotLeft_.y, slotLeft_.w, slotLeft_.h };
    combineAnim_.resultCard = c;
    
    // 延迟执行实际融合操作
    message_ = u8"正在融合卡牌...";
}


