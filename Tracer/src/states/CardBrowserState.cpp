#include "CardBrowserState.h"
#include "../core/App.h"
#include "TestState.h"
#include "../core/Cards.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include "../ui/CardRenderer.h"
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
    // 设置窗口尺寸（适中尺寸）
    screenW_ = 1600;
    screenH_ = 1000;
    SDL_SetWindowSize(app.getWindow(), screenW_, screenH_);
    
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
    // 处理印记提示
    if (event.type == SDL_MOUSEMOTION) {
        int mouseX = event.motion.x;
        int mouseY = event.motion.y;
        
        // 检查卡牌中的印记悬停
        for (int i = 0; i < (int)filteredCards_.size(); ++i) {
            const SDL_Rect& cardRect = cardRects_[i];
            if (mouseX >= cardRect.x && mouseX <= cardRect.x + cardRect.w &&
                mouseY >= cardRect.y && mouseY <= cardRect.y + cardRect.h) {
                CardRenderer::handleMarkHover(filteredCards_[i], cardRect, mouseX, mouseY, cardStatFont_);
                return;
            }
        }
        
        // 如果没有悬停在任何印记上，隐藏提示
        App::hideMarkTooltip();
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
        // 右键点击检测印记
        int mouseX = event.button.x;
        int mouseY = event.button.y;
        
        // 检查卡牌中的印记
        for (size_t i = 0; i < cardRects_.size() && i < filteredCards_.size(); ++i) {
            if (mouseX >= cardRects_[i].x && mouseX <= cardRects_[i].x + cardRects_[i].w &&
                mouseY >= cardRects_[i].y && mouseY <= cardRects_[i].y + cardRects_[i].h) {
                CardRenderer::handleMarkClick(filteredCards_[i], cardRects_[i], mouseX, mouseY, cardStatFont_);
                if (App::isMarkTooltipVisible()) {
                    return;
                }
            }
        }
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN) {
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
            SDL_Rect rect{ cardRects_[cardIndex].x, cardRects_[cardIndex].y, cardRects_[cardIndex].w, cardRects_[cardIndex].h };
            CardRenderer::renderCard(app, filteredCards_[i], rect, cardNameFont_, cardStatFont_, selected);
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
    
    // 渲染印记提示
    CardRenderer::renderGlobalMarkTooltip(app, cardStatFont_);
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

// 旧的内部渲染函数已由公共 CardRenderer 替代
