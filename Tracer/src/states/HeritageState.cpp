#include "HeritageState.h"
#include "TestState.h"
#include "../core/App.h"

HeritageState::HeritageState() = default;
HeritageState::~HeritageState() {
	if (titleTex_) SDL_DestroyTexture(titleTex_);
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (smallFont_) TTF_CloseFont(smallFont_);
	delete backButton_;
	delete confirmButton_;
}

void HeritageState::onEnter(App& app) {
	int w,h; SDL_GetWindowSize(app.getWindow(), &w, &h); screenW_ = w; screenH_ = h;
	titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 56);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 18);
	if (titleFont_) {
		SDL_Color col{200,230,255,255};
		SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, u8"文脉传承", col);
		if (s) { titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), s); SDL_FreeSurface(s);} 
	}

	backButton_ = new Button();
	if (backButton_) {
		backButton_->setRect({20,20,120,36});
		backButton_->setText(u8"返回测试");
		if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer());
		backButton_->setOnClick([this]() { pendingBackToTest_ = true; });
	}

	confirmButton_ = new Button();
	if (confirmButton_) {
		confirmButton_->setRect({screenW_ - 160, 20, 140, 36});
		confirmButton_->setText(u8"确定传承");
		if (smallFont_) confirmButton_->setFont(smallFont_, app.getRenderer());
		confirmButton_->setOnClick([this]() { performInheritance(); });
	}

	// 若首次进入且无数据，构造示例到库并抽到手牌
	auto& store = DeckStore::instance();
	if (store.hand().empty() && store.library().empty()) {
		for (int i=0;i<12;++i) {
			Card c; c.id = "C"+std::to_string(i+1); c.name = "手牌"+std::to_string(i+1); c.attack = 1 + (i%5); c.health = 3 + (i%4);
			if (i%3==0) c.marks = {"锐意"}; else if (i%3==1) c.marks = {"清风","博学"};
			store.addToLibrary(c);
		}
		store.drawToHand(handCount_);
	}
	else {
		// 依据滑条目标同步到手牌数量（简单示例：若手牌少于目标，从库抽补足；若多于目标，从尾部弃置）
		while ((int)store.hand().size() < handCount_ && !store.library().empty()) store.drawToHand(1);
		while ((int)store.hand().size() > handCount_ && !store.hand().empty()) store.discardFromHand((int)store.hand().size()-1);
	}

	layoutSlider();
	layoutGrid();
}

void HeritageState::onExit(App& app) {}

void HeritageState::handleEvent(App& app, const SDL_Event& e) {
	if (backButton_) backButton_->handleEvent(e);
	if (confirmButton_) confirmButton_->handleEvent(e);

	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mx=e.button.x, my=e.button.y;
		// 滑条
		if (mx>=sliderTrack_.x && mx<=sliderTrack_.x+sliderTrack_.w && my>=sliderTrack_.y-6 && my<=sliderTrack_.y+sliderTrack_.h+6) {
			sliderDragging_ = true;
			updateSliderFromMouse(mx);
		}
		// 选择卡
		for (size_t i=0;i<DeckStore::instance().hand().size();++i) {
			const SDL_Rect& rc = cardRects_[i];
			if (mx>=rc.x && mx<=rc.x+rc.w && my>=rc.y && my<=rc.y+rc.h) {
				if (selectedSource_==-1) selectedSource_ = (int)i;
				else if (selectedTarget_==-1 && (int)i!=selectedSource_) selectedTarget_ = (int)i;
				else { selectedSource_ = (int)i; selectedTarget_ = -1; }
				break;
			}
		}
	}
	else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
		sliderDragging_ = false;
	}
	else if (e.type == SDL_MOUSEMOTION && sliderDragging_) {
		updateSliderFromMouse(e.motion.x);
	}
}

void HeritageState::update(App& app, float dt) {
	if (pendingBackToTest_) {
		pendingBackToTest_ = false;
		app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState())));
		return;
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
	if (confirmButton_) confirmButton_->render(r);

	// 滑动条
	SDL_SetRenderDrawColor(r, 100,130,180,180);
	SDL_RenderFillRect(r, &sliderTrack_);
	SDL_SetRenderDrawColor(r, 180,210,255,230);
	SDL_RenderFillRect(r, &sliderThumb_);

	// 卡牌网格（信息展示与明显选中标记）
	auto& hand = DeckStore::instance().hand();
	for (size_t i=0;i<hand.size();++i) {
		if (i >= cardRects_.size()) break;
		const auto& c = hand[i];
		const SDL_Rect& rc = cardRects_[i];
		// 背板
		SDL_SetRenderDrawColor(r, 235, 230, 220, 230);
		SDL_RenderFillRect(r, &rc);
		// 边框（选中加粗）
		bool isSrc = ((int)i == selectedSource_);
		bool isDst = ((int)i == selectedTarget_);
		if (isSrc) SDL_SetRenderDrawColor(r, 255, 215, 0, 255);
		else if (isDst) SDL_SetRenderDrawColor(r, 0, 220, 180, 255);
		else SDL_SetRenderDrawColor(r, 60, 50, 40, 220);
		SDL_RenderDrawRect(r, &rc);
		if (isSrc || isDst) { for (int t=1; t<=3; ++t) { SDL_Rect rr{ rc.x - t, rc.y - t, rc.w + 2*t, rc.h + 2*t }; SDL_RenderDrawRect(r, &rr); } }

		// 名称
		if (smallFont_) {
			SDL_Color nameCol{50,40,30,255};
			SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, c.name.c_str(), nameCol);
			if (s) {
				SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
				int desiredH = SDL_max(12, (int)(rc.h * 0.16f)); float sc=(float)desiredH/(float)s->h; int w=(int)(s->w*sc);
				SDL_Rect nd{ rc.x + (rc.w - w)/2, rc.y + (int)(rc.h*0.05f), w, desiredH }; SDL_RenderCopy(r, t, nullptr, &nd); SDL_DestroyTexture(t);
				SDL_SetRenderDrawColor(r, 80,70,60,220); int lineY = nd.y + nd.h + SDL_max(2, (int)(rc.h*0.015f)); int thick = SDL_max(1, (int)(rc.h*0.007f)); for (int k=0;k<thick;++k) SDL_RenderDrawLine(r, rc.x+6, lineY+k, rc.x+rc.w-6, lineY+k);
				SDL_FreeSurface(s);
			}
		}
		// 攻击/生命
		if (smallFont_) {
			int desiredH = SDL_max(12, (int)(rc.h * 0.18f)); int margin = SDL_max(6, (int)(rc.h * 0.035f)); char buf[32];
			SDL_Color atkCol{80,50,40,255}; snprintf(buf, sizeof(buf), "%d", c.attack); SDL_Surface* sa = TTF_RenderUTF8_Blended(smallFont_, buf, atkCol);
			if (sa) { SDL_Texture* ta = SDL_CreateTextureFromSurface(r, sa); float sc=(float)desiredH/(float)sa->h; int w=(int)(sa->w*sc); SDL_Rect ad{ rc.x + margin, rc.y + rc.h - desiredH - margin, w, desiredH }; SDL_RenderCopy(r, ta, nullptr, &ad); SDL_DestroyTexture(ta); SDL_FreeSurface(sa);} 
			SDL_Color hpCol{160,30,40,255}; snprintf(buf, sizeof(buf), "%d", c.health); SDL_Surface* sh = TTF_RenderUTF8_Blended(smallFont_, buf, hpCol);
			if (sh) { SDL_Texture* th = SDL_CreateTextureFromSurface(r, sh); float sc=(float)desiredH/(float)sh->h; int w=(int)(sh->w*sc); SDL_Rect hd{ rc.x + rc.w - w - margin, rc.y + rc.h - desiredH - margin, w, desiredH }; SDL_RenderCopy(r, th, nullptr, &hd); SDL_DestroyTexture(th); SDL_FreeSurface(sh);} 
		}
		// 印记一行
		if (smallFont_ && !c.marks.empty()) {
			std::string marks; for (size_t mi=0; mi<c.marks.size(); ++mi) { if (mi) marks += "  "; marks += c.marks[mi]; }
			SDL_Color markCol{70,80,120,220}; SDL_Surface* ms = TTF_RenderUTF8_Blended_Wrapped(smallFont_, marks.c_str(), markCol, rc.w - 12);
			if (ms) { SDL_Texture* mt = SDL_CreateTextureFromSurface(r, ms); int top = rc.y + (int)(rc.h*0.28f); SDL_Rect md{ rc.x + 6, top, ms->w, ms->h }; SDL_RenderCopy(r, mt, nullptr, &md); SDL_DestroyTexture(mt); SDL_FreeSurface(ms);} 
		}
		// 水印
		if ((isSrc || isDst) && smallFont_) {
			const char* txt = isSrc ? u8"源" : u8"承"; SDL_Color mwCol = isSrc ? SDL_Color{255,220,100,140} : SDL_Color{120,240,220,140};
			SDL_Surface* ws = TTF_RenderUTF8_Blended(smallFont_, txt, mwCol); if (ws) { SDL_Texture* wt = SDL_CreateTextureFromSurface(r, ws); int targetH = SDL_max(24, (int)(rc.h * 0.5f)); float sc=(float)targetH/(float)ws->h; int w=(int)(ws->w*sc); SDL_Rect wd{ rc.x + (rc.w - w)/2, rc.y + (rc.h - targetH)/2, w, targetH }; SDL_SetTextureAlphaMod(wt, 130); SDL_RenderCopy(r, wt, nullptr, &wd); SDL_DestroyTexture(wt); SDL_FreeSurface(ws);} 
		}
	}

	// 提示/信息
	if (smallFont_) {
		SDL_Color col{220,230,245,255}; std::string tip = message_.empty()? u8"点击选择牺牲卡，再选择目标卡，点击右上确定传承；上方滑条调节手牌数量" : message_; SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, tip.c_str(), col, screenW_-40); if (s) { SDL_Texture* t = SDL_CreateTextureFromSurface(r, s); SDL_Rect dst{20, screenH_-s->h-20, s->w, s->h}; SDL_RenderCopy(r,t,nullptr,&dst); SDL_DestroyTexture(t); SDL_FreeSurface(s);} 
	}
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

void HeritageState::layoutSlider() { int trackW = SDL_max(200, screenW_ - 2*220); int trackH=6; int trackX=(screenW_-trackW)/2; int trackY=110; sliderTrack_={trackX,trackY,trackW,trackH}; int thumbW=14, thumbH=24; float t=(float)(SDL_clamp(handCount_,handMin_,handMax_) - handMin_)/(float)(handMax_ - handMin_); int thumbX = trackX + (int)(t*trackW) - thumbW/2; int thumbY=trackY + trackH/2 - thumbH/2; sliderThumb_={thumbX,thumbY,thumbW,thumbH}; }

void HeritageState::updateSliderFromMouse(int mx) {
	int cx = SDL_clamp(mx, sliderTrack_.x, sliderTrack_.x + sliderTrack_.w); float t=(float)(cx - sliderTrack_.x)/(float)sliderTrack_.w; int v=handMin_ + (int)(t*(handMax_-handMin_)+0.5f); v=SDL_clamp(v,handMin_,handMax_);
	if (v!=handCount_) {
		handCount_=v;
		// 根据新数量同步 hand（简单策略）
		auto& store = DeckStore::instance();
		while ((int)store.hand().size() < handCount_ && !store.library().empty()) store.drawToHand(1);
		while ((int)store.hand().size() > handCount_ && !store.hand().empty()) store.discardFromHand((int)store.hand().size()-1);
		layoutGrid();
	}
	int tw=sliderThumb_.w, th=sliderThumb_.h; int tx = sliderTrack_.x + (int)( (float)(handCount_-handMin_)/(float)(handMax_-handMin_) * sliderTrack_.w) - tw/2; int ty=sliderTrack_.y + sliderTrack_.h/2 - th/2; sliderThumb_={tx,ty,tw,th};
}

void HeritageState::performInheritance() {
	if (selectedSource_==-1 || selectedTarget_==-1) { message_ = u8"请先选定牺牲卡与目标卡"; return; }
	if (selectedSource_==selectedTarget_) { message_ = u8"目标不能与牺牲相同"; return; }
	auto& store = DeckStore::instance();
	// 如果源在目标之前，调用 inherit 后目标索引会左移一位
	bool targetAfter = selectedTarget_ > selectedSource_;
	store.inheritMarks(selectedSource_, selectedTarget_);
	if (targetAfter) selectedTarget_ -= 1;
	// 同步 handCount_ 显示
	if (handCount_ > handMin_) handCount_ -= 1;
	selectedSource_ = -1; selectedTarget_ = -1; layoutGrid();
	message_ = u8"传承成功：源卡已消失，印记并入目标";
}


