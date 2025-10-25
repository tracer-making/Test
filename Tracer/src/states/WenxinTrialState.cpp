#include "WenxinTrialState.h"
#include "MapExploreState.h"
#include "../core/App.h"
#include "../ui/CardRenderer.h"
#include "../core/Deck.h"
#include "../core/Cards.h"
#include <random>
#include <algorithm>
#include <set>
#include <map>

WenxinTrialState::WenxinTrialState() = default;

WenxinTrialState::~WenxinTrialState() {
    if (titleTex_) SDL_DestroyTexture(titleTex_);
    if (titleFont_) TTF_CloseFont(titleFont_);
    if (smallFont_) TTF_CloseFont(smallFont_);
    if (cardNameFont_) TTF_CloseFont(cardNameFont_);
    if (cardStatFont_) TTF_CloseFont(cardStatFont_);
    if (hintFont_) TTF_CloseFont(hintFont_);
    delete backButton_;
}

void WenxinTrialState::onEnter(App& app) {
    screenW_ = 1600; screenH_ = 1000; SDL_SetWindowSize(app.getWindow(), screenW_, screenH_);
    titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 56);
    smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 16);
    cardNameFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 20);
    cardStatFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 18);
    hintFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 24);
    if (titleFont_) { SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, u8"文心试炼", col); if (s) { titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), s); SDL_FreeSurface(s);} }

    backButton_ = new Button(); if (backButton_) { backButton_->setRect({20,20,120,36}); backButton_->setText(u8"返回地图"); if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer()); backButton_->setOnClick([this]() { pendingGoMapExplore_ = true; }); }

    buildTrialCards();
    layoutTrialCards();
}

void WenxinTrialState::onExit(App& app) {}

void WenxinTrialState::handleEvent(App& app, const SDL_Event& e) {
    // 处理按钮事件（只在上帝模式下）
    if (backButton_ && App::isGodMode()) backButton_->handleEvent(e);
    
    // 如果奖励卡牌准备好，处理点击事件
    if (rewardCardReady_ && e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;
        if (mx >= rewardCardRect_.x && mx <= rewardCardRect_.x + rewardCardRect_.w &&
            my >= rewardCardRect_.y && my <= rewardCardRect_.y + rewardCardRect_.h) {
            // 添加奖励卡牌到全局牌库
            DeckStore::instance().addToLibrary(rewardCard_);
            pendingGoMapExplore_ = true;
            return;
        }
    }
    
    // 如果试炼已开始，不处理试炼卡牌的交互
    if (trialStarted_) {
        return;
    }
    
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;
        
        // 检查点击试炼卡牌
        for (size_t i = 0; i < trialCards_.size(); ++i) {
            const SDL_Rect& rect = trialCards_[i].rect;
            if (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h) {
                if (!trialCards_[i].flipped) {
                    // 翻面
                    trialCards_[i].flipped = true;
                    message_ = u8"试炼内容：" + trialCards_[i].content;
                } else if (!trialCards_[i].completed) {
                    // 已翻面且未完成，开始试炼
                    startTrial((int)i);
                }
                break;
            }
        }
    }
    
    // 处理印记右键点击
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
        int mx = e.button.x, my = e.button.y;
        
        // 检查奖励卡牌中的印记
        if (rewardCardReady_) {
            if (mx >= rewardCardRect_.x && mx <= rewardCardRect_.x + rewardCardRect_.w &&
                my >= rewardCardRect_.y && my <= rewardCardRect_.y + rewardCardRect_.h) {
                CardRenderer::handleMarkClick(rewardCard_, rewardCardRect_, mx, my, cardStatFont_);
                if (App::isMarkTooltipVisible()) {
                    return;
                }
            }
        }
        
        // 检查试炼进行中抽到的卡牌中的印记
        if (trialStarted_ && !cardsFadingOut_ && !rewardCardReady_) {
            for (size_t i = 0; i < selectedCards_.size() && i < drawnCardRects_.size(); ++i) {
                if (i < cardsDrawn_) {
                    const SDL_Rect& rect = drawnCardRects_[i];
                    if (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h) {
                        CardRenderer::handleMarkClick(selectedCards_[i], rect, mx, my, cardStatFont_);
                        if (App::isMarkTooltipVisible()) {
                            return;
                        }
                    }
                }
            }
        }
    }
    
    // 处理鼠标移动事件，检查悬停
    if (e.type == SDL_MOUSEMOTION) {
        int mx = e.motion.x, my = e.motion.y;
        
        // 处理印记悬停
        // 检查奖励卡牌中的印记悬停
        if (rewardCardReady_) {
            if (mx >= rewardCardRect_.x && mx <= rewardCardRect_.x + rewardCardRect_.w &&
                my >= rewardCardRect_.y && my <= rewardCardRect_.y + rewardCardRect_.h) {
                CardRenderer::handleMarkHover(rewardCard_, rewardCardRect_, mx, my, cardStatFont_);
                return;
            }
        }
        
        // 检查试炼进行中抽到的卡牌中的印记悬停
        if (trialStarted_ && !cardsFadingOut_ && !rewardCardReady_) {
            for (size_t i = 0; i < selectedCards_.size() && i < drawnCardRects_.size(); ++i) {
                if (i < cardsDrawn_) {
                    const SDL_Rect& rect = drawnCardRects_[i];
                    if (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h) {
                        CardRenderer::handleMarkHover(selectedCards_[i], rect, mx, my, cardStatFont_);
                        return;
                    }
                }
            }
        }
        
        // 如果没有悬停在任何印记上，隐藏提示
        App::hideMarkTooltip();
        
        hoveredTrialIndex_ = -1;
        for (size_t i = 0; i < trialCards_.size(); ++i) {
            const SDL_Rect& rect = trialCards_[i].rect;
            if (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h) {
                if (trialCards_[i].flipped) {
                    hoveredTrialIndex_ = (int)i;
                }
                break;
            }
        }
    }
}

void WenxinTrialState::update(App& app, float dt) {
    // 更新抽牌动画
    if (drawAnim_.active) {
        drawAnim_.time += dt;
        if (drawAnim_.time >= drawAnim_.duration) {
            drawAnim_.active = false;
            cardsDrawn_++; // 动画结束后才增加计数
            
            // 动画结束，开始抽下一张牌
            if (cardsDrawn_ < 3) {
                drawNextCard();
            } else {
                // 所有牌抽完，开始计算结果
                calculatingResult_ = true;
                calculationTime_ = 0.0f;
            }
        }
    }
    
    // 更新计算结果动画
    if (calculatingResult_) {
        calculationTime_ += dt;
        
        // 每0.8秒显示一张卡牌的计算结果
        int targetStep = (int)(calculationTime_ / 0.8f);
        if (targetStep > currentCalculationStep_ && targetStep <= (int)selectedCards_.size()) {
            currentCalculationStep_ = targetStep;
            updateCalculationText();
        }
        
        // 所有卡牌计算完成后，再等待2秒显示最终结果，然后开始淡出
        if (calculationTime_ >= 0.8f * selectedCards_.size() + 2.0f) {
            finishTrial();
        }
    }
    
    // 更新卡牌淡出动画
    if (cardsFadingOut_) {
        fadeOutTime_ += dt;
        if (fadeOutTime_ >= fadeOutDuration_) {
            cardsFadingOut_ = false;
            if (trialSuccess_) {
                // 试炼成功，生成奖励卡牌
                generateRewardCard();
            } else {
                // 试炼失败，直接返回地图
                pendingGoMapExplore_ = true;
            }
        }
    }
    
    if (pendingGoMapExplore_) { 
        pendingGoMapExplore_ = false; 
        app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState()))); 
    }
}

void WenxinTrialState::render(App& app) {
    SDL_Renderer* r = app.getRenderer();
    SDL_SetRenderDrawColor(r, 18, 22, 32, 255);
    SDL_RenderClear(r);
    
    // 标题（已删除）
    // if (titleTex_) { 
    //     int tw, th; SDL_QueryTexture(titleTex_, nullptr, nullptr, &tw, &th); 
    //     SDL_Rect d{ (screenW_ - tw) / 2, 60, tw, th }; 
    //     SDL_RenderCopy(r, titleTex_, nullptr, &d); 
    // }
    
    // 返回按钮（只在上帝模式下显示）
    if (backButton_ && App::isGodMode()) backButton_->render(r);

    // 如果试炼已开始，不显示试炼卡牌
    if (!trialStarted_) {
        // 渲染试炼卡牌
        for (const auto& trialCard : trialCards_) {
            if (trialCard.flipped) {
            // 已翻面，显示试炼内容
            SDL_SetRenderDrawColor(r, 235, 230, 220, 230);
            SDL_RenderFillRect(r, &trialCard.rect);
            SDL_SetRenderDrawColor(r, 60, 50, 40, 220);
            SDL_RenderDrawRect(r, &trialCard.rect);
            
            // 渲染试炼内容文字（居中）
            if (cardNameFont_) {
                SDL_Color col{60, 50, 40, 255};
                SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(cardNameFont_, trialCard.content.c_str(), col, trialCard.rect.w - 20);
                if (s) {
                    SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                    SDL_Rect textRect{
                        trialCard.rect.x + (trialCard.rect.w - s->w) / 2,  // 水平居中
                        trialCard.rect.y + (trialCard.rect.h - s->h) / 2,  // 垂直居中
                        s->w,
                        s->h
                    };
                    SDL_RenderCopy(r, t, nullptr, &textRect);
                    SDL_DestroyTexture(t);
                    SDL_FreeSurface(s);
                }
            }
        } else {
            // 未翻面，显示卡牌背面
            SDL_SetRenderDrawColor(r, 100, 80, 60, 230);
            SDL_RenderFillRect(r, &trialCard.rect);
            SDL_SetRenderDrawColor(r, 60, 50, 40, 220);
            SDL_RenderDrawRect(r, &trialCard.rect);
            
            // 渲染"文心试炼"文字
            if (cardNameFont_) {
                SDL_Color col{200, 180, 160, 255};
                SDL_Surface* s = TTF_RenderUTF8_Blended(cardNameFont_, u8"文心试炼", col);
                if (s) {
                    SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                    SDL_Rect textRect{
                        trialCard.rect.x + (trialCard.rect.w - s->w) / 2,
                        trialCard.rect.y + (trialCard.rect.h - s->h) / 2,
                        s->w,
                        s->h
                    };
                    SDL_RenderCopy(r, t, nullptr, &textRect);
                    SDL_DestroyTexture(t);
                    SDL_FreeSurface(s);
                }
            }
        }
        }
    } else {
        // 试炼进行中，渲染抽到的卡牌（但不在动画过程中显示）
        if (!cardsFadingOut_ && !rewardCardReady_) {
            for (size_t i = 0; i < selectedCards_.size() && i < drawnCardRects_.size(); ++i) {
                // 只渲染已经完成动画的卡牌（i < cardsDrawn_）
                if (i < cardsDrawn_) {
                    CardRenderer::renderCard(app, selectedCards_[i], drawnCardRects_[i], cardNameFont_, cardStatFont_, false);
                }
            }
        } else if (cardsFadingOut_) {
            // 卡牌淡出动画
            float fadeProgress = fadeOutTime_ / fadeOutDuration_;
            float alpha = 1.0f - fadeProgress;
            
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            for (size_t i = 0; i < selectedCards_.size() && i < drawnCardRects_.size(); ++i) {
                if (i < cardsDrawn_) {
                    // 渲染半透明卡牌背景
                    SDL_SetRenderDrawColor(r, 255, 255, 255, (Uint8)(255 * alpha));
                    SDL_RenderFillRect(r, &drawnCardRects_[i]);
                    
                    // 渲染半透明卡牌边框
                    SDL_SetRenderDrawColor(r, 100, 100, 100, (Uint8)(255 * alpha));
                    SDL_RenderDrawRect(r, &drawnCardRects_[i]);
                    
                    // 渲染半透明卡牌名称
                    if (cardNameFont_) {
                        SDL_Color col{0, 0, 0, (Uint8)(255 * alpha)};
                        SDL_Surface* s = TTF_RenderUTF8_Blended(cardNameFont_, selectedCards_[i].name.c_str(), col);
                        if (s) {
                            SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                            SDL_SetTextureAlphaMod(t, (Uint8)(255 * alpha));
                            SDL_Rect textRect{
                                drawnCardRects_[i].x + (drawnCardRects_[i].w - s->w) / 2,
                                drawnCardRects_[i].y + (drawnCardRects_[i].h - s->h) / 2,
                                s->w,
                                s->h
                            };
                            SDL_RenderCopy(r, t, nullptr, &textRect);
                            SDL_DestroyTexture(t);
                            SDL_FreeSurface(s);
                        }
                    }
                }
            }
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        }
        
        // 渲染抽牌动画（淡入效果）
        if (drawAnim_.active) {
            float progress = drawAnim_.time / drawAnim_.duration;
            float alpha = progress; // 从0到1淡入
            
            // 创建半透明的卡牌渲染
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            
            // 渲染半透明背景
            SDL_SetRenderDrawColor(r, 255, 255, 255, (Uint8)(255 * alpha));
            SDL_RenderFillRect(r, &drawAnim_.toRect);
            
            // 渲染卡牌边框
            SDL_SetRenderDrawColor(r, 100, 100, 100, (Uint8)(255 * alpha));
            SDL_RenderDrawRect(r, &drawAnim_.toRect);
            
            // 渲染卡牌内容（需要修改CardRenderer来支持透明度）
            // 暂时用简单的方式渲染卡牌名称
            if (cardNameFont_) {
                SDL_Color col{0, 0, 0, (Uint8)(255 * alpha)};
                SDL_Surface* s = TTF_RenderUTF8_Blended(cardNameFont_, drawAnim_.card.name.c_str(), col);
                if (s) {
                    SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                    SDL_SetTextureAlphaMod(t, (Uint8)(255 * alpha));
                    SDL_Rect textRect{
                        drawAnim_.toRect.x + (drawAnim_.toRect.w - s->w) / 2,
                        drawAnim_.toRect.y + (drawAnim_.toRect.h - s->h) / 2,
                        s->w,
                        s->h
                    };
                    SDL_RenderCopy(r, t, nullptr, &textRect);
                    SDL_DestroyTexture(t);
                    SDL_FreeSurface(s);
                }
            }
            
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        }
        
        // 渲染计算过程文字（在牌位下面居中）
        if (calculatingResult_ && !calculationText_.empty() && !cardsFadingOut_) {
            if (hintFont_) {
                SDL_Color col{255, 255, 200, 255};
                SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(hintFont_, calculationText_.c_str(), col, screenW_ - 100);
                if (s) {
                    SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                    // 计算牌位下方的位置
                    int cardBottomY = 0;
                    if (!drawnCardRects_.empty()) {
                        cardBottomY = drawnCardRects_[0].y + drawnCardRects_[0].h;
                    }
                    SDL_Rect d{(screenW_ - s->w) / 2, cardBottomY + 20, s->w, s->h};
                    SDL_RenderCopy(r, t, nullptr, &d);
                    SDL_DestroyTexture(t);
                    SDL_FreeSurface(s);
                }
            }
        }
        
        // 渲染奖励卡牌
        if (rewardCardReady_) {
            CardRenderer::renderCard(app, rewardCard_, rewardCardRect_, cardNameFont_, cardStatFont_, false);
        }
    }

    // 渲染悬停提示（在牌上方一点点）
    if (hoveredTrialIndex_ >= 0 && hoveredTrialIndex_ < (int)trialCards_.size()) {
        const auto& hoveredTrial = trialCards_[hoveredTrialIndex_];
        if (hintFont_) {
            SDL_Color col{255, 255, 200, 255};
            SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(hintFont_, hoveredTrial.description.c_str(), col, screenW_ - 40);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                // 显示在牌上方一点点
                int hintY = trialCards_[hoveredTrialIndex_].rect.y - s->h - 10;
                SDL_Rect d{(screenW_ - s->w) / 2, hintY, s->w, s->h};
                SDL_RenderCopy(r, t, nullptr, &d);
                SDL_DestroyTexture(t);
                SDL_FreeSurface(s);
            }
        }
    }

    // 渲染消息
    if (!message_.empty() && smallFont_) {
        SDL_Color col{200, 230, 255, 255};
        SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, message_.c_str(), col, screenW_ - 40);
        if (s) {
            SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
            SDL_Rect d{20, screenH_ - s->h - 20, s->w, s->h};
            SDL_RenderCopy(r, t, nullptr, &d);
            SDL_DestroyTexture(t);
            SDL_FreeSurface(s);
        }
    }
    
    // 渲染全局印记提示
    CardRenderer::renderGlobalMarkTooltip(app, cardStatFont_);
}

void WenxinTrialState::buildTrialCards() {
    trialCards_.clear();
    
    // 随机选择三种试炼类型
    std::vector<TrialType> allTypes = {
        TrialType::Wisdom, TrialType::Bone, TrialType::Power, 
        TrialType::Life, TrialType::Tribe, TrialType::Blood
    };
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(allTypes.begin(), allTypes.end(), gen);
    
    // 创建三个试炼卡牌
    for (int i = 0; i < 3; ++i) {
        TrialCard card;
        card.type = allTypes[i];
        card.content = getTrialTypeName(card.type);
        card.description = getTrialDescription(card.type);
        trialCards_.push_back(card);
    }
}

void WenxinTrialState::layoutTrialCards() {
    int cardW = 150, cardH = 210;
    int gap = 40;
    int totalW = 3 * cardW + 2 * gap;
    int startX = (screenW_ - totalW) / 2;
    int startY = (screenH_ - cardH) / 2;
    
    for (size_t i = 0; i < trialCards_.size(); ++i) {
        trialCards_[i].rect = {
            startX + (int)i * (cardW + gap),
            startY,
            cardW,
            cardH
        };
    }
}

void WenxinTrialState::startTrial(int trialIndex) {
    if (trialIndex < 0 || trialIndex >= (int)trialCards_.size()) return;
    
    currentTrialIndex_ = trialIndex;
    trialStarted_ = true;
    selectedCards_.clear();
    cardsDrawn_ = 0;
    calculatingResult_ = false;
    calculationTime_ = 0.0f;
    currentCalculationStep_ = 0;
    calculationText_ = "";
    trialCompleted_ = false;
    trialSuccess_ = false;
    cardsFadingOut_ = false;
    fadeOutTime_ = 0.0f;
    rewardCardReady_ = false;
    
    // 从全局牌库随机抽取3张卡牌
    auto& library = DeckStore::instance().library();
    if (library.size() < 3) {
        message_ = u8"牌库中卡牌不足，无法进行试炼";
        return;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, (int)library.size() - 1);
    
    std::set<int> selectedIndices;
    while (selectedIndices.size() < 3) {
        selectedIndices.insert(dis(gen));
    }
    
    for (int idx : selectedIndices) {
        selectedCards_.push_back(library[idx]);
    }
    
    // 设置抽到的卡牌显示位置（中心排列）
    drawnCardRects_.clear();
    int cardW = 120, cardH = 180;
    int gap = 20;
    int totalW = 3 * cardW + 2 * gap;
    int startX = (screenW_ - totalW) / 2;
    int startY = (screenH_ - cardH) / 2;
    
    for (int i = 0; i < 3; ++i) {
        drawnCardRects_.push_back({
            startX + i * (cardW + gap),
            startY,
            cardW,
            cardH
        });
    }
    
    // 开始抽第一张牌
    message_ = u8"开始抽牌...";
    drawNextCard();
}

void WenxinTrialState::drawNextCard() {
    if (cardsDrawn_ >= 3) return;
    
    // 设置抽牌动画
    drawAnim_.active = true;
    drawAnim_.time = 0.0f;
    drawAnim_.card = selectedCards_[cardsDrawn_];
    
    // 设置动画位置（在目标位置淡入）
    drawAnim_.fromRect = drawnCardRects_[cardsDrawn_];
    drawAnim_.toRect = drawnCardRects_[cardsDrawn_];
    
    message_ = u8"正在抽取第" + std::to_string(cardsDrawn_ + 1) + u8"张牌...";
    
    // 注意：cardsDrawn_++ 在动画结束后才执行
}

void WenxinTrialState::updateCalculationText() {
    if (currentTrialIndex_ < 0 || currentTrialIndex_ >= (int)trialCards_.size()) return;
    
    TrialType type = trialCards_[currentTrialIndex_].type;
    calculationText_ = "";
    
    int totalValue = 0;
    for (int i = 0; i < currentCalculationStep_ && i < (int)selectedCards_.size(); ++i) {
        const Card& card = selectedCards_[i];
        int value = 0;
        
        switch (type) {
        case TrialType::Wisdom: {
            // 只统计随机池中的印记
            static const std::vector<std::string> availableMarks = {
                u8"空袭", u8"水袭", u8"高跳", u8"护主", u8"领袖力量", u8"掘墓人",
                u8"双重攻击", u8"双向攻击", u8"三向攻击", u8"冲刺能手", u8"蛮力冲撞",
                u8"生生不息",  u8"不死印记", u8"优质祭品", u8"丰产之巢", u8"一回合成长",
                u8"内心之蜂", u8"滋生寄生虫", u8"断尾求生", u8"反伤", u8"死神之触",
                u8"臭臭", u8"蚁后", u8"一口之量", u8"坚硬之躯", u8"守护者",
                u8"兔窝", u8"筑坝师", u8"检索", u8"道具商", u8"食尸鬼", u8"骨王"
            };
            
            value = 0;
            for (const auto& mark : card.marks) {
                if (std::find(availableMarks.begin(), availableMarks.end(), mark) != availableMarks.end()) {
                    value++;
                }
            }
            calculationText_ += card.name + u8"：" + std::to_string(value) + u8"个随机池印记\n";
            break;
        }
        case TrialType::Bone:
            value = card.sacrificeCost;
            calculationText_ += card.name + u8"：" + std::to_string(value) + u8"根魂骨\n";
            break;
        case TrialType::Power:
            value = card.attack;
            calculationText_ += card.name + u8"：" + std::to_string(value) + u8"点攻击\n";
            break;
        case TrialType::Life:
            value = card.health;
            calculationText_ += card.name + u8"：" + std::to_string(value) + u8"点生命\n";
            break;
        case TrialType::Tribe:
            // 部族试炼特殊处理
            calculationText_ += card.name + u8"：" + card.category + u8"\n";
            break;
        case TrialType::Blood: {
            // 血腥试炼：墨量=献祭消耗，0或带有“消耗骨头”视作0
            bool hasBoneCost = false;
            for (const auto& m : card.marks) if (m == u8"消耗骨头") { hasBoneCost = true; break; }
            value = (hasBoneCost || card.sacrificeCost <= 0) ? 0 : card.sacrificeCost;
            calculationText_ += card.name + u8"：" + std::to_string(value) + u8"点墨量\n";
            break;
        }
        }
        
        if (type != TrialType::Tribe) {
            totalValue += value;
        }
    }
    
    if (currentCalculationStep_ > 0 && type != TrialType::Tribe) {
        calculationText_ += u8"........\n";
        calculationText_ += u8"共计" + std::to_string(totalValue) + u8"点";
    }
}

void WenxinTrialState::finishTrial() {
    calculatingResult_ = false;
    trialCompleted_ = true;
    
    // 检查试炼完成条件
    bool completed = checkTrialCompletion(selectedCards_, trialCards_[currentTrialIndex_].type);
    trialCards_[currentTrialIndex_].completed = completed;
    trialSuccess_ = completed;
    
    if (completed) {
        message_ = u8"试炼完成！" + trialCards_[currentTrialIndex_].content;
    } else {
        message_ = u8"试炼失败！" + trialCards_[currentTrialIndex_].description;
    }
    
    // 开始卡牌淡出动画
    startCardsFadeOut();
}

bool WenxinTrialState::checkTrialCompletion(const std::vector<Card>& cards, TrialType type) {
    switch (type) {
    case TrialType::Wisdom: {
        // 智慧试炼：至少3个随机池印记
        static const std::vector<std::string> availableMarks = {
            u8"空袭", u8"水袭", u8"高跳", u8"护主", u8"领袖力量", u8"掘墓人",
            u8"双重攻击", u8"双向攻击", u8"三向攻击", u8"冲刺能手", u8"蛮力冲撞",
            u8"生生不息",  u8"不死印记", u8"优质祭品", u8"丰产之巢", u8"一回合成长",
            u8"内心之蜂", u8"滋生寄生虫", u8"断尾求生", u8"反伤", u8"死神之触",
            u8"臭臭", u8"蚁后", u8"一口之量", u8"坚硬之躯", u8"守护者",
            u8"兔窝", u8"筑坝师", u8"检索", u8"道具商", u8"食尸鬼", u8"骨王"
        };
        
        int totalMarks = 0;
        for (const auto& card : cards) {
            for (const auto& mark : card.marks) {
                // 只统计随机池中的印记
                if (std::find(availableMarks.begin(), availableMarks.end(), mark) != availableMarks.end()) {
                    totalMarks++;
                }
            }
        }
        return totalMarks >= 3;
    }
    case TrialType::Bone: {
        // 魂骨试炼：至少5根魂骨
        int totalBones = 0;
        for (const auto& card : cards) {
            totalBones += card.sacrificeCost;
        }
        return totalBones >= 5;
    }
    case TrialType::Power: {
        // 力量试炼：至少4点攻击力
        int totalAttack = 0;
        for (const auto& card : cards) {
            totalAttack += card.attack;
        }
        return totalAttack >= 4;
    }
    case TrialType::Life: {
        // 生命试炼：至少6点生命值
        int totalHealth = 0;
        for (const auto& card : cards) {
            totalHealth += card.health;
        }
        return totalHealth >= 6;
    }
    case TrialType::Tribe: {
        // 同族试炼：至少2张同部族（不算其他）
        std::map<std::string, int> tribeCount;
        for (const auto& card : cards) {
            if (card.category != u8"其他") {
                tribeCount[card.category]++;
            }
        }
        for (const auto& pair : tribeCount) {
            if (pair.second >= 2) {
                return true;
            }
        }
        return false;
    }
    case TrialType::Blood: {
        // 血腥试炼：三张牌合计墨量至少4（无消耗或带“消耗骨头”视为0）
        int totalInk = 0;
        for (const auto& card : cards) {
            bool hasBoneCost = false;
            for (const auto& m : card.marks) if (m == u8"消耗骨头") { hasBoneCost = true; break; }
            int ink = (hasBoneCost || card.sacrificeCost <= 0) ? 0 : card.sacrificeCost;
            totalInk += ink;
        }
        return totalInk >= 4;
    }
    default:
        return false;
    }
}

std::string WenxinTrialState::getTrialTypeName(TrialType type) {
    switch (type) {
    case TrialType::Wisdom: return u8"智慧试炼";
    case TrialType::Bone: return u8"魂骨试炼";
    case TrialType::Power: return u8"力量试炼";
    case TrialType::Life: return u8"生命试炼";
    case TrialType::Tribe: return u8"同族试炼";
    case TrialType::Blood: return u8"血腥试炼";
    default:     return u8"未知试炼";
    }
}

void WenxinTrialState::startCardsFadeOut() {
    cardsFadingOut_ = true;
    fadeOutTime_ = 0.0f;
    message_ = u8"卡牌正在淡出...";
}

void WenxinTrialState::generateRewardCard() {
    // 从所有牌库（CardDB）中随机选择一张可获取的卡牌
    auto allCardIds = CardDB::instance().allIds();
    std::vector<std::string> obtainableIds;
    
    // 过滤出普通卡牌（obtainable == 1）
    for (const auto& id : allCardIds) {
        Card c = CardDB::instance().make(id);
        if (c.obtainable == 1) {
            obtainableIds.push_back(id);
        }
    }
    
    if (obtainableIds.empty()) {
        // 如果没有可获取的卡牌，使用默认卡牌
        rewardCard_ = CardDB::instance().make("shulin_shucheng");
    } else {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, (int)obtainableIds.size() - 1);
        std::string randomCardId = obtainableIds[dis(gen)];
        rewardCard_ = CardDB::instance().make(randomCardId);
    }
    
    // 添加两个不重复的随机印记
    static const std::vector<std::string> availableMarks = {
        u8"空袭", u8"水袭", u8"高跳", u8"护主", u8"领袖力量", u8"掘墓人",
        u8"双重攻击", u8"双向攻击", u8"三向攻击", u8"冲刺能手", u8"蛮力冲撞",
        u8"生生不息",  u8"不死印记", u8"优质祭品", u8"丰产之巢", u8"一回合成长",
        u8"内心之蜂", u8"滋生寄生虫", u8"断尾求生", u8"反伤", u8"死神之触",
        u8"臭臭", u8"蚁后", u8"一口之量", u8"坚硬之躯", u8"守护者",
        u8"兔窝", u8"筑坝师", u8"检索", u8"道具商", u8"食尸鬼", u8"骨王"
    };
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, (int)availableMarks.size() - 1);
    
    std::set<std::string> addedMarks;
    int attempts = 0;
    while (addedMarks.size() < 2 && attempts < 20) {
        std::string mark = availableMarks[dis(gen)];
        if (addedMarks.find(mark) == addedMarks.end() && 
            std::find(rewardCard_.marks.begin(), rewardCard_.marks.end(), mark) == rewardCard_.marks.end()) {
            rewardCard_.marks.push_back(mark);
            addedMarks.insert(mark);
        }
        attempts++;
    }
    
    // 设置奖励卡牌为不可传承
    rewardCard_.canInherit = false;
    
    // 设置奖励卡牌位置（屏幕中央）
    int cardW = 120, cardH = 180;
    rewardCardRect_ = {
        (screenW_ - cardW) / 2,
        (screenH_ - cardH) / 2,
        cardW,
        cardH
    };
    
    rewardCardReady_ = true;
    message_ = u8"请领取奖励";
}

std::string WenxinTrialState::getTrialDescription(TrialType type) {
    switch (type) {
    case TrialType::Wisdom: return u8"要求抽取的三张牌必须共计拥有至少3个印记才能完成";
    case TrialType::Bone: return u8"要求抽取的三张牌必须共计消耗至少5根魂骨才能完成";
    case TrialType::Power: return u8"要求抽取的三张牌必须共计拥有4点攻击力才能完成";
    case TrialType::Life: return u8"要求抽取的三张牌必须共计拥有至少6点生命值方可通过";
    case TrialType::Tribe: return u8"要求抽取的三张牌至少2张来自同一部族（不算其他部族）方可通过";
    case TrialType::Blood: return u8"抽出的3张牌必须至少花费4点墨量才能通过（无消耗或带有消耗骨头的牌都算作0点）";
    default: return u8"未知试炼描述";
    }
}
