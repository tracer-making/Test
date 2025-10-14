#include "HeritageState.h"
#include "TestState.h"
#include "MapExploreState.h"
#include "../core/App.h"
#include "../ui/CardRenderer.h"

HeritageState::HeritageState() = default;
HeritageState::~HeritageState() {
	if (titleTex_) SDL_DestroyTexture(titleTex_);
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (smallFont_) TTF_CloseFont(smallFont_);
	if (cardNameFont_) TTF_CloseFont(cardNameFont_);
	if (cardStatFont_) TTF_CloseFont(cardStatFont_);
	delete backButton_;
	delete confirmButton_;
	delete plusButton_;
}

void HeritageState::onEnter(App& app) {
	int w,h; SDL_GetWindowSize(app.getWindow(), &w, &h); screenW_ = w; screenH_ = h;
	titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 56);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 18);
	cardNameFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 16);  // 卡牌名称字体
	cardStatFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 14);  // 卡牌属性字体
	if (titleFont_) {
		SDL_Color col{200,230,255,255};
		SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, u8"文脉传承", col);
		if (s) { titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), s); SDL_FreeSurface(s);} 
	}

	backButton_ = new Button();
	if (backButton_) {
		backButton_->setRect({20,20,120,36});
		backButton_->setText(u8"返回地图");
		if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer());
		backButton_->setOnClick([this]() { pendingGoMapExplore_ = true; });
	}

	// 创建中间的+按钮
	plusButton_ = new Button();
	if (plusButton_) {
		plusButton_->setRect({screenW_/2 - 25, screenH_/2 - 25, 50, 50});
		plusButton_->setText(u8"+");
		if (titleFont_) plusButton_->setFont(titleFont_, app.getRenderer());
		plusButton_->setOnClick([this]() { performInheritance(); });
	}

	// 使用玩家的实际牌堆
	auto& store = DeckStore::instance();
	
	// 如果牌库为空，重新初始化玩家牌堆
	if (store.library().empty()) {
		store.initializePlayerDeck();
	}
	
	// 如果手牌为空，从牌库抽取一些卡牌到手牌
	if (store.hand().empty() && !store.library().empty()) {
		// 抽取一些卡牌到手牌用于文脉传承
		int drawCount = std::min(6, (int)store.library().size());
		store.drawToHand(drawCount);
	}

	layoutCardSlots();
}

void HeritageState::onExit(App& app) {}

void HeritageState::handleEvent(App& app, const SDL_Event& e) {
	if (backButton_) backButton_->handleEvent(e);
	
	// 只有两个牌位都有卡牌时才处理+按钮事件
	if (hasSourceCard_ && hasTargetCard_ && plusButton_) {
		plusButton_->handleEvent(e);
	}

	if (e.type == SDL_MOUSEMOTION) {
		// 处理鼠标移动，检测悬停
		int mx = e.motion.x, my = e.motion.y;
		
		// 检测左边牌位悬停
		SDL_Rect leftSlot = {screenW_/2 - 300, screenH_/2 - 150, 200, 300};
		if (mx >= leftSlot.x && mx <= leftSlot.x + leftSlot.w && 
			my >= leftSlot.y && my <= leftSlot.y + leftSlot.h) {
			leftSlotHover_ = 1.0f;  // 设置悬停状态
		}
		
		// 检测右边牌位悬停
		SDL_Rect rightSlot = {screenW_/2 + 100, screenH_/2 - 150, 200, 300};
		if (mx >= rightSlot.x && mx <= rightSlot.x + rightSlot.w && 
			my >= rightSlot.y && my <= rightSlot.y + rightSlot.h) {
			rightSlotHover_ = 1.0f;  // 设置悬停状态
		}
		
		// 检测+按钮悬停（只有两个牌位都有卡牌时才检测）
		if (hasSourceCard_ && hasTargetCard_ && plusButton_) {
			SDL_Rect plusRect = plusButton_->getRect();
			if (mx >= plusRect.x && mx <= plusRect.x + plusRect.w && 
				my >= plusRect.y && my <= plusRect.y + plusRect.h) {
				plusButtonHover_ = 1.0f;  // 设置悬停状态
			}
		}
		
		// 检测手牌区卡牌悬停
		if (showingHandCards_) {
			for (size_t i = 0; i < handCardRects_.size(); ++i) {
				const SDL_Rect& rc = handCardRects_[i];
				if (mx >= rc.x && mx <= rc.x + rc.w && 
					my >= rc.y && my <= rc.y + rc.h) {
					handCardHovers_[i] = 1.0f;  // 设置悬停状态
					break;  // 只处理一个卡牌的悬停
				}
			}
		}
	}

	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mx=e.button.x, my=e.button.y;
		
		if (showingHandCards_) {
			// 在手牌区中选择卡牌
			for (size_t i=0; i<handCardRects_.size(); ++i) {
				const SDL_Rect& rc = handCardRects_[i];
				if (mx>=rc.x && mx<=rc.x+rc.w && my>=rc.y && my<=rc.y+rc.h) {
					selectedHandCardIndex_ = (int)i;
					break;
				}
			}
			
			// 检查是否点击了卡牌
			if (selectedHandCardIndex_ >= 0 && selectedHandCardIndex_ < (int)availableCards_.size()) {
				// 确认选择
				if (selectingSource_) {
					selectedSourceCard_ = availableCards_[selectedHandCardIndex_];
					hasSourceCard_ = true;
				} else {
					selectedTargetCard_ = availableCards_[selectedHandCardIndex_];
					hasTargetCard_ = true;
				}
				hideHandCards();
			}
		} else {
			// 在主界面中点击牌位 - 两个牌位都可以点击，支持重新选择
			// 左边牌位（被献祭的卡）
			SDL_Rect leftSlot = {screenW_/2 - 300, screenH_/2 - 150, 200, 300};
			if (mx>=leftSlot.x && mx<=leftSlot.x+leftSlot.w && my>=leftSlot.y && my<=leftSlot.y+leftSlot.h) {
				// 如果当前正在显示手牌区且是选择源卡，则隐藏手牌区
				if (showingHandCards_ && selectingSource_) {
					hideHandCards();
				} else {
					// 否则显示选择被献祭的卡
					showHandCards(true);
				}
			}
			
			// 右边牌位（接受传承的卡）
			SDL_Rect rightSlot = {screenW_/2 + 100, screenH_/2 - 150, 200, 300};
			if (mx>=rightSlot.x && mx<=rightSlot.x+rightSlot.w && my>=rightSlot.y && my<=rightSlot.y+rightSlot.h) {
				// 如果当前正在显示手牌区且是选择目标卡，则隐藏手牌区
				if (showingHandCards_ && !selectingSource_) {
					hideHandCards();
				} else {
					// 否则显示选择接受传承的卡
					showHandCards(false);
				}
			}
		}
		
		// 点击左右牌位：进入/切换选择模式
		SDL_Rect leftSlot = {screenW_/2 - 300, screenH_/2 - 150, 200, 300};
		SDL_Rect rightSlot = {screenW_/2 + 100, screenH_/2 - 150, 200, 300};
		if (mx >= leftSlot.x && mx <= leftSlot.x + leftSlot.w && my >= leftSlot.y && my <= leftSlot.y + leftSlot.h) {
			// 切换为选择被献祭卡
			showHandCards(true);
			return;
		}
		if (mx >= rightSlot.x && mx <= rightSlot.x + rightSlot.w && my >= rightSlot.y && my <= rightSlot.y + rightSlot.h) {
			// 切换为选择接受传承卡
			showHandCards(false);
			return;
		}
	}
}

void HeritageState::update(App& app, float dt) {
	if (pendingBackToTest_) {
		pendingBackToTest_ = false;
		app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState())));
		return;
	}
	if (pendingGoMapExplore_) {
		pendingGoMapExplore_ = false;
		app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState())));
		return;
	}
	
	// 更新悬停动画
	const float hoverSpeed = 3.0f;  // 悬停响应速度（用于高亮/缩放）
	leftSlotHover_ = std::max(0.0f, leftSlotHover_ - hoverSpeed * dt);
	rightSlotHover_ = std::max(0.0f, rightSlotHover_ - hoverSpeed * dt);
	plusButtonHover_ = std::max(0.0f, plusButtonHover_ - hoverSpeed * dt);
	// 持续漂浮时间推进（正弦波）
	hoverTime_ += dt;
	
	// 更新手牌区卡牌悬停动画
	for (size_t i = 0; i < handCardHovers_.size(); ++i) {
		handCardHovers_[i] = std::max(0.0f, handCardHovers_[i] - hoverSpeed * dt);
	}
	
	// 更新传承动画
	if (isAnimating_) {
		animationTime_ += dt;
		
		// 动画进行到一半时摧毁源卡
		if (animationTime_ >= animationDuration_ * 0.3f && !sourceCardDestroyed_) {
			sourceCardDestroyed_ = true;
			// 这里可以添加摧毁源卡的实际逻辑
		}
		
		// 动画进行到一半时强化目标卡
		if (animationTime_ >= animationDuration_ * 0.6f && !targetCardEnhanced_) {
			targetCardEnhanced_ = true;
			// 这里可以添加强化目标卡的实际逻辑
		}
		
		// 动画结束后返回地图
		if (animationTime_ >= animationDuration_) {
			isAnimating_ = false;
			// 重置选择状态
			hasSourceCard_ = false;
			hasTargetCard_ = false;
			selectedSourceCard_ = Card();
			selectedTargetCard_ = Card();
			pendingGoMapExplore_ = true;
		}
	}
}

void HeritageState::render(App& app) {
	SDL_Renderer* r = app.getRenderer();
	SDL_SetRenderDrawColor(r, 18, 22, 32, 255);
	SDL_RenderClear(r);

	if (titleTex_) {
		int tw,th; SDL_QueryTexture(titleTex_,nullptr,nullptr,&tw,&th);
		SDL_Rect dst{ (screenW_-tw)/2, 60, tw, th };
		SDL_RenderCopy(r, titleTex_, nullptr, &dst);
	}

	if (backButton_) backButton_->render(r);
	
	// 只有两个牌位都有卡牌时才显示+按钮
	if (hasSourceCard_ && hasTargetCard_ && plusButton_) {
		// 应用悬停动画到+按钮 + 持续漂浮
		SDL_Rect originalRect = plusButton_->getRect();
		SDL_Rect animatedRect = originalRect;
		// 悬停响应（仅在悬停时上移/放大）
		animatedRect.y -= (int)(plusButtonHover_ * 5);
		animatedRect.w += (int)(plusButtonHover_ * 10); // 放大
		animatedRect.h += (int)(plusButtonHover_ * 10);
		animatedRect.x -= (int)(plusButtonHover_ * 5);  // 居中调整
		
		// 临时设置动画后的位置
		plusButton_->setRect(animatedRect);
		plusButton_->render(r);
		// 恢复原始位置
		plusButton_->setRect(originalRect);
	}

	// 渲染主界面
	renderMainInterface(app);
	
	// 如果显示手牌区，渲染手牌区
	if (showingHandCards_) {
		renderHandCardArea(app);
	}
}

void HeritageState::renderMainInterface(App& app) {
	SDL_Renderer* r = app.getRenderer();
	// 说明文字
	if (smallFont_) {
		SDL_Color textColor{200,230,255,255};
		SDL_Surface* textSurface = TTF_RenderUTF8_Blended(smallFont_, u8"点击牌位选择卡牌进行文脉传承", textColor);
		if (textSurface) {
			SDL_Texture* textTexture = SDL_CreateTextureFromSurface(r, textSurface);
			if (textTexture) {
				SDL_Rect textRect = {20, screenH_ - 60, textSurface->w, textSurface->h};
				SDL_RenderCopy(r, textTexture, nullptr, &textRect);
				SDL_DestroyTexture(textTexture);
			}
			SDL_FreeSurface(textSurface);
		}
	}

    // 左边牌位（被献祭的卡）- 更大更分开，仅在悬停时漂浮
    SDL_Rect leftSlot = {screenW_/2 - 300, screenH_/2 - 150, 200, 300};
    // 悬停响应放大/上移 + 悬停持续漂浮（正弦）
    if (leftSlotHover_ > 0.0f) {
        leftSlot.y += (int)(sinf(hoverTime_ * 2.2f) * 6.0f);
    }
    leftSlot.y -= (int)(leftSlotHover_ * 10);
    leftSlot.w += (int)(leftSlotHover_ * 4);
    leftSlot.h += (int)(leftSlotHover_ * 6);
    leftSlot.x -= (int)(leftSlotHover_ * 2);
    
    // 选中高亮：当前在选择被献祭卡时高亮左槽
    if (showingHandCards_ && selectingSource_) {
        SDL_SetRenderDrawColor(r, 120, 120, 160, 220); // 带蓝紫调的高亮
    } else {
        SDL_SetRenderDrawColor(r, 100, 100, 100, 200);
    }
    SDL_RenderFillRect(r, &leftSlot);
	SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
	SDL_RenderDrawRect(r, &leftSlot);
	
	// 左边牌位标签
	if (smallFont_) {
		SDL_Color col{200,200,200,255};
		SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, u8"被献祭的卡", col);
		if (s) {
			SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
			SDL_Rect d{leftSlot.x + (leftSlot.w - s->w)/2, leftSlot.y - 30, s->w, s->h};
			SDL_RenderCopy(r, t, nullptr, &d);
			SDL_DestroyTexture(t);
			SDL_FreeSurface(s);
		}
	}
	
	// 如果已选择被献祭的卡，显示卡牌信息
	if (hasSourceCard_) {
		// 如果正在播放动画且源卡已被摧毁，不显示源卡
		if (!isAnimating_ || !sourceCardDestroyed_) {
			renderCardInSlot(app, leftSlot, selectedSourceCard_);
		}
	}

    // 右边牌位（接受传承的卡）- 更大更分开，仅在悬停时漂浮
    SDL_Rect rightSlot = {screenW_/2 + 100, screenH_/2 - 150, 200, 300};
    // 悬停响应放大/上移 + 悬停持续漂浮（正弦）
    if (rightSlotHover_ > 0.0f) {
        rightSlot.y += (int)(sinf(hoverTime_ * 2.2f) * 6.0f);
    }
    rightSlot.y -= (int)(rightSlotHover_ * 10);
    rightSlot.w += (int)(rightSlotHover_ * 4);
    rightSlot.h += (int)(rightSlotHover_ * 6);
    rightSlot.x -= (int)(rightSlotHover_ * 2);
    
    // 选中高亮：当前在选择接受传承卡时高亮右槽
    if (showingHandCards_ && !selectingSource_) {
        SDL_SetRenderDrawColor(r, 160, 120, 120, 220); // 带红调的高亮
    } else {
        SDL_SetRenderDrawColor(r, 100, 100, 100, 200);
    }
    SDL_RenderFillRect(r, &rightSlot);
	SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
	SDL_RenderDrawRect(r, &rightSlot);
	
	// 右边牌位标签
	if (smallFont_) {
		SDL_Color col{200,200,200,255};
		SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, u8"接受传承的卡", col);
		if (s) {
			SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
			SDL_Rect d{rightSlot.x + (rightSlot.w - s->w)/2, rightSlot.y - 30, s->w, s->h};
			SDL_RenderCopy(r, t, nullptr, &d);
			SDL_DestroyTexture(t);
			SDL_FreeSurface(s);
		}
	}
	
	// 如果已选择接受传承的卡，显示卡牌信息
	if (hasTargetCard_) {
		// 如果正在播放动画且目标卡已被强化，添加强化效果
		if (isAnimating_ && targetCardEnhanced_) {
			// 添加强化光环效果
			SDL_SetRenderDrawColor(r, 255, 215, 0, 100); // 金色半透明
			SDL_Rect glowRect = {rightSlot.x - 10, rightSlot.y - 10, rightSlot.w + 20, rightSlot.h + 20};
			SDL_RenderFillRect(r, &glowRect);
		}
		renderCardInSlot(app, rightSlot, selectedTargetCard_);
	}

	// +按钮就是确认按钮，不需要额外的确认按钮

	// 提示/信息
	if (smallFont_) {
		SDL_Color col{220,230,245,255}; 
		std::string tip;
		
		if (isAnimating_) {
			// 动画期间的提示
			if (sourceCardDestroyed_ && targetCardEnhanced_) {
				tip = u8"传承完成！即将返回地图...";
			} else if (sourceCardDestroyed_) {
				tip = u8"源卡已摧毁，正在强化目标卡...";
			} else {
				tip = u8"正在摧毁源卡...";
			}
		} else if (hasSourceCard_ && hasTargetCard_) {
			tip = message_.empty()? u8"点击中间的+按钮完成传承，或点击牌位重新选择" : message_;
		} else if (hasSourceCard_) {
			tip = message_.empty()? u8"点击右边牌位选择接受传承的卡牌，或点击左边牌位重新选择" : message_;
		} else if (hasTargetCard_) {
			tip = message_.empty()? u8"点击左边牌位选择被献祭的卡牌，或点击右边牌位重新选择" : message_;
		} else if (showingHandCards_) {
			tip = message_.empty()? (selectingSource_ ? u8"选择被献祭的卡牌，或点击其他牌位切换" : u8"选择接受传承的卡牌，或点击其他牌位切换") : message_;
		} else {
			tip = message_.empty()? u8"点击任意牌位选择卡牌，下方会显示可选择的卡牌" : message_;
		}
		
		SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, tip.c_str(), col, screenW_-40); 
		if (s) { 
			SDL_Texture* t = SDL_CreateTextureFromSurface(r, s); 
			SDL_Rect dst{20, screenH_ - 40, s->w, s->h}; 
			SDL_RenderCopy(r,t,nullptr,&dst); 
			SDL_DestroyTexture(t); 
			SDL_FreeSurface(s);
		} 
	}
}

void HeritageState::renderHandCardArea(App& app) {
	SDL_Renderer* r = app.getRenderer();
	
	// 手牌区背景
	SDL_SetRenderDrawColor(r, 30, 35, 45, 200);
	SDL_Rect handArea = {0, screenH_ - 200, screenW_, 200};
	SDL_RenderFillRect(r, &handArea);
	
	// 手牌区标题
	if (smallFont_) {
		SDL_Color textColor{200,230,255,255};
		std::string title = selectingSource_ ? u8"选择被献祭的卡（必须有印记）" : u8"选择接受传承的卡";
		SDL_Surface* textSurface = TTF_RenderUTF8_Blended(smallFont_, title.c_str(), textColor);
		if (textSurface) {
			SDL_Texture* textTexture = SDL_CreateTextureFromSurface(r, textSurface);
			if (textTexture) {
				SDL_Rect textRect = {20, screenH_ - 180, textSurface->w, textSurface->h};
				SDL_RenderCopy(r, textTexture, nullptr, &textRect);
				SDL_DestroyTexture(textTexture);
			}
			SDL_FreeSurface(textSurface);
		}
	}

    // 渲染可选卡牌（仅悬停时上移/放大，参考战斗手牌）
    for (size_t i=0; i<availableCards_.size() && i<handCardRects_.size(); ++i) {
		const auto& card = availableCards_[i];
		SDL_Rect rc = handCardRects_[i];
        // 仅在悬停时做动画
        if (i < handCardHovers_.size()) {
            float hover = handCardHovers_[i];
            rc.y -= (int)(hover * 12);
            rc.w += (int)(hover * 8);
            rc.h += (int)(hover * 12);
            rc.x -= (int)(hover * 4);
        }
		
		// 选中高亮效果
		if (selectedHandCardIndex_ == (int)i) {
			SDL_SetRenderDrawColor(r, 255, 215, 0, 200);
			SDL_Rect highlightRect{ rc.x - 3, rc.y - 3, rc.w + 6, rc.h + 6 };
			SDL_RenderFillRect(r, &highlightRect);
		}
		
		// 使用CardRenderer渲染卡牌，与战斗界面保持一致
		CardRenderer::renderCard(app, card, rc, cardNameFont_, cardStatFont_, selectedHandCardIndex_ == (int)i);
	}
	
	// 确认选择提示
	if (smallFont_ && selectedHandCardIndex_ >= 0) {
		SDL_Color col{255,255,0,255};
		SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, u8"点击卡牌确认选择", col);
		if (s) { 
			SDL_Texture* t = SDL_CreateTextureFromSurface(r, s); 
			SDL_Rect d{screenW_ - s->w - 20, screenH_ - 30, s->w, s->h}; 
			SDL_RenderCopy(r,t,nullptr,&d); 
			SDL_DestroyTexture(t); 
			SDL_FreeSurface(s);
		} 
	}
}

void HeritageState::renderCardInSlot(App& app, const SDL_Rect& slot, const Card& card) {
	// 使用固定的卡牌大小，类似于检索界面
	const int cardWidth = 120;   // 固定宽度
	const int cardHeight = 180;  // 固定高度（2:3比例）
	
	// 计算卡牌在牌位中的居中位置
	int cardX = slot.x + (slot.w - cardWidth) / 2;
	int cardY = slot.y + (slot.h - cardHeight) / 2;
	
	SDL_Rect cardRect = { cardX, cardY, cardWidth, cardHeight };
	
	// 使用CardRenderer渲染卡牌，与战斗界面保持一致
	CardRenderer::renderCard(app, card, cardRect, cardNameFont_, cardStatFont_, false);
}

void HeritageState::layoutCardSlots() {
	// 新的布局方法，不需要卡牌网格
	// 牌位位置已经在renderMainInterface中硬编码
}

void HeritageState::layoutGrid() {
	int n = (int)DeckStore::instance().hand().size(); if (n<=0) { cardRects_.clear(); return; }
	cardRects_.assign(n, SDL_Rect{0,0,0,0});
	int marginX = 40; int topY = 150; int bottom = 40;
	int availW = SDL_max(200, screenW_-marginX*2); int availH = SDL_max(160, screenH_-topY-bottom);
	const float aspect = 2.0f/3.0f; int gap = 12; int bestCols=1, bestW=0, bestH=0, bestRows=n;
	for (int cols = SDL_min(n, 8); cols>=1; --cols) { int rows = (n + cols - 1)/cols; int cw = (availW - (cols-1)*gap)/cols; if (cw<=20) continue; int ch = (int)(cw/aspect); int totalH = rows*ch + (rows-1)*gap; if (totalH<=availH && cw>bestW) { bestW=cw; bestH=ch; bestCols=cols; bestRows=rows; } }
	if (bestW==0) { int cols = SDL_min(n, SDL_max(1, availW/80)); int rows=(n+cols-1)/cols; int cw=(availW-(cols-1)*gap)/cols; int ch=(int)(cw/aspect); float s=(float)availH/(rows*ch+(rows-1)*gap); bestW=SDL_max(24,(int)(cw*s)); bestH=SDL_max(36,(int)(ch*s)); bestCols=cols; bestRows=rows; }
	int totalW = bestCols*bestW + (bestCols-1)*gap; int startX = (screenW_-totalW)/2; int startY = topY + (availH - (bestRows*bestH + (bestRows-1)*gap))/2; startY = SDL_max(topY, startY);
	for (int i=0;i<n;++i) { int r=i/bestCols, c=i%bestCols; cardRects_[i] = { startX + c*(bestW+gap), startY + r*(bestH+gap), bestW, bestH }; }
}


void HeritageState::showHandCards(bool isSource) {
	showingHandCards_ = true;
	selectingSource_ = isSource;
	selectedHandCardIndex_ = -1;
	
	// 获取所有可用的卡牌
	auto& store = DeckStore::instance();
	availableCards_.clear();
	
	// 从手牌和牌库中获取所有卡牌，排除已经放在牌位上的卡牌
	for (const auto& card : store.hand()) {
		// 检查是否已经放在牌位上
		bool isAlreadySelected = false;
		if (hasSourceCard_ && card.id == selectedSourceCard_.id) {
			isAlreadySelected = true;
		}
		if (hasTargetCard_ && card.id == selectedTargetCard_.id) {
			isAlreadySelected = true;
		}
		
		if (!isAlreadySelected) {
			if (isSource) {
				// 选择被献祭的卡：必须有印记
				if (canBeSacrificed(card)) {
					availableCards_.push_back(card);
				}
			} else {
				// 选择接受传承的卡：不能曾经接受过文脉传承
				if (canReceiveInheritance(card)) {
					availableCards_.push_back(card);
				}
			}
		}
	}
	
	for (const auto& card : store.library()) {
		// 检查是否已经放在牌位上
		bool isAlreadySelected = false;
		if (hasSourceCard_ && card.id == selectedSourceCard_.id) {
			isAlreadySelected = true;
		}
		if (hasTargetCard_ && card.id == selectedTargetCard_.id) {
			isAlreadySelected = true;
		}
		
		if (!isAlreadySelected) {
			if (isSource) {
				// 选择被献祭的卡：必须有印记
				if (canBeSacrificed(card)) {
					availableCards_.push_back(card);
				}
			} else {
				// 选择接受传承的卡：不能曾经接受过文脉传承
				if (canReceiveInheritance(card)) {
					availableCards_.push_back(card);
				}
			}
		}
	}
	
	layoutHandCards();
	
	// 初始化手牌区卡牌悬停动画
	handCardHovers_.assign(availableCards_.size(), 0.0f);
}

void HeritageState::hideHandCards() {
	showingHandCards_ = false;
	selectingSource_ = false;
	selectedHandCardIndex_ = -1;
	availableCards_.clear();
	handCardRects_.clear();
	handCardHovers_.clear();
}

void HeritageState::layoutHandCards() {
	int n = (int)availableCards_.size();
	if (n <= 0) { 
		handCardRects_.clear(); 
		return; 
	}
	
	handCardRects_.assign(n, SDL_Rect{0,0,0,0});
	
	// 手牌区布局：水平排列，固定大小
	const int cardWidth = 100;
	const int cardHeight = 150;
	const int gap = 10;
	const int startY = screenH_ - 160;  // 手牌区顶部
	const int marginX = 20;
	
	// 计算起始X位置（居中）
	int totalWidth = n * cardWidth + (n - 1) * gap;
	int startX = (screenW_ - totalWidth) / 2;
	if (startX < marginX) startX = marginX;
	
	// 设置每个卡牌的位置
	for (int i = 0; i < n; ++i) {
		handCardRects_[i] = { 
			startX + i * (cardWidth + gap), 
			startY, 
			cardWidth, 
			cardHeight 
		};
	}
}

bool HeritageState::canBeSacrificed(const Card& card) {
	// 墨锭不能作为被献祭的卡
	if (card.id == "moding") {
		return false;
	}
	// 被献祭的卡必须有印记
	return !card.marks.empty();
}

bool HeritageState::canReceiveInheritance(const Card& card) {
	// 墨锭不能接受传承
	if (card.id == "moding") {
		return false;
	}
	// 接受传承的卡不能曾经接受过文脉传承
	// 这里可以通过检查卡牌是否有特定的印记来判断
	// 暂时假设没有接受过传承的卡都可以接受传承
	// 可以添加一个特殊的印记来标记已经接受过传承的卡
	for (const auto& mark : card.marks) {
		if (mark == u8"文脉传承") {
			return false;  // 已经接受过文脉传承
		}
	}
	return true;
}

void HeritageState::performInheritance() {
	if (!hasSourceCard_ || !hasTargetCard_) { 
		message_ = u8"请先选择被献祭的卡和接受传承的卡"; 
		return; 
	}
	
	if (selectedSourceCard_.id == selectedTargetCard_.id) { 
		message_ = u8"被献祭的卡和接受传承的卡不能相同"; 
		return; 
	}
	auto& store = DeckStore::instance();
	
	// 找到被献祭的卡在牌堆中的位置
	int sourceIndex = -1;
	int targetIndex = -1;
	
	// 在手牌中查找
	for (size_t i = 0; i < store.hand().size(); ++i) {
		if (store.hand()[i].id == selectedSourceCard_.id) {
			sourceIndex = (int)i;
			break;
		}
	}
	
	for (size_t i = 0; i < store.hand().size(); ++i) {
		if (store.hand()[i].id == selectedTargetCard_.id) {
			targetIndex = (int)i;
			break;
		}
	}
	
	// 如果在手牌中没找到，在牌库中查找
	if (sourceIndex == -1) {
		for (size_t i = 0; i < store.library().size(); ++i) {
			if (store.library()[i].id == selectedSourceCard_.id) {
				// 从牌库移动到手牌
				Card sourceCard = store.library()[i];
				store.library().erase(store.library().begin() + i);
				store.hand().push_back(sourceCard);
				sourceIndex = (int)store.hand().size() - 1;
				break;
			}
		}
	}
	
	if (targetIndex == -1) {
		for (size_t i = 0; i < store.library().size(); ++i) {
			if (store.library()[i].id == selectedTargetCard_.id) {
				// 从牌库移动到手牌
				Card targetCard = store.library()[i];
				store.library().erase(store.library().begin() + i);
				store.hand().push_back(targetCard);
				targetIndex = (int)store.hand().size() - 1;
				break;
			}
		}
	}
	
	if (sourceIndex != -1 && targetIndex != -1) {
		// 执行传承：将源卡的印记传给目标卡
		Card& sourceCard = store.hand()[sourceIndex];
		Card& targetCard = store.hand()[targetIndex];
		
		// 将源卡的印记添加到目标卡
		for (const auto& mark : sourceCard.marks) {
			targetCard.marks.push_back(mark);
		}
		
		// 更新selectedTargetCard_以反映新的印记
		selectedTargetCard_ = targetCard;
		
		// 移除源卡
		store.hand().erase(store.hand().begin() + sourceIndex);
		
		message_ = u8"传承成功：源卡已消失，印记并入目标";
		
		// 启动传承动画
		isAnimating_ = true;
		animationTime_ = 0.0f;
		sourceCardDestroyed_ = false;
		targetCardEnhanced_ = false;
	} else {
		message_ = u8"传承失败：找不到指定的卡牌";
	}
	
	// 注意：不在这里重置选择状态，等动画完成后再重置
}


