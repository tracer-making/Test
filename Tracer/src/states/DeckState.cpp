#include "DeckState.h"
#include "TestState.h"
#include "../core/App.h"
#include "../ui/CardRenderer.h"
#include "../core/Deck.h"
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

	buildGlobalDeck();
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
	
	// 处理印记提示
	if (e.type == SDL_MOUSEMOTION) {
		int mouseX = e.motion.x;
		int mouseY = e.motion.y;
		
		// 检查卡牌中的印记悬停
		for (int i = 0; i < (int)cards_.size(); ++i) {
			const SDL_Rect& cardRect = cards_[i].rect;
			if (mouseX >= cardRect.x && mouseX <= cardRect.x + cardRect.w &&
				mouseY >= cardRect.y && mouseY <= cardRect.y + cardRect.h) {
				// 创建临时的Card对象用于印记悬停检测
				Card tempCard;
				tempCard.name = cards_[i].name;
				tempCard.attack = cards_[i].attack;
				tempCard.health = cards_[i].health;
				tempCard.marks = cards_[i].marks;
				CardRenderer::handleMarkHover(tempCard, cardRect, mouseX, mouseY, statFont_);
				return;
			}
		}
		
		// 如果没有悬停在任何印记上，隐藏提示
		App::hideMarkTooltip();
	}
	else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
		// 右键点击检测印记
		int mouseX = e.button.x;
		int mouseY = e.button.y;
		
		// 检查卡牌网格中的印记
		for (size_t i = 0; i < cards_.size(); ++i) {
			const SDL_Rect& cardRect = cards_[i].rect;
			if (mouseX >= cardRect.x && mouseX <= cardRect.x + cardRect.w &&
				mouseY >= cardRect.y && mouseY <= cardRect.y + cardRect.h) {
				// 转换 CardView 为 Card
				Card card;
				card.name = cards_[i].name;
				card.attack = cards_[i].attack;
				card.health = cards_[i].health;
				card.marks = cards_[i].marks;
				CardRenderer::handleMarkClick(card, cardRect, mouseX, mouseY, statFont_);
				if (App::isMarkTooltipVisible()) {
					return;
				}
			}
		}
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

	if (backButton_ && App::isGodMode()) backButton_->render(r);

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
	
	// 显示牌库总数信息
	if (smallFont_) {
		auto& globalLibrary = DeckStore::instance().library();
		std::string countText = u8"牌库总数: " + std::to_string(globalLibrary.size());
		SDL_Color col{200,220,250,255};
		SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, countText.c_str(), col);
		if (s) { 
			SDL_Texture* t = SDL_CreateTextureFromSurface(r, s); 
			SDL_Rect dst{ screenW_ - s->w - 20, 20, s->w, s->h }; 
			SDL_RenderCopy(r, t, nullptr, &dst); 
			SDL_DestroyTexture(t); 
			SDL_FreeSurface(s);
		} 
	}
	
	// 渲染印记提示
	CardRenderer::renderGlobalMarkTooltip(app, statFont_);
}

void DeckState::buildGlobalDeck() {
	cards_.clear();
	
	// 从全局牌库获取所有卡牌
	auto& globalLibrary = DeckStore::instance().library();
	
	for (const auto& card : globalLibrary) {
		CardView cv;
		cv.name = card.name;
		cv.attack = card.attack;
		cv.health = card.health;
		cv.marks = card.marks;
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

	// 固定卡牌大小
	const int fixedCardW = 120;  // 固定宽度
	const int fixedCardH = 180; // 固定高度（保持2:3比例）
	int gap = 8; // 小间距
	
	// 计算能容纳多少列
	int maxCols = (availableW + gap) / (fixedCardW + gap);
	maxCols = SDL_max(1, maxCols);
	
	int bestCols = SDL_min(n, maxCols);
	int bestRows = (n + bestCols - 1) / bestCols;
	int bestCardW = fixedCardW;
	int bestCardH = fixedCardH;

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



