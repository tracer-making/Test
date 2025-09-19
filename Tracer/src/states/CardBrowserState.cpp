#include "CardBrowserState.h"
#include "../core/App.h"
#include "TestState.h"
#include "../core/Cards.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <algorithm>
#include <sstream>

CardBrowserState::CardBrowserState() {
    // Buttons will be initialized in onEnter
}

CardBrowserState::~CardBrowserState() {
    if (titleFont_) TTF_CloseFont(titleFont_);
    if (cardNameFont_) TTF_CloseFont(cardNameFont_);
    if (cardStatFont_) TTF_CloseFont(cardStatFont_);
    if (pageInfoFont_) TTF_CloseFont(pageInfoFont_);
}

void CardBrowserState::onEnter(App& app) {
    // Load fonts
    titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 24);
    cardNameFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 16);
    cardStatFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 12);
    pageInfoFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 14);
    _TTF_Font* smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 14);
    
    // Initialize buttons
    backButton_.setRect({50, 50, 120, 40});
    backButton_.setText("返回测试");
    backButton_.setFont(smallFont_, app.getRenderer());
    prevPageButton_.setRect({200, 50, 100, 40});
    prevPageButton_.setText("上一页");
    prevPageButton_.setFont(smallFont_, app.getRenderer());
    nextPageButton_.setRect({320, 50, 100, 40});
    nextPageButton_.setText("下一页");
    nextPageButton_.setFont(smallFont_, app.getRenderer());
    
    allCategoryButton_.setRect({50, 100, 80, 35});
    allCategoryButton_.setText("全部");
    allCategoryButton_.setFont(smallFont_, app.getRenderer());
    yuCategoryButton_.setRect({140, 100, 80, 35});
    yuCategoryButton_.setText("羽部");
    yuCategoryButton_.setFont(smallFont_, app.getRenderer());
    quanCategoryButton_.setRect({230, 100, 80, 35});
    quanCategoryButton_.setText("犬部");
    quanCategoryButton_.setFont(smallFont_, app.getRenderer());
    luCategoryButton_.setRect({320, 100, 80, 35});
    luCategoryButton_.setText("鹿部");
    luCategoryButton_.setFont(smallFont_, app.getRenderer());
    jieCategoryButton_.setRect({410, 100, 80, 35});
    jieCategoryButton_.setText("介部");
    jieCategoryButton_.setFont(smallFont_, app.getRenderer());
    linCategoryButton_.setRect({500, 100, 80, 35});
    linCategoryButton_.setText("鳞部");
    linCategoryButton_.setFont(smallFont_, app.getRenderer());
    otherCategoryButton_.setRect({590, 100, 80, 35});
    otherCategoryButton_.setText("其他");
    otherCategoryButton_.setFont(smallFont_, app.getRenderer());
    
    // Load all cards from CardDB
    allCards_.clear();
    auto allIds = CardDB::instance().allIds();
    for (const auto& id : allIds) {
        Card card = CardDB::instance().make(id);
        if (!card.id.empty()) {
            allCards_.push_back(card);
        }
    }
    
    // Initialize filtered cards
    filteredCards_ = allCards_;
    currentPage_ = 0;
    selectedCardIndex_ = -1;
    
    // Setup button callbacks
    backButton_.setOnClick([this]() {
        pendingBackToTest_ = true;
    });
    
    prevPageButton_.setOnClick([this]() {
        if (currentPage_ > 0) {
            currentPage_--;
            layoutCards();
        }
    });
    
    nextPageButton_.setOnClick([this]() {
        int maxPages = (filteredCards_.size() + cardsPerPage_ - 1) / cardsPerPage_;
        if (currentPage_ < maxPages - 1) {
            currentPage_++;
            layoutCards();
        }
    });
    
    allCategoryButton_.setOnClick([this]() {
        filterByCategory("全部");
    });
    
    yuCategoryButton_.setOnClick([this]() {
        filterByCategory("羽部");
    });
    
    quanCategoryButton_.setOnClick([this]() {
        filterByCategory("犬部");
    });
    
    luCategoryButton_.setOnClick([this]() {
        filterByCategory("鹿部");
    });
    
    jieCategoryButton_.setOnClick([this]() {
        filterByCategory("介部");
    });
    
    linCategoryButton_.setOnClick([this]() {
        filterByCategory("鳞部");
    });
    
    otherCategoryButton_.setOnClick([this]() {
        filterByCategory("其他");
    });
    
    layoutCards();
    layoutButtons();
}

void CardBrowserState::onExit(App& app) {
    // Cleanup handled in destructor
}

void CardBrowserState::handleEvent(App& app, const SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX = event.button.x;
        int mouseY = event.button.y;
        
        // Check card clicks
        for (size_t i = 0; i < cardRects_.size(); i++) {
            if (mouseX >= cardRects_[i].x && mouseX <= cardRects_[i].x + cardRects_[i].w &&
                mouseY >= cardRects_[i].y && mouseY <= cardRects_[i].y + cardRects_[i].h) {
                selectedCardIndex_ = static_cast<int>(i);
                break;
            }
        }
        
        // Handle button clicks
        SDL_Event buttonEvent = event;
        backButton_.handleEvent(buttonEvent);
        prevPageButton_.handleEvent(buttonEvent);
        nextPageButton_.handleEvent(buttonEvent);
        allCategoryButton_.handleEvent(buttonEvent);
        yuCategoryButton_.handleEvent(buttonEvent);
        quanCategoryButton_.handleEvent(buttonEvent);
        luCategoryButton_.handleEvent(buttonEvent);
        jieCategoryButton_.handleEvent(buttonEvent);
        linCategoryButton_.handleEvent(buttonEvent);
        otherCategoryButton_.handleEvent(buttonEvent);
    }
}

void CardBrowserState::update(App& app, float deltaTime) {
    if (pendingBackToTest_) {
        app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState())));
        return;
    }
}

void CardBrowserState::render(App& app) {
    // Clear screen with dark background like DeckState
    SDL_SetRenderDrawColor(app.getRenderer(), 20, 24, 34, 255);
    SDL_RenderClear(app.getRenderer());
    
    // Render title with light color like DeckState
    if (titleFont_) {
        SDL_Color titleColor = {200, 220, 250, 255};
        SDL_Surface* titleSurface = TTF_RenderUTF8_Blended(titleFont_, "卡牌图鉴", titleColor);
        if (titleSurface) {
            SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(app.getRenderer(), titleSurface);
            SDL_Rect titleRect = {50, 10, titleSurface->w, titleSurface->h};
            SDL_RenderCopy(app.getRenderer(), titleTexture, nullptr, &titleRect);
            SDL_DestroyTexture(titleTexture);
            SDL_FreeSurface(titleSurface);
        }
    }
    
    // Render buttons
    backButton_.render(app.getRenderer());
    prevPageButton_.render(app.getRenderer());
    nextPageButton_.render(app.getRenderer());
    
    allCategoryButton_.render(app.getRenderer());
    yuCategoryButton_.render(app.getRenderer());
    quanCategoryButton_.render(app.getRenderer());
    luCategoryButton_.render(app.getRenderer());
    jieCategoryButton_.render(app.getRenderer());
    linCategoryButton_.render(app.getRenderer());
    otherCategoryButton_.render(app.getRenderer());
    
    // Render cards
    int startIndex = currentPage_ * cardsPerPage_;
    int endIndex = std::min(startIndex + cardsPerPage_, static_cast<int>(filteredCards_.size()));
    
    for (int i = startIndex; i < endIndex; i++) {
        int cardIndex = i - startIndex;
        if (cardIndex < static_cast<int>(cardRects_.size())) {
            bool selected = (selectedCardIndex_ == cardIndex);
            renderCard(app, filteredCards_[i], cardRects_[cardIndex].x, cardRects_[cardIndex].y, 
                      cardRects_[cardIndex].w, cardRects_[cardIndex].h, selected);
        }
    }
    
    // Render page info
    updatePageInfo();
    if (pageInfoFont_) {
        std::stringstream ss;
        ss << "第 " << (currentPage_ + 1) << " 页 / 共 " << ((filteredCards_.size() + cardsPerPage_ - 1) / cardsPerPage_) << " 页";
        ss << " (" << filteredCards_.size() << " 张卡牌)";
        
        SDL_Color infoColor = {200, 220, 250, 255};
        SDL_Surface* infoSurface = TTF_RenderUTF8_Blended(pageInfoFont_, ss.str().c_str(), infoColor);
        if (infoSurface) {
            SDL_Texture* infoTexture = SDL_CreateTextureFromSurface(app.getRenderer(), infoSurface);
            SDL_Rect infoRect = {50, 150, infoSurface->w, infoSurface->h};
            SDL_RenderCopy(app.getRenderer(), infoTexture, nullptr, &infoRect);
            SDL_DestroyTexture(infoTexture);
            SDL_FreeSurface(infoSurface);
        }
    }
}

void CardBrowserState::layoutCards() {
    cardRects_.clear();
    
    int cardWidth = 180;
    int cardHeight = 260;
    int marginX = 50;
    int marginY = 180;
    int spacingX = 24;
    int spacingY = 24;
    int cardsPerRow = 5; // 5列布局，2行
    
    int startIndex = currentPage_ * cardsPerPage_;
    int endIndex = std::min(startIndex + cardsPerPage_, static_cast<int>(filteredCards_.size()));
    
    for (int i = startIndex; i < endIndex; i++) {
        int cardIndex = i - startIndex;
        int row = cardIndex / cardsPerRow;
        int col = cardIndex % cardsPerRow;
        
        int x = marginX + col * (cardWidth + spacingX);
        int y = marginY + row * (cardHeight + spacingY);
        
        cardRects_.push_back({x, y, cardWidth, cardHeight});
    }
}

void CardBrowserState::layoutButtons() {
    // Button positions are set in constructor
}

void CardBrowserState::updatePageInfo() {
    // Page info is updated in render()
}

void CardBrowserState::filterByCategory(const std::string& category) {
    currentCategory_ = category;
    currentPage_ = 0;
    selectedCardIndex_ = -1;
    
    if (category == "全部") {
        filteredCards_ = allCards_;
    } else {
        filteredCards_.clear();
        for (const auto& card : allCards_) {
            if (card.category == category) {
                filteredCards_.push_back(card);
            }
        }
    }
    
    layoutCards();
}

void CardBrowserState::renderCard(App& app, const Card& card, int x, int y, int width, int height, bool selected) {
    SDL_Renderer* r = app.getRenderer();
    SDL_Rect cardRect = {x, y, width, height};
    
    // 纸面底色（模仿DeckState）
    SDL_SetRenderDrawColor(r, 235, 230, 220, 230);
    SDL_RenderFillRect(r, &cardRect);
    
    // 边框（深墨）
    SDL_SetRenderDrawColor(r, 60, 50, 40, 220);
    SDL_RenderDrawRect(r, &cardRect);
    
    // 选中时的高亮边框
    if (selected) {
        SDL_SetRenderDrawColor(r, 100, 150, 200, 255);
        SDL_RenderDrawRect(r, &cardRect);
    }
    
    // 角落小装饰点
    SDL_SetRenderDrawColor(r, 120, 110, 100, 150);
    SDL_Rect dots[4] = {
        {cardRect.x + 4, cardRect.y + 4, 2, 2},
        {cardRect.x + cardRect.w - 6, cardRect.y + 4, 2, 2},
        {cardRect.x + 4, cardRect.y + cardRect.h - 6, 2, 2},
        {cardRect.x + cardRect.w - 6, cardRect.y + cardRect.h - 6, 2, 2}
    };
    for (const auto& d : dots) SDL_RenderFillRect(r, &d);
    
    // 名称（顶部居中）使用更大字体，并在下方画明显分割线
    if (cardNameFont_) {
        SDL_Color nameCol{50, 40, 30, 255};
        SDL_Surface* s = TTF_RenderUTF8_Blended(cardNameFont_, card.name.c_str(), nameCol);
        if (s) {
            SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
            int desiredNameH = SDL_max(12, (int)(cardRect.h * 0.16f));
            float scaleN = (float)desiredNameH / (float)s->h;
            int scaledW = (int)(s->w * scaleN);
            int nx = cardRect.x + (cardRect.w - scaledW) / 2;
            SDL_Rect ndst{ nx, cardRect.y + (int)(cardRect.h * 0.04f), scaledW, desiredNameH };
            SDL_RenderCopy(r, t, nullptr, &ndst);
            SDL_DestroyTexture(t);
            
            // 分割线
            int lineY = ndst.y + ndst.h + SDL_max(2, (int)(cardRect.h * 0.015f));
            int thickness = SDL_max(1, (int)(cardRect.h * 0.007f));
            SDL_SetRenderDrawColor(r, 80, 70, 60, 220);
            for (int i = 0; i < thickness; ++i) {
                SDL_RenderDrawLine(r, cardRect.x + 6, lineY + i, cardRect.x + cardRect.w - 6, lineY + i);
            }
            SDL_FreeSurface(s);
        }
    }
    
    // 计算分割线位置
    int lineY = 0;
    if (cardNameFont_) {
        SDL_Color nameCol{50, 40, 30, 255};
        SDL_Surface* s = TTF_RenderUTF8_Blended(cardNameFont_, card.name.c_str(), nameCol);
        if (s) {
            int desiredNameH = SDL_max(12, (int)(cardRect.h * 0.16f));
            float scaleN = (float)desiredNameH / (float)s->h;
            int scaledW = (int)(s->w * scaleN);
            int nx = cardRect.x + (cardRect.w - scaledW) / 2;
            SDL_Rect ndst{ nx, cardRect.y + (int)(cardRect.h * 0.04f), scaledW, desiredNameH };
            lineY = ndst.y + ndst.h + SDL_max(2, (int)(cardRect.h * 0.015f));
            SDL_FreeSurface(s);
        }
    }
    
    // 攻击力（左下角）和生命值（右下角）
    if (cardStatFont_) {
        SDL_Color statCol{80, 50, 40, 255};
        int desiredStatH = SDL_max(12, (int)(cardRect.h * 0.18f));
        int margin = SDL_max(6, (int)(cardRect.h * 0.035f));
        
        // 攻击
        std::string attackText = std::to_string(card.attack);
        SDL_Surface* sa = TTF_RenderUTF8_Blended(cardStatFont_, attackText.c_str(), statCol);
        if (sa) {
            SDL_Texture* ta = SDL_CreateTextureFromSurface(r, sa);
            float scaleA = (float)desiredStatH / (float)sa->h;
            int aW = (int)(sa->w * scaleA);
            SDL_Rect adst{ cardRect.x + margin, cardRect.y + cardRect.h - desiredStatH - margin, aW, desiredStatH };
            SDL_RenderCopy(r, ta, nullptr, &adst);
            SDL_DestroyTexture(ta);
            
            // 剑形装饰
            SDL_SetRenderDrawColor(r, 60, 70, 100, 60);
            int swordY = adst.y + adst.h + SDL_max(1, (int)(cardRect.h * 0.006f));
            int swordW = SDL_max(adst.w, (int)(cardRect.w * 0.22f));
            int swordX = adst.x;
            SDL_RenderDrawLine(r, swordX, swordY, swordX + swordW, swordY);
            SDL_RenderDrawLine(r, swordX + swordW/3, swordY-2, swordX + swordW/3, swordY+2);
            SDL_RenderDrawLine(r, swordX + swordW, swordY, swordX + swordW - 4, swordY - 3);
            SDL_RenderDrawLine(r, swordX + swordW, swordY, swordX + swordW - 4, swordY + 3);
            SDL_FreeSurface(sa);
        }
        
        // 生命
        std::string healthText = std::to_string(card.health);
        SDL_Color hpCol{160, 30, 40, 255};
        SDL_Surface* sh = TTF_RenderUTF8_Blended(cardStatFont_, healthText.c_str(), hpCol);
        if (sh) {
            SDL_Texture* th = SDL_CreateTextureFromSurface(r, sh);
            float scaleH = (float)desiredStatH / (float)sh->h;
            int hW = (int)(sh->w * scaleH);
            SDL_Rect hdst{ cardRect.x + cardRect.w - hW - margin, cardRect.y + cardRect.h - desiredStatH - margin, hW, desiredStatH };
            SDL_RenderCopy(r, th, nullptr, &hdst);
            SDL_DestroyTexture(th);
            SDL_FreeSurface(sh);
        }
    }
    
    // 分类和献祭消耗（分割线下方）
    if (cardStatFont_ && lineY > 0) {
        int desiredStatH = SDL_max(10, (int)(cardRect.h * 0.12f));
        int spacing = desiredStatH + 4;
        
        // 分类（左侧）
        SDL_Color categoryCol{100, 80, 60, 255};
        SDL_Surface* s = TTF_RenderUTF8_Blended(cardStatFont_, card.category.c_str(), categoryCol);
        if (s) {
            SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
            float scale = (float)desiredStatH / (float)s->h;
            int scaledW = (int)(s->w * scale);
            SDL_Rect dst{ cardRect.x + 6, lineY + 4, scaledW, desiredStatH };
            SDL_RenderCopy(r, t, nullptr, &dst);
            SDL_DestroyTexture(t);
            SDL_FreeSurface(s);
        }
        
        // 献祭消耗（右侧）- 检查是否有"消耗骨头"印记
        if (card.sacrificeCost > 0) {
            bool hasBoneCost = false;
            for (const auto& mark : card.marks) {
                if (mark == "消耗骨头") {
                    hasBoneCost = true;
                    break;
                }
            }
            
            int dropSize = SDL_max(4, (int)(cardRect.h * 0.08f));
            int dropSpacing = dropSize + 2;
            int startX, startY = lineY + 4;
            
            if (card.sacrificeCost > 3) {
                // 超过3点：显示图案*数量
                startX = cardRect.x + cardRect.w - 6 - (dropSize + 20); // 为数字留出空间
                
                // 绘制一个图案
                int dropX = startX;
                int dropY = startY;
                
                if (hasBoneCost) {
                    // 绘制白骨图案（白色）
                    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
                    SDL_Rect boneRect = {dropX, dropY, dropSize, dropSize};
                    SDL_RenderFillRect(r, &boneRect);
                    // 白骨边框（深色）
                    SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
                    SDL_RenderDrawRect(r, &boneRect);
                    // 白骨内部线条
                    SDL_RenderDrawLine(r, dropX + 1, dropY + 1, dropX + dropSize - 2, dropY + dropSize - 2);
                    SDL_RenderDrawLine(r, dropX + dropSize - 2, dropY + 1, dropX + 1, dropY + dropSize - 2);
                } else {
                    // 绘制墨滴图案（黑色）
                    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
                    SDL_Rect dropRect = {dropX, dropY, dropSize, dropSize};
                    SDL_RenderFillRect(r, &dropRect);
                    // 水滴尖端
                    SDL_RenderDrawLine(r, dropX + dropSize/2, dropY + dropSize, dropX + dropSize/2, dropY + dropSize + dropSize/2);
                }
                
                // 绘制乘号和数量
                SDL_SetRenderDrawColor(r, 60, 50, 40, 255);
                int multX = dropX + dropSize + 2;
                int multY = dropY + dropSize/2;
                // 绘制乘号 "×"
                SDL_RenderDrawLine(r, multX, multY - 2, multX + 4, multY + 2);
                SDL_RenderDrawLine(r, multX + 4, multY - 2, multX, multY + 2);
                
                // 绘制数量文字
                if (cardStatFont_) {
                    std::string countStr = std::to_string(card.sacrificeCost);
                    SDL_Color countCol{60, 50, 40, 255};
                    SDL_Surface* s = TTF_RenderUTF8_Blended(cardStatFont_, countStr.c_str(), countCol);
                    if (s) {
                        SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                        float scale = (float)desiredStatH / (float)s->h;
                        int scaledW = (int)(s->w * scale);
                        int scaledH = (int)(s->h * scale);
                        SDL_Rect dst{ multX + 8, multY - scaledH/2, scaledW, scaledH };
                        SDL_RenderCopy(r, t, nullptr, &dst);
                        SDL_DestroyTexture(t);
                        SDL_FreeSurface(s);
                    }
                }
            } else {
                // 3点及以下：显示多个图案
                startX = cardRect.x + cardRect.w - 6 - (card.sacrificeCost * dropSpacing);
                
                for (int i = 0; i < card.sacrificeCost; i++) {
                    int dropX = startX + i * dropSpacing;
                    int dropY = startY;
                    
                    if (hasBoneCost) {
                        // 绘制白骨图案（白色）
                        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
                        SDL_Rect boneRect = {dropX, dropY, dropSize, dropSize};
                        SDL_RenderFillRect(r, &boneRect);
                        // 白骨边框（深色）
                        SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
                        SDL_RenderDrawRect(r, &boneRect);
                        // 白骨内部线条
                        SDL_RenderDrawLine(r, dropX + 1, dropY + 1, dropX + dropSize - 2, dropY + dropSize - 2);
                        SDL_RenderDrawLine(r, dropX + dropSize - 2, dropY + 1, dropX + 1, dropY + dropSize - 2);
                    } else {
                        // 绘制墨滴图案（黑色）
                        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
                        SDL_Rect dropRect = {dropX, dropY, dropSize, dropSize};
                        SDL_RenderFillRect(r, &dropRect);
                        // 水滴尖端
                        SDL_RenderDrawLine(r, dropX + dropSize/2, dropY + dropSize, dropX + dropSize/2, dropY + dropSize + dropSize/2);
                    }
                }
            }
        }
    }
    
    // 印记（分割线下方，居中）- 分行显示，但排除"消耗骨头"印记
    if (cardStatFont_ && !card.marks.empty() && lineY > 0) {
        SDL_Color markCol{120, 80, 50, 255};
        int desiredStatH = SDL_max(8, (int)(cardRect.h * 0.10f));
        int lineHeight = desiredStatH + 2;
        int startY = lineY + 4 + desiredStatH + 8; // 在分类和献祭下方
        int markIndex = 0;
        
        for (size_t i = 0; i < card.marks.size(); i++) {
            // 跳过"消耗骨头"印记，不显示在牌面上
            if (card.marks[i] == "消耗骨头") {
                continue;
            }
            
            SDL_Surface* s = TTF_RenderUTF8_Blended(cardStatFont_, card.marks[i].c_str(), markCol);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                float scale = (float)desiredStatH / (float)s->h;
                int scaledW = (int)(s->w * scale);
                int mx = cardRect.x + (cardRect.w - scaledW) / 2;
                int my = startY + markIndex * lineHeight;
                SDL_Rect dst{ mx, my, scaledW, desiredStatH };
                SDL_RenderCopy(r, t, nullptr, &dst);
                SDL_DestroyTexture(t);
                SDL_FreeSurface(s);
                markIndex++;
            }
        }
    }
}
