#include "MemoryRepairState.h"
#include "../core/App.h"
#include "../core/Deck.h"
#include "../core/Cards.h"
#include "../ui/CardRenderer.h"
#include "MapExploreState.h"
#include <random>
#include <algorithm>
#include <unordered_set>

// 定义静态变量
MemoryRepairState::BackHintType MemoryRepairState::mapHintType_ = BackHintType::Unknown;

static void drawBackIcon(SDL_Renderer* r, bool bone, int x, int y, int size) {
    if (bone) {
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_Rect boneRect{ x, y, size, size };
        SDL_RenderFillRect(r, &boneRect);
        SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
        SDL_RenderDrawRect(r, &boneRect);
        SDL_RenderDrawLine(r, x + 1, y + 1, x + size - 2, y + size - 2);
        SDL_RenderDrawLine(r, x + size - 2, y + 1, x + 1, y + size - 2);
    } else {
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_Rect dropRect{ x, y, size, size };
        SDL_RenderFillRect(r, &dropRect);
        SDL_RenderDrawLine(r, x + size/2, y + size, x + size/2, y + size + size/2);
    }
}

MemoryRepairState::MemoryRepairState() {
	backButton_ = new Button();
	rerollButton_ = new Button();
}

MemoryRepairState::MemoryRepairState(bool isBossVictory) : isBossVictory_(isBossVictory) {
	backButton_ = new Button();
	rerollButton_ = new Button();
}
MemoryRepairState::~MemoryRepairState() {
	if (backButton_) { delete backButton_; backButton_ = nullptr; }
	if (rerollButton_) { delete rerollButton_; rerollButton_ = nullptr; }
	if (titleFont_) { TTF_CloseFont(titleFont_); titleFont_ = nullptr; }
	if (smallFont_) { TTF_CloseFont(smallFont_); smallFont_ = nullptr; }
	if (cardNameFont_) { TTF_CloseFont(cardNameFont_); cardNameFont_ = nullptr; }
	if (cardStatFont_) { TTF_CloseFont(cardStatFont_); cardStatFont_ = nullptr; }
}

void MemoryRepairState::onEnter(App& app) {
	int w,h; SDL_GetWindowSize(app.getWindow(), &w, &h); screenW_ = w; screenH_ = h;
	// 加载字体
	titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 32);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 16);
	cardNameFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 14);
	cardStatFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 12);
	if (!titleFont_ || !smallFont_ || !cardNameFont_ || !cardStatFont_) {
		SDL_Log("TTF_OpenFont failed: %s", TTF_GetError());
	}
	// 返回按钮
	if (backButton_) {
		SDL_Rect rc{20,20,100,40};
		backButton_->setRect(rc);
		backButton_->setText(u8"返回地图");
		if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer());
		backButton_->setOnClick([&app]() {
			app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState())));
		});
	}
	// 重新抽卡按钮（Boss战胜利时不显示）
	if (rerollButton_ && !isBossVictory_) {
		SDL_Rect rc{140,20,120,40}; // 临时位置，会在layoutCandidates后重新设置
		rerollButton_->setRect(rc);
		rerollButton_->setText(u8"重新抽卡");
		if (smallFont_) rerollButton_->setFont(smallFont_, app.getRenderer());
		rerollButton_->setOnClick([this]() {
			if (!rerollUsed_) {
				rerollUsed_ = true;
				// 重新生成候选卡牌
				buildCandidates();
				layoutCandidates();
				// 重置翻转状态
				for (int i = 0; i < 3; ++i) flipped_[i] = false;
				selected_ = -1;
				added_ = false;
			}
		});
	}
	// 使用地图传递的提示类型，如果没有则随机选择
	if (mapHintType_ != BackHintType::Unknown) {
		sessionHint_ = mapHintType_;
		// 重置地图提示类型，避免影响下次
		mapHintType_ = BackHintType::Unknown;
	} else {
		// 随机选择提示类型（兼容旧逻辑）
		std::random_device rd; std::mt19937 gen(rd());
		std::uniform_int_distribution<int> d(0,2);
		int r = d(gen);
		sessionHint_ = (r==0? BackHintType::Unknown : (r==1? BackHintType::KnownTribe : BackHintType::KnownCost));
	}
	// 如果是已知消耗，随机分配三个不同的消耗类型
	if (sessionHint_ == BackHintType::KnownCost) {
		std::random_device rd; std::mt19937 gen(rd());
		// 四种消耗类型：0=1墨滴, 1=2墨滴, 2=3墨滴, 3=魂骨
		std::vector<int> costTypes = {0, 1, 2, 3};
		std::shuffle(costTypes.begin(), costTypes.end(), gen);
		// 取前三个作为三个牌位的消耗类型
		for (int i = 0; i < 3; ++i) {
			sessionCostTypes_[i] = costTypes[i];
		}
	}
	// 重置重新抽卡状态
	rerollUsed_ = false;
	buildCandidates();
	layoutCandidates();
	for (int i=0;i<3;++i) flipped_[i] = false;
}
void MemoryRepairState::onExit(App& app) {}
void MemoryRepairState::handleEvent(App& app, const SDL_Event& e) {
	// 处理返回按钮事件
	if (backButton_) backButton_->handleEvent(e);
	// 处理重新抽卡按钮事件（Boss战胜利时不处理）
	if (rerollButton_ && !rerollUsed_ && !isBossVictory_) rerollButton_->handleEvent(e);
	
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mx = e.button.x, my = e.button.y;
		for (int i=0;i<(int)candidates_.size();++i) {
			const auto& c = candidates_[i];
			if (mx>=c.rect.x && mx<=c.rect.x+c.rect.w && my>=c.rect.y && my<=c.rect.y+c.rect.h) {
				if (!flipped_[i]) { // 第一次点击：翻面
					flipped_[i] = true;
					selected_ = i;
				} else { // 已翻面：第二次点击加入并返回
					selected_ = i;
					if (!added_) {
						const std::string id = candidates_[selected_].card.id;
						Card c2 = CardDB::instance().make(id);
						DeckStore::instance().addToLibrary(c2);
						added_ = true;
						app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState())));
					}
				}
				break;
			}
		}
	}
}
void MemoryRepairState::update(App& app, float dt) {}
void MemoryRepairState::render(App& app) {
	SDL_Renderer* r = app.getRenderer();
	SDL_SetRenderDrawColor(r, 18, 22, 32, 255);
	SDL_RenderClear(r);

	// 标题
	if (titleFont_) {
		SDL_Color col{220,230,255,255};
		const char* titleText = isBossVictory_ ? u8"Boss战胜利奖励（三选一）" : u8"记忆修复（三选一）";
		SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, titleText, col);
		if (s) {
			SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
			SDL_Rect dst{ (screenW_-s->w)/2, 60, s->w, s->h };
			SDL_RenderCopy(r, t, nullptr, &dst);
			SDL_DestroyTexture(t);
			SDL_FreeSurface(s);
		}
	}

	// 返回按钮
	if (backButton_) backButton_->render(r);
	// 重新抽卡按钮（Boss战胜利时不渲染）
	if (rerollButton_ && !rerollUsed_ && !isBossVictory_) rerollButton_->render(r);

    // 渲染三张候选卡（使用与战斗界面相近尺寸）
	for (int i=0;i<(int)candidates_.size(); ++i) {
		const auto& c = candidates_[i];
		if (flipped_[i]) {
			CardRenderer::renderCard(app, c.card, c.rect, cardNameFont_, cardStatFont_, selected_ == i);
		} else {
            // 牌背：基础底色与边框
			SDL_SetRenderDrawColor(r, 60, 65, 80, 255);
			SDL_RenderFillRect(r, &c.rect);
			SDL_SetRenderDrawColor(r, 120, 130, 160, 255);
			for (int t=0;t<2;++t) { SDL_Rect rr{ c.rect.x - t, c.rect.y - t, c.rect.w + 2*t, c.rect.h + 2*t }; SDL_RenderDrawRect(r, &rr); }
            // 根据提示类型绘制牌背图案
            if (c.hint == BackHintType::Unknown) {
                // 无图案（纯背面）
            } else if (c.hint == BackHintType::KnownTribe) {
                // 在牌背中央用文字标识部族（放大显示，后绘制，避免被覆盖）
                if (smallFont_ && !c.card.category.empty()) {
                    SDL_Color col{220, 230, 255, 255};
                    SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, c.card.category.c_str(), col);
                    if (s) {
                        SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                        int desiredH = SDL_max(14, (int)(c.rect.h * 0.24f));
                        float scale = (float)desiredH / (float)s->h;
                        int scaledW = (int)(s->w * scale);
                        SDL_Rect dst{ c.rect.x + (c.rect.w - scaledW)/2, c.rect.y + (c.rect.h - desiredH)/2, scaledW, desiredH };
                        SDL_RenderCopy(r, t, nullptr, &dst);
                        SDL_DestroyTexture(t);
                        SDL_FreeSurface(s);
                    }
                }
            } else if (c.hint == BackHintType::KnownCost) {
                // 使用对应牌位的消耗类型：消耗1墨滴、消耗2墨滴、消耗3墨滴、消耗魂骨
                int costType = sessionCostTypes_[i]; // 使用第i个牌位的消耗类型
                bool hasBone = (costType == 3);
                int count = hasBone ? 1 : (costType + 1); // 魂骨显示1个，墨滴显示对应数量
                if (count > 0) {
                    int sz = SDL_max(12, (int)(SDL_min(c.rect.w, c.rect.h) * 0.18f)); // 稍小一些
                    if (hasBone && count > 0) {
                        // 以"魂骨图案 X ？"的方式显示
                        int y = c.rect.y + (c.rect.h - sz) / 2;
                        int startX = c.rect.x + (c.rect.w - (sz + 30)) / 2; // 图标 + "X ？"宽度预估
                        drawBackIcon(r, true, startX, y, sz);
                        if (smallFont_) {
                            SDL_Color col{220, 230, 255, 255};
                            SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, "X ？", col);
                            if (s) {
                                SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                                int desiredH = SDL_max(10, (int)(sz * 0.8f));
                                float scale = (float)desiredH / (float)s->h;
                                int w = (int)(s->w * scale);
                                SDL_Rect dst{ startX + sz + 6, y + (sz - desiredH)/2, w, desiredH };
                                SDL_RenderCopy(r, t, nullptr, &dst);
                                SDL_DestroyTexture(t);
                                SDL_FreeSurface(s);
                            }
                        }
                    } else if (!hasBone && count > 0) {
                        // 显示墨滴图案
                        int gap = SDL_max(6, sz / 3);
                        int totalW = count * sz + (count - 1) * gap;
                        int startX = c.rect.x + (c.rect.w - totalW) / 2;
                        int y = c.rect.y + (c.rect.h - sz) / 2;
                        for (int k = 0; k < count; ++k) {
                            drawBackIcon(r, false, startX + k * (sz + gap), y, sz);
                        }
                    }
                }
            }
		}
	}
}

void MemoryRepairState::buildCandidates() {
	// 从全卡库随机选择不重复的最多3张
	candidates_.clear();
	std::vector<std::string> ids = CardDB::instance().allIds();
	if (ids.empty()) {
		// 回退：使用一个已知ID占位
		for (int i=0;i<3;++i) { Card c; c.id = "shulin_shucheng"; Candidate cand; cand.card = c; candidates_.push_back(cand); }
		return;
	}
	
	// Boss战胜利：专门生成稀有牌
	if (isBossVictory_) {
		std::vector<std::string> rareIds;
		for (const auto& id : ids) {
			Card c = CardDB::instance().make(id);
			// 只选择稀有牌（obtainable == 2）且可获取的卡牌
			if (c.obtainable == 2) {
				rareIds.push_back(id);
			}
		}
		
		if (!rareIds.empty()) {
			std::random_device rd; 
			std::mt19937 gen(rd());
			std::shuffle(rareIds.begin(), rareIds.end(), gen);
			
			// 取前3张稀有牌，确保不重复
			std::unordered_set<std::string> usedIds;
			for (const auto& id : rareIds) {
				if (usedIds.find(id) == usedIds.end() && candidates_.size() < 3) {
					Card c = CardDB::instance().make(id);
					Candidate cand; 
					cand.card = c; 
					cand.hint = BackHintType::Unknown; // Boss战胜利不显示提示
					candidates_.push_back(cand);
					usedIds.insert(id);
				}
			}
		}
		
		// 如果稀有牌不足3张，用普通牌补齐
		while (candidates_.size() < 3 && !ids.empty()) {
			std::random_device rd; 
			std::mt19937 gen(rd());
			std::uniform_int_distribution<int> dis(0, ids.size() - 1);
			std::string randomId = ids[dis(gen)];
			Card c = CardDB::instance().make(randomId);
			if (c.obtainable > 0) {
				Candidate cand; 
				cand.card = c; 
				cand.hint = BackHintType::Unknown;
				candidates_.push_back(cand);
			}
		}
		return;
	}
	std::random_device rd; std::mt19937 gen(rd());
    std::shuffle(ids.begin(), ids.end(), gen);
    std::vector<std::string> filtered;
    if (sessionHint_ == BackHintType::KnownTribe) {
        // 从不同部族各取一张，最多三张；忽略"其他"类别，只考虑可获取的卡牌
        std::unordered_map<std::string, std::vector<std::string>> catToIds;
        for (const auto& id : ids) {
            Card c = CardDB::instance().make(id);
            if (c.category.empty() || c.category == u8"其他" || c.obtainable == 0) continue;
            catToIds[c.category].push_back(id);
        }
        // 收集并打乱部族列表
        std::vector<std::string> cats; cats.reserve(catToIds.size());
        for (auto& kv : catToIds) cats.push_back(kv.first);
        std::shuffle(cats.begin(), cats.end(), gen);
        int takeCats = (int)SDL_min((int)cats.size(), 3);
        for (int i = 0; i < takeCats; ++i) {
            auto& vec = catToIds[cats[i]];
            if (vec.empty()) continue;
            std::shuffle(vec.begin(), vec.end(), gen);
            filtered.push_back(vec.front());
        }
    } else if (sessionHint_ == BackHintType::KnownCost) {
        // 根据每个牌位的消耗类型生成对应的卡牌
        for (int i = 0; i < 3; ++i) {
            int costType = sessionCostTypes_[i];
            std::vector<std::string> matchingCards;
            
            for (const auto& id : ids) {
                Card c = CardDB::instance().make(id);
                if (c.obtainable == 0) continue;
                
                bool hasBone = false; 
                for (const auto& m : c.marks) { 
                    if (m == u8"消耗骨头") { 
                        hasBone = true; 
                        break; 
                    } 
                }
                
                if (costType == 3) {
                    // 消耗魂骨的情况
                    if (hasBone) {
                        matchingCards.push_back(id);
                    }
                } else {
                    // 消耗墨滴的情况：1、2、3墨滴
                    if (!hasBone && c.sacrificeCost == (costType + 1)) {
                        matchingCards.push_back(id);
                    }
                }
            }
            
            if (!matchingCards.empty()) {
                std::shuffle(matchingCards.begin(), matchingCards.end(), gen);
                filtered.push_back(matchingCards.front());
            } else {
                // 如果没有匹配的卡牌，使用默认卡牌
                filtered.push_back("shulin_shucheng");
            }
        }
    } else {
        // 默认情况：只考虑可获取的卡牌
        for (const auto& id : ids) {
            Card c = CardDB::instance().make(id);
            if (c.obtainable > 0) filtered.push_back(id);
        }
    }
    
    // 只有在非已知消耗的情况下才打乱顺序
    if (sessionHint_ != BackHintType::KnownCost) {
        std::shuffle(filtered.begin(), filtered.end(), gen);
    }
    
    int take = (int)SDL_min((int)filtered.size(), 3);
    for (int i=0; i<take; ++i) {
        Card c = CardDB::instance().make(filtered[i]);
        Candidate cand; cand.card = c; cand.hint = sessionHint_;
        candidates_.push_back(cand);
    }
	// 若少于3张，用第一张补齐到3张，确保界面完整
	while ((int)candidates_.size() < 3) {
		Candidate cand; cand.card = candidates_.empty() ? CardDB::instance().make("shulin_shucheng") : candidates_[0].card;
		candidates_.push_back(cand);
	}
}

void MemoryRepairState::layoutCandidates() {
	// 参照战斗界面卡面比例（大一些便于阅读）
	int w = 150; 
	int h = (int)(w * 1.4f);
	int gap = 40;
	int totalW = 3*w + 2*gap; 
	int x0 = (screenW_-totalW)/2; 
	int y = 180;
	for (int i=0;i<3 && i<(int)candidates_.size(); ++i) { candidates_[i].rect = { x0 + i*(w+gap), y, w, h }; }
	
	// 设置重新抽卡按钮位置（在牌位下方居中）
	if (rerollButton_) {
		int buttonY = y + h + 30; // 牌位下方30像素
		int buttonX = (screenW_ - 120) / 2; // 居中
		SDL_Rect buttonRect{ buttonX, buttonY, 120, 40 };
		rerollButton_->setRect(buttonRect);
	}
}
