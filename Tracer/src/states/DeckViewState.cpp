#include "DeckViewState.h"
#include "MapExploreState.h"
#include "../core/App.h"
#include "../ui/CardRenderer.h"
#include "../core/TutorialTexts.h"
#include <SDL.h>
#include <SDL_ttf.h>

DeckViewState::DeckViewState() = default;

DeckViewState::~DeckViewState() {
    if (titleTex_) SDL_DestroyTexture(titleTex_);
    if (titleFont_) TTF_CloseFont(titleFont_);
    if (smallFont_) TTF_CloseFont(smallFont_);
    if (tutorialButton_) delete tutorialButton_;
}

void DeckViewState::onEnter(App& app) {
    // 获取屏幕尺寸
    int w, h;
    SDL_GetWindowSize(app.getWindow(), &w, &h);
    screenW_ = w;
    screenH_ = h;

    // 加载字体
    titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 48);
    smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 16);
    if (!titleFont_ || !smallFont_) {
        SDL_Log("TTF_OpenFont failed: %s", TTF_GetError());
    }
    else {
        SDL_Color titleCol{ 200, 230, 255, 255 };
        SDL_Surface* ts = TTF_RenderUTF8_Blended(titleFont_, u8"牌库查看", titleCol);
        if (ts) {
            titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), ts);
            titleW_ = ts->w;
            titleH_ = ts->h;
            SDL_FreeSurface(ts);
        }
    }

    // 获取牌库数据
    auto& store = DeckStore::instance();
    libraryCards_ = store.library();
    
    // 布局卡牌
    layoutCards();
    
    // 创建教程按钮
    tutorialButton_ = new Button();
    tutorialButton_->setText(u8"?");
    tutorialButton_->setRect({ screenW_ - 60, 20, 40, 40 });
    tutorialButton_->setOnClick([this]() { startTutorial(); });
}

void DeckViewState::onExit(App& app) {}

void DeckViewState::handleEvent(App& app, const SDL_Event& e) {
    // 教程系统处理
    if (CardRenderer::isTutorialActive()) {
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            CardRenderer::handleTutorialClick();
        }
        return;
    }
    
    if (tutorialButton_) tutorialButton_->handleEvent(e);
    
    // 处理S键返回地图
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_s) {
        pendingGoMapExplore_ = true;
        return;
    }
    
    if (e.type == SDL_MOUSEMOTION) {
        int mx = e.motion.x, my = e.motion.y;
        
        // 处理印记悬停
        for (size_t i = 0; i < libraryCards_.size() && i < cardRects_.size(); ++i) {
            SDL_Rect renderRect = cardRects_[i];
            renderRect.y -= scrollY_; // 应用滚动偏移
            
            // 只检查可见的卡牌
            if (renderRect.y + renderRect.h >= 0 && renderRect.y <= screenH_) {
                if (mx >= renderRect.x && mx <= renderRect.x + renderRect.w && 
                    my >= renderRect.y && my <= renderRect.y + renderRect.h) {
                    CardRenderer::handleMarkHover(libraryCards_[i], renderRect, mx, my, smallFont_);
                    return;
                }
            }
        }
        
        // 如果没有悬停在任何印记上，隐藏提示
        App::hideMarkTooltip();
    }
    // 处理印记右键点击
    else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
        int mx = e.button.x, my = e.button.y;
        
        for (size_t i = 0; i < libraryCards_.size() && i < cardRects_.size(); ++i) {
            SDL_Rect renderRect = cardRects_[i];
            renderRect.y -= scrollY_; // 应用滚动偏移
            
            // 只检查可见的卡牌
            if (renderRect.y + renderRect.h >= 0 && renderRect.y <= screenH_) {
                if (mx >= renderRect.x && mx <= renderRect.x + renderRect.w && 
                    my >= renderRect.y && my <= renderRect.y + renderRect.h) {
                    CardRenderer::handleMarkClick(libraryCards_[i], renderRect, mx, my, smallFont_);
                    if (App::isMarkTooltipVisible()) {
                        return;
                    }
                }
            }
        }
    }
    // 处理滚轮滚动
    else if (e.type == SDL_MOUSEWHEEL) {
        int oldScrollY = scrollY_;
        scrollY_ -= e.wheel.y * 30; // 滚动步长
        if (scrollY_ < 0) scrollY_ = 0;
        if (scrollY_ > maxScrollY_) scrollY_ = maxScrollY_;
        
        SDL_Log("DeckView scroll: wheel.y=%d, oldScrollY=%d, newScrollY=%d, maxScrollY=%d", 
                e.wheel.y, oldScrollY, scrollY_, maxScrollY_);
    }
}

void DeckViewState::update(App& app, float dt) {
    // 更新教程系统
    CardRenderer::updateTutorial(dt);
    
    if (pendingGoMapExplore_) {
        pendingGoMapExplore_ = false;
        app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState())));
    }
}

void DeckViewState::render(App& app) {
    SDL_Renderer* r = app.getRenderer();
    SDL_SetRenderDrawColor(r, 18, 22, 32, 255);
    SDL_RenderClear(r);

    // 渲染标题（已删除）
    // if (titleTex_) {
    //     SDL_Rect titleRect{ (screenW_ - titleW_) / 2, 20, titleW_, titleH_ };
    //     SDL_RenderCopy(r, titleTex_, nullptr, &titleRect);
    // }

    // 渲染提示信息
    if (smallFont_) {
        SDL_Color hintCol{ 150, 150, 150, 255 };
        SDL_Surface* hintSurf = TTF_RenderUTF8_Blended(smallFont_, u8"按S键返回地图", hintCol);
        if (hintSurf) {
            SDL_Texture* hintTex = SDL_CreateTextureFromSurface(r, hintSurf);
            SDL_Rect hintRect{ 20, screenH_ - 30, hintSurf->w, hintSurf->h };
            SDL_RenderCopy(r, hintTex, nullptr, &hintRect);
            SDL_DestroyTexture(hintTex);
            SDL_FreeSurface(hintSurf);
        }
    }

    // 渲染卡牌
    for (size_t i = 0; i < libraryCards_.size() && i < cardRects_.size(); ++i) {
        const Card& card = libraryCards_[i];
        SDL_Rect renderRect = cardRects_[i];
        renderRect.y -= scrollY_; // 应用滚动偏移
        
        // 只渲染可见的卡牌
        if (renderRect.y + renderRect.h >= 0 && renderRect.y <= screenH_) {
            CardRenderer::renderCard(app, card, renderRect, smallFont_, smallFont_, false);
        }
    }
    
    // 渲染全局印记提示
    CardRenderer::renderGlobalMarkTooltip(app, smallFont_);
    
    // 渲染教程按钮
    if (tutorialButton_) {
        tutorialButton_->render(r);
    }
    
    // 渲染教程系统
    CardRenderer::renderTutorial(r, smallFont_, screenW_, screenH_);
}

void DeckViewState::layoutCards() {
    cardRects_.clear();
    
    if (libraryCards_.empty()) return;
    
    // 计算布局参数
    int marginX = 20;
    int marginY = 100; // 为标题留出空间
    int availableWidth = screenW_ - marginX * 2;
    int availableHeight = screenH_ - marginY - 50; // 为提示信息留出空间
    
    // 计算每行卡牌数
    cardsPerRow_ = availableWidth / (cardWidth_ + cardSpacing_);
    if (cardsPerRow_ < 1) cardsPerRow_ = 1;
    
    // 计算总行数
    int totalRows = (static_cast<int>(libraryCards_.size()) + cardsPerRow_ - 1) / cardsPerRow_;
    
    // 计算最大滚动距离
    int totalHeight = totalRows * (cardHeight_ + cardSpacing_) + marginY;
    maxScrollY_ = std::max(0, totalHeight - availableHeight);
    
    // 布局卡牌
    for (size_t i = 0; i < libraryCards_.size(); ++i) {
        int row = static_cast<int>(i) / cardsPerRow_;
        int col = static_cast<int>(i) % cardsPerRow_;
        
        int x = marginX + col * (cardWidth_ + cardSpacing_);
        int y = marginY + row * (cardHeight_ + cardSpacing_);
        
        SDL_Rect rect{ x, y, cardWidth_, cardHeight_ };
        cardRects_.push_back(rect);
    }
}

void DeckViewState::updateScrollBounds() {
    if (libraryCards_.empty()) {
        maxScrollY_ = 0;
        return;
    }
    
    int totalRows = (static_cast<int>(libraryCards_.size()) + cardsPerRow_ - 1) / cardsPerRow_;
    int totalHeight = totalRows * (cardHeight_ + cardSpacing_) + 100; // 100为标题空间
    int availableHeight = screenH_ - 150; // 150为标题和提示信息空间
    
    maxScrollY_ = std::max(0, totalHeight - availableHeight);
    
    if (scrollY_ > maxScrollY_) scrollY_ = maxScrollY_;
}

void DeckViewState::startTutorial() {
	// 使用统一的教程文本
	std::vector<std::string> tutorialTexts = TutorialTexts::getDeckViewTutorial();
	
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
