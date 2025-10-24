#include "RelicPickupState.h"
#include "TestState.h"
#include "MapExploreState.h"
#include "../core/App.h"
#include "BattleState.h" // 为了复用 AVAILABLE_ITEMS 列表
#include "../core/Cards.h"
#include "../core/Deck.h"
#include "../core/ItemStore.h"
#include "../ui/CardRenderer.h"
#include <random>

RelicPickupState::RelicPickupState() = default;
RelicPickupState::~RelicPickupState() {
	if (titleTex_) SDL_DestroyTexture(titleTex_);
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (smallFont_) TTF_CloseFont(smallFont_);
	delete backButton_;
}

void RelicPickupState::onEnter(App& app) {
	int w,h; SDL_GetWindowSize(app.getWindow(), &w, &h); screenW_ = w; screenH_ = h;
	titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 56);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 18);
	if (titleFont_) { SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, u8"墨宝拾遗", col); if (s) { titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), s); SDL_FreeSurface(s);} }

	backButton_ = new Button();
	if (backButton_) { backButton_->setRect({20,20,120,36}); backButton_->setText(u8"返回地图"); if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer()); backButton_->setOnClick([this]() { pendingGoMapExplore_ = true; }); }

    setupPickupContent();
}

void RelicPickupState::onExit(App& app) {}

void RelicPickupState::handleEvent(App& app, const SDL_Event& e) {
    if (backButton_) backButton_->handleEvent(e);

    if (spawnCard_) {
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int mx = e.button.x, my = e.button.y;
            if (mx>=cardRect_.x && mx<=cardRect_.x+cardRect_.w && my>=cardRect_.y && my<=cardRect_.y+cardRect_.h) {
                Card c = CardDB::instance().make("shulin_shucheng");
                DeckStore::instance().addToLibrary(c);
                pendingGoMapExplore_ = true;
            }
        }
        return;
    }

    if (e.type == SDL_MOUSEMOTION) {
        int mx = e.motion.x, my = e.motion.y;
        
        // 处理印记悬停
        // 检查卡片中的印记悬停
        if (spawnCard_) {
            if (mx >= cardRect_.x && mx <= cardRect_.x + cardRect_.w &&
                my >= cardRect_.y && my <= cardRect_.y + cardRect_.h) {
                Card c = CardDB::instance().make("shulin_shucheng");
                CardRenderer::handleMarkHover(c, cardRect_, mx, my, smallFont_);
                return;
            }
        }
        
        // 检查候选道具中的印记悬停
        for (auto& cand : candidates_) {
            if (!cand.picked && mx >= cand.rect.x && mx <= cand.rect.x + cand.rect.w && 
                my >= cand.rect.y && my <= cand.rect.y + cand.rect.h) {
                // 检查是否有对应的卡牌可以悬停
                if (cand.item.id == "shulin_shucheng") {
                    Card c = CardDB::instance().make("shulin_shucheng");
                    CardRenderer::handleMarkHover(c, cand.rect, mx, my, smallFont_);
                    return;
                }
            }
        }
        
        // 如果没有悬停在任何印记上，隐藏提示
        App::hideMarkTooltip();
        
        for (auto& cand : candidates_) {
            cand.hovered = (mx>=cand.rect.x && mx<=cand.rect.x+cand.rect.w && my>=cand.rect.y && my<=cand.rect.y+cand.rect.h);
        }
    }
    // 处理印记右键点击
    else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
        int mx = e.button.x, my = e.button.y;
        
        // 检查卡片中的印记
        if (spawnCard_) {
            if (mx >= cardRect_.x && mx <= cardRect_.x + cardRect_.w &&
                my >= cardRect_.y && my <= cardRect_.y + cardRect_.h) {
                Card c = CardDB::instance().make("shulin_shucheng");
                CardRenderer::handleMarkClick(c, cardRect_, mx, my, smallFont_);
                if (App::isMarkTooltipVisible()) {
                    return;
                }
            }
        }
        
        // 检查候选道具中的印记
        for (auto& cand : candidates_) {
            if (!cand.picked && mx >= cand.rect.x && mx <= cand.rect.x + cand.rect.w && 
                my >= cand.rect.y && my <= cand.rect.y + cand.rect.h) {
                // 检查是否有对应的卡牌可以点击
                if (cand.item.id == "shulin_shucheng") {
                    Card c = CardDB::instance().make("shulin_shucheng");
                    CardRenderer::handleMarkClick(c, cand.rect, mx, my, smallFont_);
                    if (App::isMarkTooltipVisible()) {
                        return;
                    }
                }
            }
        }
    } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;
        for (auto& cand : candidates_) {
            if (!cand.picked && mx>=cand.rect.x && mx<=cand.rect.x+cand.rect.w && my>=cand.rect.y && my<=cand.rect.y+cand.rect.h) {
                // 启动拾取动画（先不标记为picked，等动画结束）
                pickupAnim_.active = true;
                pickupAnim_.time = 0.0f;
                pickupAnim_.rect = cand.rect;
                pickupAnim_.itemName = cand.item.name;
                message_ = "获得道具：" + cand.item.name;
                break; // 一次只处理一个
            }
        }
    }
}

void RelicPickupState::update(App& app, float dt) {
	// 更新拾取动画
	if (pickupAnim_.active) {
		pickupAnim_.time += dt;
		if (pickupAnim_.time >= pickupAnim_.duration) {
			// 动画结束后才真正拾取道具
			auto& globalStore = ItemStore::instance();
			if (globalStore.totalCount() < MAX_ITEMS) {
				// 找到对应的候选道具并标记为已拾取
				for (auto& cand : candidates_) {
					if (cand.rect.x == pickupAnim_.rect.x && cand.rect.y == pickupAnim_.rect.y) {
						globalStore.addItem(cand.item.id, cand.item.name, cand.item.description, 1);
						cand.picked = true;
						break;
					}
				}
			}
			pickupAnim_.active = false;
			
			// 检查是否全部拾取完成
			bool allPicked = true; 
			for (const auto& c : candidates_) if (!c.picked) { allPicked = false; break; }
			if (allPicked || globalStore.totalCount() >= MAX_ITEMS) {
				pendingGoMapExplore_ = true;
			}
		}
	}
	
	if (pendingBackToTest_) { pendingBackToTest_ = false; app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState()))); return; }
	if (pendingGoMapExplore_) { pendingGoMapExplore_ = false; app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState()))); return; }
}

void RelicPickupState::render(App& app) {
	SDL_Renderer* r = app.getRenderer(); SDL_SetRenderDrawColor(r, 18,22,32,255); SDL_RenderClear(r);
    if (titleTex_) { int tw,th; SDL_QueryTexture(titleTex_,nullptr,nullptr,&tw,&th); SDL_Rect d{ (screenW_-tw)/2, 60, tw, th }; SDL_RenderCopy(r,titleTex_,nullptr,&d); }
	if (backButton_ && App::isGodMode()) backButton_->render(r);

    if (spawnCard_) {
        // 居中渲染书林署丞卡
        SDL_Rect rect{ (screenW_-150)/2, (screenH_-210)/2, 150, 210 };
        cardRect_ = rect;
        Card c = CardDB::instance().make("shulin_shucheng");
        CardRenderer::renderCard(app, c, rect, smallFont_, smallFont_, false);
        if (smallFont_) {
            SDL_Color col{220,230,255,255};
            SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, u8"点击拾取加入牌库", col, 400);
            if (s) { SDL_Texture* t = SDL_CreateTextureFromSurface(r, s); SDL_Rect d{ (screenW_-s->w)/2, rect.y + rect.h + 16, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);}        
        }
        return;
    }

    // 渲染候选道具图标
    for (const auto& cand : candidates_) {
        // 跳过已拾取的道具
        if (cand.picked) {
            continue;
        }
        
        // 跳过正在播放拾取动画的道具
        if (pickupAnim_.active && pickupAnim_.rect.x == cand.rect.x && pickupAnim_.rect.y == cand.rect.y) {
            continue;
        }
        
        SDL_SetRenderDrawColor(r, cand.hovered ? 230 : 200, cand.hovered ? 220 : 200, cand.hovered ? 140 : 160, 255);
        SDL_RenderFillRect(r, &cand.rect);
        SDL_SetRenderDrawColor(r, 240, 240, 240, 255);
        SDL_RenderDrawRect(r, &cand.rect);
        if (smallFont_) {
            SDL_Color col{20,20,20,255};
            SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, cand.item.name.c_str(), col, cand.rect.w - 10);
            if (s) { SDL_Texture* t = SDL_CreateTextureFromSurface(r, s); SDL_Rect d{ cand.rect.x + (cand.rect.w - s->w)/2, cand.rect.y + cand.rect.h/2 - s->h/2, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);}        
        }
        // 悬停提示
        if (cand.hovered && smallFont_) {
            SDL_Color col{230,230,255,255};
            SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, cand.item.description.c_str(), col, 480);
            if (s) { SDL_Texture* t = SDL_CreateTextureFromSurface(r, s); SDL_Rect d{ (screenW_-s->w)/2, cand.rect.y - s->h - 10, s->w, s->h }; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);}        
        }
    }
    
    // 渲染拾取动画
    if (pickupAnim_.active) {
        float progress = pickupAnim_.time / pickupAnim_.duration;
        float alpha = 1.0f - progress; // 淡出效果
        float scale = 1.0f + progress * 0.5f; // 放大效果
        float moveY = -progress * 50.0f; // 向上移动
        
        SDL_Rect animRect = pickupAnim_.rect;
        animRect.x += (int)(animRect.w * (scale - 1.0f) * 0.5f);
        animRect.y += (int)moveY;
        animRect.w = (int)(animRect.w * scale);
        animRect.h = (int)(animRect.h * scale);
        
        // 绘制半透明道具框
        SDL_SetRenderDrawColor(r, 255, 255, 100, (Uint8)(255 * alpha));
        SDL_RenderFillRect(r, &animRect);
        SDL_SetRenderDrawColor(r, 255, 255, 0, (Uint8)(255 * alpha));
        SDL_RenderDrawRect(r, &animRect);
        
        // 绘制道具名称
        if (smallFont_) {
            SDL_Color col{255, 255, 0, (Uint8)(255 * alpha)};
            SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, pickupAnim_.itemName.c_str(), col, animRect.w - 10);
            if (s) { 
                SDL_Texture* t = SDL_CreateTextureFromSurface(r, s); 
                SDL_Rect d{ animRect.x + (animRect.w - s->w)/2, animRect.y + animRect.h/2 - s->h/2, s->w, s->h }; 
                SDL_RenderCopy(r,t,nullptr,&d); 
                SDL_DestroyTexture(t); 
                SDL_FreeSurface(s);
            }        
        }
    }

    if (!message_.empty() && smallFont_) { SDL_Color col{200,230,255,255}; SDL_Surface* s = TTF_RenderUTF8_Blended_Wrapped(smallFont_, message_.c_str(), col, screenW_-40); if (s) { SDL_Texture* t=SDL_CreateTextureFromSurface(r,s); SDL_Rect d{20, screenH_-s->h-20, s->w, s->h}; SDL_RenderCopy(r,t,nullptr,&d); SDL_DestroyTexture(t); SDL_FreeSurface(s);} }
    
    // 渲染全局印记提示
    CardRenderer::renderGlobalMarkTooltip(app, smallFont_);
}

void RelicPickupState::setupPickupContent() {
    // 若已满3件：生成书林署丞
    auto& globalStore = ItemStore::instance();
    if (globalStore.totalCount() >= MAX_ITEMS) {
        spawnCard_ = true;
        candidates_.clear();
        return;
    }
    spawnCard_ = false;
    candidates_.clear();
    // n = 3 - 当前数量
    int need = MAX_ITEMS - globalStore.totalCount();
    // 从现有道具系统的清单中随机（BattleState::AVAILABLE_ITEMS）
    std::vector<std::string> pool = BattleState::getAvailableItems();
    std::random_device rd; std::mt19937 g(rd());
    std::shuffle(pool.begin(), pool.end(), g);

    auto prettyName = [](const std::string& id) -> std::string {
        if (id == "yinyang_pei" || id == "yinyangpei") return u8"阴阳佩";
        if (id == "mobao_ping") return u8"墨宝瓶";
        if (id == "wangchuan_shi") return u8"忘川石";
        if (id == "fengya_shan") return u8"风雅扇";
        if (id == "rigui") return u8"日晷";
        if (id == "duanyinjian") return u8"断因剑";
        if (id == "tunmohao") return u8"吞墨毫";
        if (id == "fuhunsuo") return u8"缚魂锁";
        if (id == "wuzitianshu") return u8"无字天书";
        if (id == "xuanmuping") return u8"玄牡瓶";
        if (id == "tianjiluopan") return u8"天机罗盘";
        if (id == "sanguiping") return u8"散骨瓶";
        return id;
    };
    auto prettyDesc = [](const std::string& id) -> std::string {
        if (id == "yinyang_pei" || id == "yinyangpei") return u8"平衡墨尺偏移";
        if (id == "mobao_ping") return u8"存放溢出的墨量";
        if (id == "wangchuan_shi") return u8"转移伤害至忘川";
        if (id == "fengya_shan") return u8"赋予空袭等效果";
        if (id == "rigui") return u8"记录回合影响结算";
        if (id == "duanyinjian") return u8"选择目标斩断因缘";
        if (id == "tunmohao") return u8"摧毁敌牌并得狼皮";
        if (id == "fuhunsuo") return u8"拖拽移动敌方单位";
        if (id == "wuzitianshu") return u8"赋予随机印记";
        if (id == "xuanmuping") return u8"获得护体之力";
        if (id == "tianjiluopan") return u8"洞察牌堆";
        if (id == "sanguiping") return u8"使用后获得4个魂骨";
        return u8"神秘道具";
    };

    for (const auto& id : pool) {
        if ((int)candidates_.size() >= need) break;
        bool dup = false;
        for (const auto& c : candidates_) if (c.item.id == id) { dup = true; break; }
        if (dup) continue;
        ItemCandidate cand; cand.item = Item{ id, prettyName(id), prettyDesc(id) };
        candidates_.push_back(cand);
    }
    layoutCandidates();
}

void RelicPickupState::layoutCandidates() {
    int n = (int)candidates_.size(); if (n<=0) return;
    int w = 120, h = 120, gap = 30;
    int totalW = n*w + (n-1)*gap; int x0 = (screenW_-totalW)/2; int y = (screenH_-h)/2;
    for (int i=0;i<n;++i) candidates_[i].rect = { x0 + i*(w+gap), y, w, h };
}


