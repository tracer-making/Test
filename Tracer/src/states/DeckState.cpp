#include "DeckState.h"
#include "TestState.h"
#include "../core/App.h"
#include "../ui/CardRenderer.h"
#include <cmath>

DeckState::DeckState() = default;

DeckState::~DeckState() {
	if (titleTex_) SDL_DestroyTexture(titleTex_);
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (smallFont_) TTF_CloseFont(smallFont_);
	if (nameFont_) TTF_CloseFont(nameFont_);
	if (statFont_) TTF_CloseFont(statFont_);
	delete backButton_;
}

void DeckState::onEnter(App& app) {
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
		SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, u8"牌库", col);
		if (s) { titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), s); SDL_FreeSurface(s);} 
	}

	backButton_ = new Button();
	if (backButton_) {
		backButton_->setRect({20,20,120,36});
		backButton_->setText(u8"返回测试");
		if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer());
		App* appPtr = &app;
		backButton_->setOnClick([appPtr]() {
			appPtr->setState(std::unique_ptr<State>(static_cast<State*>(new TestState())));
		});
	}

	buildDemoDeck();
	layoutSlider();
	layoutGrid();

	// 生成水墨背景点阵
	bgDots_.clear();
	srand(4242);
	for (int i=0;i<400;++i) {
		int x = rand()%screenW_;
		int y = rand()%screenH_;
		bgDots_.push_back({x,y});
	}
}

void DeckState::onExit(App& app) {}

void DeckState::handleEvent(App& app, const SDL_Event& e) {
	if (backButton_) backButton_->handleEvent(e);

	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mx=e.button.x, my=e.button.y;
		
		if (mx>=sliderTrack_.x && mx<=sliderTrack_.x+sliderTrack_.w && my>=sliderTrack_.y-6 && my<=sliderTrack_.y+sliderTrack_.h+6) {
			sliderDragging_ = true;
			updateSliderFromMouse(mx);
		}
	}
	else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
		sliderDragging_ = false;
	}
	else if (e.type == SDL_MOUSEMOTION && sliderDragging_) {
		updateSliderFromMouse(e.motion.x);
	}
}

void DeckState::update(App& app, float dt) {}

void DeckState::render(App& app) {
	SDL_Renderer* r = app.getRenderer();
	SDL_SetRenderDrawColor(r, 20, 24, 34, 255);
	SDL_RenderClear(r);

	// 水墨背景点
	SDL_SetRenderDrawColor(r, 80, 70, 60, 60);
	for (const auto& p : bgDots_) {
		SDL_RenderDrawPoint(r, p.first, p.second);
	}

	if (titleTex_) {
		int tw,th; SDL_QueryTexture(titleTex_,nullptr,nullptr,&tw,&th);
		SDL_Rect dst{ (screenW_-tw)/2, 60, tw, th };
		SDL_RenderCopy(r, titleTex_, nullptr, &dst);
	}

	if (backButton_) backButton_->render(r);

	// 渲染滑动条
	SDL_SetRenderDrawColor(r, 100,130,180,180);
	SDL_RenderFillRect(r, &sliderTrack_);
	SDL_SetRenderDrawColor(r, 180,210,255,230);
	SDL_RenderFillRect(r, &sliderThumb_);
	// 数字显示
	if (smallFont_) {
		std::string label = std::string("数量:") + std::to_string(numCards_);
		SDL_Color col{200,220,250,255};
		SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, label.c_str(), col);
		if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r, s); SDL_Rect dst{ sliderTrack_.x, sliderTrack_.y- s->h - 6, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&dst); SDL_DestroyTexture(t); SDL_FreeSurface(s);} 
	}

	// 绘制卡牌网格（统一水墨风格）
	for (const auto& c : cards_) {
		// 将CardView转换为Card结构以使用CardRenderer
		Card card;
		card.name = c.name;
		card.attack = c.attack;
		card.health = c.health;
		card.category = "其他"; // 默认分类
		card.sacrificeCost = 0; // 默认无献祭消耗
		card.marks = c.marks;
		
		CardRenderer::renderCard(app, card, c.rect, nameFont_, statFont_, false);
	}
}

void DeckState::buildDemoDeck() {
	cards_.clear();
	int n = SDL_clamp(numCards_, minCards_, maxCards_);
	for (int i=0;i<n;++i) {
		CardView cv; cv.name = "卡牌" + std::to_string(i+1); cv.attack = 1 + (i%5); cv.health = 3 + (i%4);
		cards_.push_back(cv);
	}
}


void DeckState::layoutGrid() {
	int n = (int)cards_.size();
	if (n <= 0) return;

	// 屏幕内边距与可用区域
	int marginX = 40;
	int topY = 130; // 标题下方
	int bottomMargin = 30;
	int availableW = SDL_max(200, screenW_ - marginX * 2);
	int availableH = SDL_max(160, screenH_ - topY - bottomMargin);

	// 目标比例（更小的卡，约 2:3）
	const float aspect = 2.0f / 3.0f; // w:h
	int bestCols = 1;
	int bestCardW = 0;
	int bestCardH = 0;
	int bestRows = n;
	int gap = 12; // 小间距

	int maxTryCols = SDL_min(n, 10); // 不必超过10列
	for (int cols = maxTryCols; cols >= 1; --cols) {
		int rows = (n + cols - 1) / cols;
		// 根据宽度估算卡片宽度
		int cardW = (availableW - (cols - 1) * gap) / cols;
		if (cardW <= 20) continue; // 太小直接跳过
		int cardH = (int)(cardW / aspect);
		// 检查高度是否可容纳
		int totalH = rows * cardH + (rows - 1) * gap;
		if (totalH <= availableH) {
			// 选择卡片更大的方案
			if (cardW > bestCardW) {
				bestCardW = cardW;
				bestCardH = cardH;
				bestCols = cols;
				bestRows = rows;
			}
		}
	}

	// 如果仍未找到合适布局，强制缩放以高度优先适配
	if (bestCardW == 0) {
		int cols = SDL_min(n, SDL_max(1, availableW / 80)); // 预估列宽80
		cols = SDL_max(1, cols);
		int rows = (n + cols - 1) / cols;
		int cardW = (availableW - (cols - 1) * gap) / cols;
		int cardH = (int)(cardW / aspect);
		int totalH = rows * cardH + (rows - 1) * gap;
		if (totalH > availableH) {
			float scale = (float)availableH / (float)totalH;
			cardW = SDL_max(24, (int)(cardW * scale));
			cardH = SDL_max(36, (int)(cardH * scale));
		}
		bestCols = cols; bestRows = rows; bestCardW = cardW; bestCardH = cardH;
	}

	int totalW = bestCols * bestCardW + (bestCols - 1) * gap;
	int startX = (screenW_ - totalW) / 2;
	int startY = topY + (availableH - (bestRows * bestCardH + (bestRows - 1) * gap)) / 2;
	startY = SDL_max(topY, startY);

	for (int i = 0; i < n; ++i) {
		int r = i / bestCols;
		int c = i % bestCols;
		cards_[i].rect = { startX + c * (bestCardW + gap), startY + r * (bestCardH + gap), bestCardW, bestCardH };
	}
}

void DeckState::layoutSlider() {
	int trackW = SDL_max(200, screenW_ - 2*220);
	int trackH = 6;
	int trackX = (screenW_ - trackW) / 2;
	int trackY = 110;
	sliderTrack_ = { trackX, trackY, trackW, trackH };
	// 拇指大小
	int thumbW = 14, thumbH = 24;
	// 位置根据 numCards_ 映射
	float t = float(SDL_clamp(numCards_, minCards_, maxCards_) - minCards_) / float(maxCards_ - minCards_);
	int thumbX = trackX + int(t * trackW) - thumbW/2;
	int thumbY = trackY + trackH/2 - thumbH/2;
	sliderThumb_ = { thumbX, thumbY, thumbW, thumbH };
}

void DeckState::updateSliderFromMouse(int mx) {
	int clampedX = SDL_clamp(mx, sliderTrack_.x, sliderTrack_.x + sliderTrack_.w);
	float t = float(clampedX - sliderTrack_.x) / float(sliderTrack_.w);
	int value = minCards_ + int(t * float(maxCards_ - minCards_) + 0.5f);
	value = SDL_clamp(value, minCards_, maxCards_);
	if (value != numCards_) {
		numCards_ = value;
		buildDemoDeck();
		layoutGrid();
	}
	// 更新thumb位置
	int thumbW = sliderThumb_.w;
	int thumbH = sliderThumb_.h;
	int thumbX = sliderTrack_.x + int(float(numCards_-minCards_) / float(maxCards_-minCards_) * sliderTrack_.w) - thumbW/2;
	int thumbY = sliderTrack_.y + sliderTrack_.h/2 - thumbH/2;
	sliderThumb_ = { thumbX, thumbY, thumbW, thumbH };
}


