#include "EngraveState.h"
#include "TestState.h"
#include "MapExploreState.h"
#include "../core/App.h"
#include <random>
#include <cmath>

EngraveState::EngraveState() = default;
EngraveState::~EngraveState() {
	if (titleTex_) SDL_DestroyTexture(titleTex_);
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (smallFont_) TTF_CloseFont(smallFont_);
	if (choiceFont_) TTF_CloseFont(choiceFont_);
	delete backButton_;
}

void EngraveState::onEnter(App& app) {
	int w,h; SDL_GetWindowSize(app.getWindow(), &w, &h); screenW_ = w; screenH_ = h;
	titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 56);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 18);
    choiceFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 32);
	if (titleFont_) {
		SDL_Color col{200,230,255,255};
		SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, u8"意境刻画：三选一", col);
		if (s) { titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), s); SDL_FreeSurface(s);} 
	}

	backButton_ = new Button();
	if (backButton_) {
		backButton_->setRect({20,20,120,36});
		backButton_->setText(u8"返回测试");
		if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer());
		backButton_->setOnClick([this]() { pendingGoMapExplore_ = true; });
	}

    // 不再需要确认按钮

	buildRandomChoices();
	layoutChoices();
}

void EngraveState::onExit(App& app) {}

void EngraveState::handleEvent(App& app, const SDL_Event& e) {
	if (backButton_) backButton_->handleEvent(e);
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mx=e.button.x, my=e.button.y;
		for (int i=0;i<(int)choices_.size();++i) {
			const auto& c = choices_[i];
			if (mx>=c.rect.x && mx<=c.rect.x+c.rect.w && my>=c.rect.y && my<=c.rect.y+c.rect.h) { selected_ = i; break; }
		}

        // 选择后立即生效
        if (selected_ != -1 && !hasChosen_) {
            const auto& c = choices_[selected_];
            auto& store = EngraveStore::instance();
            if (c.isYi) store.addYi(c.value); else store.addJing(c.value);
            result_ = c.title + u8" 已记录";
            // 调试信息
            printf("选择记录: %s, 当前意数量: %d, 境数量: %d\n", c.value.c_str(), (int)store.yis().size(), (int)store.jings().size());
            hasChosen_ = true;
            
            // 检查是否需要手动组合（多个意或多个境）
            if (store.yis().size() > 1 || store.jings().size() > 1) {
                // 检查是否为相同类型（AA、AAA、BB、BBB等情况）
                if ((store.yis().size() > 0 && store.jings().size() == 0) || 
                    (store.jings().size() > 0 && store.yis().size() == 0)) {
                    // 相同类型情况（AA、AAA、BB、BBB等），直接返回地图
                    isAnimating_ = true;
                    animTime_ = 0.0f;
                } else {
                    // 有不同类型（意和境都有），进入组合选择模式
                    showCombinePanel_ = true;
                    selectedYi_ = -1;
                    selectedJing_ = -1;
                }
            } else {
                // 唯一组合或无组合，直接返回地图
                isAnimating_ = true;
                animTime_ = 0.0f;
            }
        }

        // 自由组合面板点击：当满足条件时，点击选择意/境进行组合
        auto& store = EngraveStore::instance();
        bool showCombine = hasChosen_ && !isAnimating_ && showCombinePanel_ && !store.yis().empty() && !store.jings().empty();
        if (showCombine) {
            const auto& yis = store.yis();
            const auto& jings = store.jings();
            
            // 意选择区域（居中）
            int circle = 60; int gap = 20;
            int totalWidth = (int)yis.size() * circle + ((int)yis.size() - 1) * gap;
            int startX = (screenW_ - totalWidth) / 2;
            int yY = screenH_ - 250;
            for (int i=0;i<(int)yis.size(); ++i) {
                int cx = startX + circle/2 + i*(circle + gap);
                int cy = yY + circle/2;
                SDL_Rect r{ cx - circle/2, cy - circle/2, circle, circle };
                if (mx>=r.x && mx<=r.x+r.w && my>=r.y && my<=r.y+r.h) { 
                    selectedYi_ = i; 
                    result_ = std::string(u8"已选择意：") + yis[i];
                    // 自动组合：如果已经选择了意和境，立即组合
                    if (selectedYi_ >= 0 && selectedJing_ >= 0) {
                        auto& store = EngraveStore::instance();
                        const auto& yis = store.yis();
                        const auto& jings = store.jings();
                        if (selectedYi_ < (int)yis.size() && selectedJing_ < (int)jings.size()) {
                            store.combine(jings[selectedJing_], yis[selectedYi_]);
                            result_ = std::string(u8"已组合：") + jings[selectedJing_] + " + " + yis[selectedYi_];
                            isAnimating_ = true; animTime_ = 0.0f;
                        }
                    }
                }
            }
            
            // 境选择区域（居中，在意下方）
            int square = 60;
            int totalWidthJ = (int)jings.size() * square + ((int)jings.size() - 1) * gap;
            int startXJ = (screenW_ - totalWidthJ) / 2;
            int jY = screenH_ - 140;
            for (int i=0;i<(int)jings.size(); ++i) {
                int sx = startXJ + i*(square + gap);
                int sy = jY;
                SDL_Rect r{ sx, sy, square, square };
            if (mx>=r.x && mx<=r.x+r.w && my>=r.y && my<=r.y+r.h) { 
                selectedJing_ = i; 
                result_ = std::string(u8"已选择境：") + jings[i];
                // 自动组合：如果已经选择了意和境，立即组合
                if (selectedYi_ >= 0 && selectedJing_ >= 0) {
                    auto& store = EngraveStore::instance();
                    const auto& yis = store.yis();
                    const auto& jings = store.jings();
                    if (selectedYi_ < (int)yis.size() && selectedJing_ < (int)jings.size()) {
                        store.combine(jings[selectedJing_], yis[selectedYi_]);
                        result_ = std::string(u8"已组合：") + jings[selectedJing_] + " + " + yis[selectedYi_];
                        isAnimating_ = true; animTime_ = 0.0f;
                    }
                }
            }
            }
        }
	}

    // 不再需要Enter键处理，组合会自动进行
}

void EngraveState::update(App& app, float dt) {
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
    // 播放选择完成动画，结束后回到地图
    if (isAnimating_) {
        animTime_ += dt;
        if (animTime_ >= animDuration_) {
            isAnimating_ = false;
            pendingGoMapExplore_ = true;
        }
	}
}

void EngraveState::render(App& app) {
	SDL_Renderer* r = app.getRenderer();
	SDL_SetRenderDrawColor(r, 18, 22, 32, 255);
	SDL_RenderClear(r);

    if (titleTex_) { int tw,th; SDL_QueryTexture(titleTex_,nullptr,nullptr,&tw,&th); SDL_Rect dst{ (screenW_-tw)/2, 40, tw, th }; SDL_RenderCopy(r, titleTex_, nullptr, &dst);} 
	// 显示组合提示
	auto& store = EngraveStore::instance();
	if (!store.getLastCombineHint().empty() && choiceFont_) {
		SDL_Color col{255,255,100,255}; // 金黄色提示
		std::string hintText = u8"提示:已拥有意境,可在下次进入此界面时进行意+境自定义组合(后续将提供入口)";
		SDL_Surface* s = TTF_RenderUTF8_Blended(choiceFont_, hintText.c_str(), col);
		if (s) {
			SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
			SDL_Rect d{ 40, screenH_ - 60, s->w, s->h };
			SDL_RenderCopy(r, t, nullptr, &d);
			SDL_DestroyTexture(t);
			SDL_FreeSurface(s);
		}
	}

	if (backButton_) backButton_->render(r);
    {
        // 简易填充圆绘制
        auto drawFilledCircle = [&](int cx, int cy, int radius, SDL_Color fill, SDL_Color border) {
            SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, fill.a);
            for (int dy = -radius; dy <= radius; ++dy) {
                int dxMax = (int)std::floor(std::sqrt((double)radius * radius - (double)dy * dy));
                int y = cy + dy;
                SDL_RenderDrawLine(r, cx - dxMax, y, cx + dxMax, y);
            }
            SDL_SetRenderDrawColor(r, border.r, border.g, border.b, border.a);
            for (int a = 0; a < 360; a += 3) {
                double rad = a * 3.1415926535 / 180.0;
                int x = cx + (int)std::round(radius * std::cos(rad));
                int y = cy + (int)std::round(radius * std::sin(rad));
                SDL_RenderDrawPoint(r, x, y);
            }
        };

	for (int i=0;i<(int)choices_.size(); ++i) {
		auto& c = choices_[i];
            _TTF_Font* font = choiceFont_ ? choiceFont_ : smallFont_;
            if (c.isYi) {
                // 圆形：印记
                int pad = 12;
                int cx = c.rect.x + c.rect.w/2;
                int cy = c.rect.y + c.rect.h/2;
                int side = std::min(c.rect.w, c.rect.h);
                int radius = std::max(24, side/2 - pad);
                SDL_Color fill{235,230,220,230};
                SDL_Color border = (i==selected_) ? SDL_Color{0,220,180,255} : SDL_Color{60,50,40,220};
                drawFilledCircle(cx, cy, radius, fill, border);
                if (font) {
                    SDL_Color col{40,40,50,255};
                    SDL_Surface* s = TTF_RenderUTF8_Blended(font, c.value.c_str(), col);
                    if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{ cx - s->w/2, cy - s->h/2, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} 
                }
            } else {
                // 正方形：部类
                int side = std::min(c.rect.w, c.rect.h);
                SDL_Rect sq{ c.rect.x + (c.rect.w - side)/2, c.rect.y + (c.rect.h - side)/2, side, side };
		SDL_SetRenderDrawColor(r, 235,230,220,230);
                SDL_RenderFillRect(r, &sq);
                SDL_SetRenderDrawColor(r, (i==selected_)?0:60, (i==selected_)?220:50, (i==selected_)?180:40, 220);
                SDL_RenderDrawRect(r, &sq);
                if (font) {
                    SDL_Color col{40,40,50,255};
                    SDL_Surface* s = TTF_RenderUTF8_Blended(font, c.value.c_str(), col);
                    if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{ sq.x + (sq.w - s->w)/2, sq.y + (sq.h - s->h)/2, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} 
                }
            }
        }
    }

    // 已拥有意境展示（小一号）：始终显示已收集的意/境池，但组合界面时不显示
    {
        auto& storeView = EngraveStore::instance();
        const auto& yiPool = storeView.yis();
        const auto& jingPool = storeView.jings();
        bool showCombine = hasChosen_ && !isAnimating_ && showCombinePanel_ && !storeView.yis().empty() && !storeView.jings().empty();
        if ((!yiPool.empty() || !jingPool.empty()) && !showCombine) {
            int topY = 220 + 240 + 28;
            SDL_Color labelCol{200,230,255,255};
            if (choiceFont_) {
                SDL_Surface* s = TTF_RenderUTF8_Blended(choiceFont_, u8"已收集：", labelCol);
                if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{ 40, topY, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s); topY += s->h + 12; }
            }
            _TTF_Font* lblFont = smallFont_ ? smallFont_ : choiceFont_;
            int startX = 40;
            int x = startX;
            int y = topY;
            int circle = 64; int gap = 14;
            // 画意（圆）
            for (size_t i=0;i<yiPool.size(); ++i) {
                int cx = x + circle/2; int cy = y + circle/2;
                // 填充圆 - 黑白色
                SDL_Color fill{200,200,200,200}; SDL_Color border{100,100,100,220};
                SDL_SetRenderDrawColor(r,fill.r,fill.g,fill.b,fill.a);
                for (int dy=-circle/2; dy<=circle/2; ++dy) {
                    int dxMax=(int)std::floor(std::sqrt((double)(circle/2)*(circle/2)-(double)dy*dy));
                    int yy=cy+dy; SDL_RenderDrawLine(r,cx-dxMax,yy,cx+dxMax,yy);
                }
                SDL_SetRenderDrawColor(r,border.r,border.g,border.b,border.a);
                for (int a=0;a<360;a+=6){ double rad=a*3.1415926535/180.0; int px=cx+(int)std::round((circle/2)*std::cos(rad)); int py=cy+(int)std::round((circle/2)*std::sin(rad)); SDL_RenderDrawPoint(r,px,py);} 
                if (lblFont){ SDL_Color col{50,50,50,220}; SDL_Surface* s=TTF_RenderUTF8_Blended(lblFont, yiPool[i].c_str(), col); if (s){ SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{ cx - s->w/2, cy - s->h/2, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} }
                x += circle + gap;
                if (x + circle > screenW_-20) { x = startX; y += circle + gap; }
            }
            // 换行后画境（方）
            y += circle + gap;
            int square = 68;
            x = startX;
            for (size_t i=0;i<jingPool.size(); ++i) {
                SDL_Rect sq{ x, y, square, square };
                SDL_SetRenderDrawColor(r, 200,200,200,200); SDL_RenderFillRect(r, &sq);
                SDL_SetRenderDrawColor(r, 100,100,100,220); SDL_RenderDrawRect(r, &sq);
                if (lblFont){ SDL_Color col{50,50,50,220}; SDL_Surface* s=TTF_RenderUTF8_Blended(lblFont, jingPool[i].c_str(), col); if (s){ SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{ sq.x + (sq.w - s->w)/2, sq.y + (sq.h - s->h)/2, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} }
                x += square + gap;
                if (x + square > screenW_-20) { x = startX; y += square + gap; }
            }
        }
    }

	if (!result_.empty() && smallFont_) {
		SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, result_.c_str(), col, screenW_-40); if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{20, screenH_-s->h-20, s->w, s->h}; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} 
	}

    // 显示自由组合界面
    auto& storeCombine = EngraveStore::instance();
    bool showCombine = hasChosen_ && !isAnimating_ && showCombinePanel_ && !storeCombine.yis().empty() && !storeCombine.jings().empty();
    if (showCombine) {
        const auto& yis = storeCombine.yis();
        const auto& jings = storeCombine.jings();
        
        // 背景遮罩
        SDL_SetRenderDrawColor(r, 0, 0, 0, 180);
        SDL_Rect overlay{0, 0, screenW_, screenH_};
        SDL_RenderFillRect(r, &overlay);
        
        // 标题
        if (choiceFont_) {
            SDL_Color col{255,255,255,255};
            SDL_Surface* s = TTF_RenderUTF8_Blended(choiceFont_, u8"选择要组合的意和境", col);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                SDL_Rect d{ (screenW_-s->w)/2, screenH_ - 350, s->w, s->h };
                SDL_RenderCopy(r, t, nullptr, &d);
                SDL_DestroyTexture(t);
                SDL_FreeSurface(s);
            }
        }
        
        // 意选择区域（居中，下移避免重叠）
        if (smallFont_) {
            SDL_Color col{255,255,255,255};
            SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, u8"意（印记）", col);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                SDL_Rect d{ (screenW_-s->w)/2, screenH_ - 300, s->w, s->h };
                SDL_RenderCopy(r, t, nullptr, &d);
                SDL_DestroyTexture(t);
                SDL_FreeSurface(s);
            }
        }
        
        // 计算意选择区域的居中位置
        int circle = 60; int gap = 20;
        int totalWidth = (int)yis.size() * circle + ((int)yis.size() - 1) * gap;
        int startX = (screenW_ - totalWidth) / 2;
        int yY = screenH_ - 250;
        
        for (int i=0;i<(int)yis.size(); ++i) {
            int cx = startX + circle/2 + i*(circle + gap);
            int cy = yY + circle/2;
            
            // 绘制圆形
            SDL_Color fill = (selectedYi_ == i) ? SDL_Color{100,200,255,255} : SDL_Color{220,220,220,200};
            SDL_Color border = (selectedYi_ == i) ? SDL_Color{255,255,255,255} : SDL_Color{150,150,150,220};
            
            for (int dy = -circle/2; dy <= circle/2; ++dy) {
                int dxMax = (int)std::floor(std::sqrt((double)(circle/2)*(circle/2) - (double)dy*dy));
                int y = cy + dy;
                SDL_RenderDrawLine(r, cx - dxMax, y, cx + dxMax, y);
            }
            SDL_SetRenderDrawColor(r, border.r, border.g, border.b, border.a);
            for (int a=0;a<360;a+=4){ 
                double rad=a*3.1415926535/180.0; 
                int x=cx+(int)std::round((circle/2)*std::cos(rad)); 
                int y=cy+(int)std::round((circle/2)*std::sin(rad)); 
                SDL_RenderDrawPoint(r,x,y);
            }
            
            // 文字
            if (smallFont_) {
                SDL_Color col{40,40,50,255};
                SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, yis[i].c_str(), col);
                if (s) {
                    SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                    SDL_Rect d{ cx - s->w/2, cy - s->h/2, s->w, s->h };
                    SDL_RenderCopy(r, t, nullptr, &d);
                    SDL_DestroyTexture(t);
                    SDL_FreeSurface(s);
                }
            }
        }
        
        // 境选择区域（居中，在意下方）
        if (smallFont_) {
            SDL_Color col{255,255,255,255};
            SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, u8"境（部类）", col);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                SDL_Rect d{ (screenW_-s->w)/2, screenH_ - 190, s->w, s->h };
                SDL_RenderCopy(r, t, nullptr, &d);
                SDL_DestroyTexture(t);
                SDL_FreeSurface(s);
            }
        }
        
        // 计算境选择区域的居中位置
        int square = 60;
        int totalWidthJ = (int)jings.size() * square + ((int)jings.size() - 1) * gap;
        int startXJ = (screenW_ - totalWidthJ) / 2;
        int jY = screenH_ - 140;
        
        for (int i=0;i<(int)jings.size(); ++i) {
            int sx = startXJ + i*(square + gap);
            int sy = jY;
            SDL_Rect rect{ sx, sy, square, square };
            
            // 绘制正方形
            SDL_Color fill = (selectedJing_ == i) ? SDL_Color{100,200,255,255} : SDL_Color{220,220,220,200};
            SDL_Color border = (selectedJing_ == i) ? SDL_Color{255,255,255,255} : SDL_Color{150,150,150,220};
            
            SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, fill.a);
            SDL_RenderFillRect(r, &rect);
            SDL_SetRenderDrawColor(r, border.r, border.g, border.b, border.a);
            SDL_RenderDrawRect(r, &rect);
            
            // 文字
            if (smallFont_) {
                SDL_Color col{40,40,50,255};
                SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, jings[i].c_str(), col);
                if (s) {
                    SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                    SDL_Rect d{ rect.x + (rect.w - s->w)/2, rect.y + (rect.h - s->h)/2, s->w, s->h };
                    SDL_RenderCopy(r, t, nullptr, &d);
                    SDL_DestroyTexture(t);
                    SDL_FreeSurface(s);
                }
            }
        }
        
        // 提示文字
        if (smallFont_) {
            SDL_Color col{255,255,100,255};
            SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, u8"选择意和境后自动组合", col);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                SDL_Rect d{ (screenW_-s->w)/2, screenH_ - 90, s->w, s->h };
                SDL_RenderCopy(r, t, nullptr, &d);
                SDL_DestroyTexture(t);
                SDL_FreeSurface(s);
            }
        }
    }
}

void EngraveState::buildRandomChoices() {
	choices_.clear();
    // 意：从已知印记池；境：从卡牌种族/部类（category）池
    static const char* yi[] = {u8"不死印记", u8"空袭", u8"护主", u8"双重攻击", u8"水袭", u8"坚硬之躯"};
    static const char* jing[] = {u8"介部", u8"鹿部", u8"犬部", u8"羽部", u8"鳞部"};
    const int yiCount = 6;
    const int jingCount = 5;

    // 伪随机：确保当次选项中至少包含一个意和一个境，且内容不重复
    std::random_device rd; std::mt19937 gen(rd());
    std::uniform_int_distribution<int> yiDist(0, yiCount - 1);
    std::uniform_int_distribution<int> jingDist(0, jingCount - 1);

    // 先保证一个意、一个境
    int yiIdx = yiDist(gen);
    int jingIdx = jingDist(gen);
    Choice cYi; cYi.isYi = true; cYi.value = yi[yiIdx]; cYi.title = std::string("意·") + cYi.value; cYi.desc = u8"选择一个印记"; choices_.push_back(cYi);
    Choice cJing; cJing.isYi = false; cJing.value = jing[jingIdx]; cJing.title = std::string("境·") + cJing.value; cJing.desc = u8"选择一个部类（种族）"; choices_.push_back(cJing);

    // 第三个选项：随机意或境，但不得与前两项内容重复
    std::uniform_int_distribution<int> typeDist(0,1);
    for (;;) {
        bool isYi = typeDist(gen) == 1;
        if (isYi) {
            int idx = yiDist(gen);
            std::string val = yi[idx];
            if (val != cYi.value) {
                Choice c; c.isYi = true; c.value = val; c.title = std::string("意·") + c.value; c.desc = u8"选择一个印记"; choices_.push_back(c);
                break;
            }
        } else {
            int idx = jingDist(gen);
            std::string val = jing[idx];
            if (val != cJing.value) {
                Choice c; c.isYi = false; c.value = val; c.title = std::string("境·") + c.value; c.desc = u8"选择一个部类（种族）"; choices_.push_back(c);
                break;
            }
        }
    }

    // 为了避免视觉上相邻两个同类，简单洗牌
    std::shuffle(choices_.begin(), choices_.end(), gen);
}

void EngraveState::layoutChoices() {
    int size = 240; int gap = 36; int totalW = 3*size + 2*gap; int x0 = (screenW_-totalW)/2; int y = 220;
    for (int i=0;i<3;++i) { choices_[i].rect = { x0 + i*(size+gap), y, size, size }; }
}


