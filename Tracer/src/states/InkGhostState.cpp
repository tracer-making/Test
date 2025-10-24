#include "InkGhostState.h"
#include "../core/App.h"
#include "../core/Deck.h"
#include "../core/Cards.h"
#include "MapExploreState.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <random>
#include <algorithm>

InkGhostState::InkGhostState() {
    backButton_ = new Button();
}

InkGhostState::~InkGhostState() {
    if (backButton_) {
        delete backButton_;
    }
}

void InkGhostState::onEnter(App& app) {
    SDL_GetWindowSize(app.getWindow(), &screenW_, &screenH_);
    
    // 加载字体
    titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 32);
    smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 16);
    
    if (!titleFont_ || !smallFont_) {
        SDL_Log("TTF_OpenFont failed: %s", TTF_GetError());
    }
    
    // 设置返回按钮
    if (backButton_) {
        SDL_Rect backButtonRect{20, 20, 80, 40};
        backButton_->setRect(backButtonRect);
        backButton_->setText(u8"返回地图");
        if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer());
        backButton_->setOnClick([&app]() {
            app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState())));
        });
    }
    
    // 从全局牌堆获取所有卡牌（手牌+牌库+弃牌堆），排除墨锭
    auto& store = DeckStore::instance();
    handCards_.clear();
    
    // 从手牌获取
    for (const auto& card : store.hand()) {
        if (card.id != "moding") {  // 排除墨锭
            handCards_.push_back(card);
        }
    }
    
    // 从牌库获取
    for (const auto& card : store.library()) {
        if (card.id != "moding") {  // 排除墨锭
            handCards_.push_back(card);
        }
    }
    
    // 从弃牌堆获取
    for (const auto& card : store.discard()) {
        if (card.id != "moding") {  // 排除墨锭
            handCards_.push_back(card);
        }
    }
    
    // 布局手牌
    layoutHandCards();
    
    // 初始化牌位位置
    int slotSize = 200;
    int slotGap = 50;
    int totalWidth = 2 * slotSize + slotGap;
    int startX = (screenW_ - totalWidth) / 2;
    int slotY = 150;
    
    leftSlotRect_ = {startX, slotY, slotSize, static_cast<int>(slotSize * 1.4f)};
    rightSlotRect_ = {startX + slotSize + slotGap, slotY, slotSize, static_cast<int>(slotSize * 1.4f)};
    
    // 重置状态
    rightSlotSelected_ = false;
    leftSlotFilled_ = false;
    hasSelectedCard_ = false;
    hasGeneratedCard_ = false;
    isAnimating_ = false;
    hoveredHandCardIndex_ = -1;
    hoverTime_ = 0.0f;
}

void InkGhostState::onExit(App& app) {
    if (titleFont_) {
        TTF_CloseFont(titleFont_);
        titleFont_ = nullptr;
    }
    if (smallFont_) {
        TTF_CloseFont(smallFont_);
        smallFont_ = nullptr;
    }
}

void InkGhostState::update(App& app, float deltaTime) {
    hoverTime_ += deltaTime;
    
    // 处理动画
    if (isAnimating_) {
        animTime_ += deltaTime;
        
        // 动画进行到一半时生成新卡牌
        if (animTime_ >= animDuration_ * 0.5f && !hasGeneratedCard_) {
            generateCard();
            addGeneratedCardToDeck();
        }
        
        if (animTime_ >= animDuration_) {
            isAnimating_ = false;
            animTime_ = 0.0f;
            // 动画完成后返回地图
            app.setState(std::make_unique<MapExploreState>());
        }
    }
}

void InkGhostState::render(App& app) {
    SDL_Renderer* r = app.getRenderer();
    
    // 清屏
    SDL_SetRenderDrawColor(r, 40, 35, 30, 255);
    SDL_RenderClear(r);
    
    // 渲染返回按钮（只在上帝模式下显示）
    if (backButton_ && App::isGodMode()) backButton_->render(r);
    
    // 渲染标题
    if (titleFont_) {
        SDL_Color titleColor{255, 255, 200, 255};
        SDL_Surface* titleSurface = TTF_RenderUTF8_Blended(titleFont_, u8"墨鬼", titleColor);
        if (titleSurface) {
            SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(r, titleSurface);
            SDL_Rect titleRect{(screenW_ - titleSurface->w) / 2, 50, titleSurface->w, titleSurface->h};
            SDL_RenderCopy(r, titleTexture, nullptr, &titleRect);
            SDL_DestroyTexture(titleTexture);
            SDL_FreeSurface(titleSurface);
        }
    }
    
    // 渲染说明文字
    if (smallFont_) {
        SDL_Color descColor{200, 200, 200, 255};
        SDL_Surface* descSurface = TTF_RenderUTF8_Blended(smallFont_, u8"点击右侧牌位选择卡牌，生成相同牌面的新卡牌（从所有牌堆中检索，排除墨锭）", descColor);
        if (descSurface) {
            SDL_Texture* descTexture = SDL_CreateTextureFromSurface(r, descSurface);
            SDL_Rect descRect{(screenW_ - descSurface->w) / 2, 100, descSurface->w, descSurface->h};
            SDL_RenderCopy(r, descTexture, nullptr, &descRect);
            SDL_DestroyTexture(descTexture);
            SDL_FreeSurface(descSurface);
        }
    }
    
    // 渲染牌位
    renderCardSlots(app);
    
    // 渲染选中的卡牌
    if (hasSelectedCard_) {
        renderSelectedCard(app);
    }
    
    // 渲染生成的卡牌
    if (hasGeneratedCard_) {
        renderGeneratedCard(app);
    }
    
    // 渲染手牌区
    renderHandCards(app);
    
    // 渲染说明文字
    if (smallFont_) {
        SDL_Color descColor{200, 200, 200, 255};
        std::string desc = u8"点击右侧牌位选择一张卡牌，左侧将生成相同牌面的新卡牌";
        SDL_Surface* descSurface = TTF_RenderUTF8_Blended(smallFont_, desc.c_str(), descColor);
        if (descSurface) {
            SDL_Texture* descTexture = SDL_CreateTextureFromSurface(r, descSurface);
            SDL_Rect descRect{(screenW_ - descSurface->w) / 2, screenH_ - 50, descSurface->w, descSurface->h};
            SDL_RenderCopy(r, descTexture, nullptr, &descRect);
            SDL_DestroyTexture(descTexture);
            SDL_FreeSurface(descSurface);
        }
    }
    
    // 渲染全局印记提示
    CardRenderer::renderGlobalMarkTooltip(app, smallFont_);
}

void InkGhostState::handleEvent(App& app, const SDL_Event& event) {
    // 处理返回按钮事件
    if (backButton_) backButton_->handleEvent(event);
    
    if (event.type == SDL_MOUSEMOTION) {
        int mx = event.motion.x;
        int my = event.motion.y;
        
        // 处理印记悬停
        // 检查手牌中的印记悬停
        for (int i = 0; i < (int)handCardRects_.size(); ++i) {
            if (mx >= handCardRects_[i].x && mx <= handCardRects_[i].x + handCardRects_[i].w &&
                my >= handCardRects_[i].y && my <= handCardRects_[i].y + handCardRects_[i].h) {
                if (i < (int)handCards_.size()) {
                    CardRenderer::handleMarkHover(handCards_[i], handCardRects_[i], mx, my, smallFont_);
                    return;
                }
            }
        }
        
        // 检查右侧牌位中的印记悬停
        if (mx >= rightSlotRect_.x && mx <= rightSlotRect_.x + rightSlotRect_.w &&
            my >= rightSlotRect_.y && my <= rightSlotRect_.y + rightSlotRect_.h) {
            if (selectedCard_.id != "") {
                CardRenderer::handleMarkHover(selectedCard_, rightSlotRect_, mx, my, smallFont_);
                return;
            }
        }
        
        // 如果没有悬停在任何印记上，隐藏提示
        App::hideMarkTooltip();
        
        // 检查手牌悬停
        hoveredHandCardIndex_ = -1;
        for (int i = 0; i < (int)handCardRects_.size(); ++i) {
            if (mx >= handCardRects_[i].x && mx <= handCardRects_[i].x + handCardRects_[i].w &&
                my >= handCardRects_[i].y && my <= handCardRects_[i].y + handCardRects_[i].h) {
                hoveredHandCardIndex_ = i;
                break;
            }
        }
    }
    // 处理印记右键点击
    else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
        int mx = event.button.x;
        int my = event.button.y;
        
        // 检查手牌中的印记
        for (int i = 0; i < (int)handCardRects_.size(); ++i) {
            if (mx >= handCardRects_[i].x && mx <= handCardRects_[i].x + handCardRects_[i].w &&
                my >= handCardRects_[i].y && my <= handCardRects_[i].y + handCardRects_[i].h) {
                if (i < (int)handCards_.size()) {
                    CardRenderer::handleMarkClick(handCards_[i], handCardRects_[i], mx, my, smallFont_);
                    if (App::isMarkTooltipVisible()) {
                        return;
                    }
                }
            }
        }
        
        // 检查右侧牌位中的印记
        if (mx >= rightSlotRect_.x && mx <= rightSlotRect_.x + rightSlotRect_.w &&
            my >= rightSlotRect_.y && my <= rightSlotRect_.y + rightSlotRect_.h) {
            if (selectedCard_.id != "") {
                CardRenderer::handleMarkClick(selectedCard_, rightSlotRect_, mx, my, smallFont_);
                if (App::isMarkTooltipVisible()) {
                    return;
                }
            }
        }
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mx = event.button.x;
        int my = event.button.y;
        
        // 检查右侧牌位点击
        if (mx >= rightSlotRect_.x && mx <= rightSlotRect_.x + rightSlotRect_.w &&
            my >= rightSlotRect_.y && my <= rightSlotRect_.y + rightSlotRect_.h) {
            rightSlotSelected_ = true;
        }
        // 检查手牌点击（当右侧牌位被选中时）
        else if (rightSlotSelected_ && hoveredHandCardIndex_ >= 0 && hoveredHandCardIndex_ < (int)handCards_.size()) {
            // 选择卡牌
            selectedCard_ = handCards_[hoveredHandCardIndex_];
            hasSelectedCard_ = true;
            rightSlotSelected_ = false;
            
            // 开始生成动画（延时生成）
            isAnimating_ = true;
            animTime_ = 0.0f;
            animDuration_ = 2.0f;  // 2秒动画
        }
    }
    else if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_ESCAPE) {
            app.setState(std::make_unique<MapExploreState>());
        }
    }
}

void InkGhostState::layoutHandCards() {
    handCardRects_.clear();
    
    if (handCards_.empty()) return;
    
    int cardWidth = 120;
    int cardHeight = 168;
    int cardGap = 10;
    int startY = screenH_ - cardHeight - 100;
    
    // 计算总宽度
    int totalWidth = (int)handCards_.size() * cardWidth + ((int)handCards_.size() - 1) * cardGap;
    int startX = (screenW_ - totalWidth) / 2;
    
    for (int i = 0; i < (int)handCards_.size(); ++i) {
        SDL_Rect rect{startX + i * (cardWidth + cardGap), startY, cardWidth, cardHeight};
        handCardRects_.push_back(rect);
    }
}

void InkGhostState::renderHandCards(App& app) {
    SDL_Renderer* r = app.getRenderer();
    
    // 渲染手牌区域背景
    if (!handCards_.empty()) {
        SDL_SetRenderDrawColor(r, 60, 55, 50, 100);
        SDL_Rect handArea{0, screenH_ - 200, screenW_, 200};
        SDL_RenderFillRect(r, &handArea);
        
        // 渲染可用卡牌数量提示
        if (smallFont_) {
            SDL_Color countColor{255, 255, 255, 255};
            std::string countText = u8"可用卡牌数量: " + std::to_string(handCards_.size()) + u8" (已排除墨锭)";
            SDL_Surface* countSurface = TTF_RenderUTF8_Blended(smallFont_, countText.c_str(), countColor);
            if (countSurface) {
                SDL_Texture* countTexture = SDL_CreateTextureFromSurface(r, countSurface);
                SDL_Rect countRect{20, screenH_ - 180, countSurface->w, countSurface->h};
                SDL_RenderCopy(r, countTexture, nullptr, &countRect);
                SDL_DestroyTexture(countTexture);
                SDL_FreeSurface(countSurface);
            }
        }
    }
    
    for (int i = 0; i < (int)handCards_.size(); ++i) {
        const Card& card = handCards_[i];
        const SDL_Rect& rect = handCardRects_[i];
        
        // 悬停动画
        SDL_Rect renderRect = rect;
        if (i == hoveredHandCardIndex_) {
            float hoverOffset = sinf(hoverTime_ * 5.0f) * 5.0f;
            renderRect.y += (int)hoverOffset;
        }
        
        // 渲染卡牌
        CardRenderer::renderCard(app, card, renderRect, smallFont_, smallFont_, false);
    }
}

void InkGhostState::renderCardSlots(App& app) {
    SDL_Renderer* r = app.getRenderer();
    
    // 渲染牌位状态提示
    if (smallFont_) {
        SDL_Color statusColor{255, 255, 255, 255};
        std::string statusText;
        if (rightSlotSelected_) {
            statusText = u8"请从下方卡牌中选择一张（来自所有牌堆，已排除墨锭）";
        } else if (hasSelectedCard_ && isAnimating_) {
            if (animTime_ < animDuration_ * 0.5f) {
                statusText = u8"正在分析卡牌结构...";
            } else {
                statusText = u8"正在生成新卡牌...";
            }
        } else if (hasSelectedCard_) {
            statusText = u8"已选择卡牌，正在生成新卡牌...";
        } else {
            statusText = u8"点击右侧牌位开始选择";
        }
        
        SDL_Surface* statusSurface = TTF_RenderUTF8_Blended(smallFont_, statusText.c_str(), statusColor);
        if (statusSurface) {
            SDL_Texture* statusTexture = SDL_CreateTextureFromSurface(r, statusSurface);
            SDL_Rect statusRect{(screenW_ - statusSurface->w) / 2, 400, statusSurface->w, statusSurface->h};
            SDL_RenderCopy(r, statusTexture, nullptr, &statusRect);
            SDL_DestroyTexture(statusTexture);
            SDL_FreeSurface(statusSurface);
        }
    }
    
    // 渲染左侧牌位（生成卡牌位置）
    SDL_Color leftColor = leftSlotFilled_ ? SDL_Color{100, 150, 100, 255} : SDL_Color{80, 80, 80, 255};
    
    // 如果正在生成卡牌，添加发光效果
    if (isAnimating_ && hasGeneratedCard_) {
        float progress = (animTime_ - animDuration_ * 0.5f) / (animDuration_ * 0.5f);
        if (progress > 0.0f && progress < 1.0f) {
            // 发光效果：颜色变亮
            int glow = (int)(50 * (1.0f - progress));
            leftColor.r = std::min(255, leftColor.r + glow);
            leftColor.g = std::min(255, leftColor.g + glow);
            leftColor.b = std::min(255, leftColor.b + glow);
        }
    }
    
    SDL_SetRenderDrawColor(r, leftColor.r, leftColor.g, leftColor.b, leftColor.a);
    SDL_RenderFillRect(r, &leftSlotRect_);
    SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
    SDL_RenderDrawRect(r, &leftSlotRect_);
    
    // 渲染右侧牌位（选择卡牌位置）
    SDL_Color rightColor = rightSlotSelected_ ? SDL_Color{150, 100, 100, 255} : SDL_Color{100, 100, 150, 255};
    
    // 如果正在处理选中的卡牌，添加动画效果
    if (isAnimating_ && hasSelectedCard_) {
        float progress = animTime_ / animDuration_;
        // 脉冲效果
        float pulse = (sinf(progress * 3.14159f * 4.0f) + 1.0f) * 0.5f;
        int pulseIntensity = (int)(30 * pulse);
        rightColor.r = std::min(255, rightColor.r + pulseIntensity);
        rightColor.g = std::min(255, rightColor.g + pulseIntensity);
        rightColor.b = std::min(255, rightColor.b + pulseIntensity);
    }
    
    SDL_SetRenderDrawColor(r, rightColor.r, rightColor.g, rightColor.b, rightColor.a);
    SDL_RenderFillRect(r, &rightSlotRect_);
    SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
    SDL_RenderDrawRect(r, &rightSlotRect_);
    
    // 渲染牌位标签
    if (smallFont_) {
        SDL_Color labelColor{255, 255, 255, 255};
        
        // 左侧标签
        SDL_Surface* leftLabel = TTF_RenderUTF8_Blended(smallFont_, u8"生成卡牌", labelColor);
        if (leftLabel) {
            SDL_Texture* leftTexture = SDL_CreateTextureFromSurface(r, leftLabel);
            SDL_Rect leftLabelRect{leftSlotRect_.x + (leftSlotRect_.w - leftLabel->w) / 2, 
                                   leftSlotRect_.y + leftSlotRect_.h + 10, leftLabel->w, leftLabel->h};
            SDL_RenderCopy(r, leftTexture, nullptr, &leftLabelRect);
            SDL_DestroyTexture(leftTexture);
            SDL_FreeSurface(leftLabel);
        }
        
        // 右侧标签
        SDL_Surface* rightLabel = TTF_RenderUTF8_Blended(smallFont_, u8"选择卡牌", labelColor);
        if (rightLabel) {
            SDL_Texture* rightTexture = SDL_CreateTextureFromSurface(r, rightLabel);
            SDL_Rect rightLabelRect{rightSlotRect_.x + (rightSlotRect_.w - rightLabel->w) / 2, 
                                    rightSlotRect_.y + rightSlotRect_.h + 10, rightLabel->w, rightLabel->h};
            SDL_RenderCopy(r, rightTexture, nullptr, &rightLabelRect);
            SDL_DestroyTexture(rightTexture);
            SDL_FreeSurface(rightLabel);
        }
    }
}

void InkGhostState::renderSelectedCard(App& app) {
    // 在右侧牌位渲染选中的卡牌，添加动画效果
    SDL_Rect renderRect = rightSlotRect_;
    
    if (isAnimating_) {
        // 动画效果：卡牌逐渐变透明和缩小
        float progress = animTime_ / animDuration_;
        float scale = 1.0f - progress * 0.3f;  // 缩小到70%
        float alpha = 1.0f - progress * 0.5f;  // 变透明到50%
        
        // 调整渲染位置和大小
        int newWidth = (int)(renderRect.w * scale);
        int newHeight = (int)(renderRect.h * scale);
        renderRect.x += (renderRect.w - newWidth) / 2;
        renderRect.y += (renderRect.h - newHeight) / 2;
        renderRect.w = newWidth;
        renderRect.h = newHeight;
        
        // 设置透明度（需要修改CardRenderer来支持透明度）
        // 暂时使用缩放效果
    }
    
    CardRenderer::renderCard(app, selectedCard_, renderRect, smallFont_, smallFont_, false);
}

void InkGhostState::renderGeneratedCard(App& app) {
    // 在左侧牌位渲染生成的卡牌，添加生成动画效果
    SDL_Rect renderRect = leftSlotRect_;
    
    if (isAnimating_ && hasGeneratedCard_) {
        // 生成动画：卡牌从无到有，从小到大
        float progress = (animTime_ - animDuration_ * 0.5f) / (animDuration_ * 0.5f);
        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;
        
        // 使用缓动函数让动画更自然
        float easeOut = 1.0f - (1.0f - progress) * (1.0f - progress);
        float scale = 0.1f + easeOut * 0.9f;  // 从10%缩放到100%
        
        // 调整渲染位置和大小
        int newWidth = (int)(renderRect.w * scale);
        int newHeight = (int)(renderRect.h * scale);
        renderRect.x += (renderRect.w - newWidth) / 2;
        renderRect.y += (renderRect.h - newHeight) / 2;
        renderRect.w = newWidth;
        renderRect.h = newHeight;
    }
    
    CardRenderer::renderCard(app, generatedCard_, renderRect, smallFont_, smallFont_, false);
}

void InkGhostState::generateCard() {
    // 复制选中的卡牌
    generatedCard_ = selectedCard_;
    
    // 50%概率改变卡牌属性
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> changeDist(0, 1);
    
    if (changeDist(gen) == 1) { // 50%概率改变
        std::uniform_int_distribution<int> propertyDist(0, 2); // 0: 印记, 1: 生命, 2: 攻击力
        int property = propertyDist(gen);
        
        switch (property) {
            case 0: { // 改变印记
                std::vector<std::string> marks = {u8"不死印记", u8"空袭", u8"护主", u8"双重攻击", u8"水袭", u8"坚硬之躯"};
                std::uniform_int_distribution<int> markDist(0, (int)marks.size() - 1);
                std::string newMark = marks[markDist(gen)];
                
                // 移除所有现有印记，添加新印记
                generatedCard_.marks.clear();
                generatedCard_.marks.push_back(newMark);
                break;
            }
            case 1: { // 改变生命值
                // 相对改动，幅度不超过2点，且可降低
                std::uniform_int_distribution<int> deltaDist(-2, 2);
                int delta = deltaDist(gen);
                int base = selectedCard_.health;
                int updated = base + delta;
                if (updated < 0) updated = 0;
                // 确保变化幅度不超过2点（已由分布保证），并写回
                generatedCard_.health = updated;
                break;
            }
            case 2: { // 改变攻击力
                // 相对改动，幅度不超过2点，且可降低
                std::uniform_int_distribution<int> deltaDist(-2, 2);
                int delta = deltaDist(gen);
                int base = selectedCard_.attack;
                int updated = base + delta;
                if (updated < 0) updated = 0;
                generatedCard_.attack = updated;
                break;
            }
        }
    }
    
    hasGeneratedCard_ = true;
    leftSlotFilled_ = true;
}

void InkGhostState::addGeneratedCardToDeck() {
    // 将生成的卡牌添加到全局牌库中
    auto& store = DeckStore::instance();
    store.addToLibrary(generatedCard_);
}
