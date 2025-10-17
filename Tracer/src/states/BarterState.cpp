#include "BarterState.h"
#include "MapExploreState.h"
#include "../core/App.h"
#include "../core/Deck.h"
#include "../core/Cards.h"
#include "../ui/CardRenderer.h"

BarterState::BarterState() = default;

BarterState::~BarterState() {
    if (titleFont_) TTF_CloseFont(titleFont_);
    if (smallFont_) TTF_CloseFont(smallFont_);
    if (titleTex_) SDL_DestroyTexture(titleTex_);
    if (backButton_) delete backButton_;
}

void BarterState::onEnter(App& app) {
    // 屏幕与字体
    screenW_ = 1600; screenH_ = 1000; SDL_SetWindowSize(app.getWindow(), screenW_, screenH_);
    titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 48);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 16);
    if (titleFont_) { 
        SDL_Color col{200,230,255,255}; 
        SDL_Surface* s = TTF_RenderUTF8_Blended(titleFont_, u8"以物易物", col); 
        if (s) { 
            titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), s); 
            SDL_FreeSurface(s);
        } 
    }

    backButton_ = new Button(); 
    if (backButton_) { 
        backButton_->setRect({20,20,120,36}); 
        backButton_->setText(u8"返回地图"); 
        if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer()); 
        backButton_->setOnClick([this]() { pendingGoMapExplore_ = true; }); 
    }

    checkFurCards();
    
    // 如果有毛皮，开始交易
    if (hasFur_) {
        startTrade(TradeType::RabbitFur); // 默认从兔皮开始
    }
}

void BarterState::onExit(App& app) {}

void BarterState::checkFurCards() {
    hasFur_ = hasFurCards();
    if (!hasFur_) {
        message_ = u8"没有毛皮无法交易，返回地图";
        // 不立即设置pendingGoMapExplore_，等待用户操作
    } else {
        message_ = u8"以物易物界面 - 待实现";
    }
}

bool BarterState::hasFurCards() {
    const auto& library = DeckStore::instance().library();
    
    for (const auto& card : library) {
        // 检查是否是毛皮卡牌
        if (card.id == "tuopi_mao" || card.id == "langpi" || card.id == "jinang_mao") {
            return true;
        }
    }
    
    return false;
}

void BarterState::startTrade(TradeType type) {
    currentTradeType_ = type;
    animActive_ = false;
    animTime_ = 0.0f;
    animatingTradeCard_ = -1;
    animatingPlayerCard_ = -1;
    
    buildPlayerFurCards();
    buildTradeCards();
    layoutCards();
}

void BarterState::buildPlayerFurCards() {
    playerFurCards_.clear();
    const auto& library = DeckStore::instance().library();
    
    std::string targetFurId;
    switch (currentTradeType_) {
        case TradeType::RabbitFur: targetFurId = "tuopi_mao"; break;
        case TradeType::WolfFur: targetFurId = "langpi"; break;
        case TradeType::GoldenFur: targetFurId = "jinang_mao"; break;
        default: return;
    }
    
    for (const auto& card : library) {
        if (card.id == targetFurId) {
            PlayerFurCard furCard;
            furCard.card = card;
            playerFurCards_.push_back(furCard);
        }
    }
}

void BarterState::buildTradeCards() {
    tradeCards_.clear();
    
    std::random_device rd;
    std::mt19937 g(rd());
    
    switch (currentTradeType_) {
        case TradeType::RabbitFur:
            // 兔皮：2行4列共8个牌位，从所有牌库中随机选择
            tradeCards_.resize(8);
            {
                auto allCardIds = CardDB::instance().allIds();
                std::shuffle(allCardIds.begin(), allCardIds.end(), g);
                
                for (int i = 0; i < 8 && i < (int)allCardIds.size(); ++i) {
                    tradeCards_[i].card = CardDB::instance().make(allCardIds[i]);
                }
            }
            break;
            
        case TradeType::WolfFur:
            // 狼皮：2行4列共8个牌位，从所有牌库中随机选择，并添加1~2个随机印记
            tradeCards_.resize(8);
            {
                auto allCardIds = CardDB::instance().allIds();
                std::shuffle(allCardIds.begin(), allCardIds.end(), g);
                
                // 可添加的印记池
                std::vector<std::string> availableMarks = {
                    u8"空袭", u8"水袭", u8"高跳", u8"护主", u8"领袖力量", u8"掘墓人",
                    u8"双重攻击", u8"双向攻击", u8"三向攻击", u8"冲刺能手", u8"蛮力冲撞",
                    u8"生生不息", u8"不死印记", u8"优质祭品",
                    u8"内心之蜂", u8"滋生寄生虫", u8"断尾求生", u8"反伤", u8"死神之触",
                    u8"令人生厌", u8"臭臭", u8"蚁后", u8"一口之量", u8"坚硬之躯",
                    u8"兔窝", u8"筑坝师", u8"检索", u8"磐石之身", u8"道具商"
                };
                
                for (int i = 0; i < 8 && i < (int)allCardIds.size(); ++i) {
                    tradeCards_[i].card = CardDB::instance().make(allCardIds[i]);
                    
                    // 随机添加1~2个印记
                    std::uniform_int_distribution<int> markCount(1, 2);
                    int marksToAdd = markCount(g);
                    std::uniform_int_distribution<int> markIndex(0, (int)availableMarks.size() - 1);
                    
                    int added = 0;
                    int guard = 0;
                    while (added < marksToAdd && guard < 20) {
                        ++guard;
                        std::string mark = availableMarks[markIndex(g)];
                        
                        // 检查是否已存在该印记
                        bool exists = false;
                        for (const auto& existingMark : tradeCards_[i].card.marks) {
                            if (existingMark == mark) {
                                exists = true;
                                break;
                            }
                        }
                        
                        if (!exists) {
                            tradeCards_[i].card.marks.push_back(mark);
                            ++added;
                        }
                    }
                }
            }
            break;
            
        case TradeType::GoldenFur:
            // 金羊皮：只有4张特定卡牌
            tradeCards_.resize(4);
            {
                std::vector<std::string> goldenFurCards = {
                    "maomin",      // 毛民
                    "chuanfen_yinshi", // 穿坟隐士
                    "tengshe_zihuan",  // 螣蛇自环
                    "baina_ou"     // 百衲偶
                };
                
                for (int i = 0; i < 4; ++i) {
                    tradeCards_[i].card = CardDB::instance().make(goldenFurCards[i]);
                }
            }
            break;
            
        default:
            tradeCards_.resize(8);
            break;
    }
}

void BarterState::layoutCards() {
    // 左侧玩家毛皮卡牌布局
    int leftX = 100;  // 向中心移动
    int leftY = 250;  // 向中心移动
    int cardWidth = 150;  // 更大的卡牌
    int cardHeight = 200; // 更大的卡牌
    int cardSpacing = 25;
    
    for (size_t i = 0; i < playerFurCards_.size(); ++i) {
        int row = static_cast<int>(i) / 3; // 每行3张
        int col = static_cast<int>(i) % 3;
        playerFurCards_[i].rect = {
            leftX + col * (cardWidth + cardSpacing),
            leftY + row * (cardHeight + cardSpacing),
            cardWidth,
            cardHeight
        };
    }
    
    // 右侧交易卡牌布局
    int rightX, rightY;
    int cardsPerRow, totalCards;
    
    if (currentTradeType_ == TradeType::GoldenFur) {
        // 金羊皮：1行4列
        totalCards = 4;
        cardsPerRow = 4;
        rightX = screenW_ - 100 - 4 * (cardWidth + cardSpacing);  // 向中心移动
        rightY = 300;  // 稍微向下移动
    } else {
        // 兔皮和狼皮：2行4列
        totalCards = 8;
        cardsPerRow = 4;
        rightX = screenW_ - 100 - 4 * (cardWidth + cardSpacing);  // 向中心移动
        rightY = 250;  // 向中心移动
    }
    
    for (int i = 0; i < totalCards && i < (int)tradeCards_.size(); ++i) {
        int row = i / cardsPerRow;
        int col = i % cardsPerRow;
        tradeCards_[i].rect = {
            rightX + col * (cardWidth + cardSpacing),
            rightY + row * (cardHeight + cardSpacing),
            cardWidth,
            cardHeight
        };
    }
}

void BarterState::performTrade(int tradeCardIndex) {
    if (tradeCardIndex >= 0 && tradeCardIndex < (int)tradeCards_.size() && !playerFurCards_.empty()) {
        // 开始动画
        animActive_ = true;
        animTime_ = 0.0f;
        animatingTradeCard_ = tradeCardIndex;
        animatingPlayerCard_ = 0; // 使用第一张毛皮卡牌
        
        // 添加交易得到的卡牌
        DeckStore::instance().addToLibrary(tradeCards_[tradeCardIndex].card);
        
        // 移除第一张毛皮卡牌
        auto& library = DeckStore::instance().library();
        for (auto it = library.begin(); it != library.end(); ++it) {
            if (it->instanceId == playerFurCards_[0].card.instanceId) {
                library.erase(it);
                break;
            }
        }
    }
}

void BarterState::nextTradeType() {
    // 按兔皮、狼皮、金羊皮的顺序切换
    switch (currentTradeType_) {
        case TradeType::RabbitFur:
            if (hasFurCards()) {
                // 检查是否有狼皮
                const auto& library = DeckStore::instance().library();
                bool hasWolfFur = false;
                for (const auto& card : library) {
                    if (card.id == "langpi") {
                        hasWolfFur = true;
                        break;
                    }
                }
                if (hasWolfFur) {
                    startTrade(TradeType::WolfFur);
                    return;
                }
            }
            // 检查金羊皮
            if (hasFurCards()) {
                const auto& library = DeckStore::instance().library();
                bool hasGoldenFur = false;
                for (const auto& card : library) {
                    if (card.id == "jinang_mao") {
                        hasGoldenFur = true;
                        break;
                    }
                }
                if (hasGoldenFur) {
                    startTrade(TradeType::GoldenFur);
                    return;
                }
            }
            // 没有更多毛皮，返回地图
            pendingGoMapExplore_ = true;
            break;
            
        case TradeType::WolfFur:
            // 检查金羊皮
            if (hasFurCards()) {
                const auto& library = DeckStore::instance().library();
                bool hasGoldenFur = false;
                for (const auto& card : library) {
                    if (card.id == "jinang_mao") {
                        hasGoldenFur = true;
                        break;
                    }
                }
                if (hasGoldenFur) {
                    startTrade(TradeType::GoldenFur);
                    return;
                }
            }
            // 没有更多毛皮，返回地图
            pendingGoMapExplore_ = true;
            break;
            
        case TradeType::GoldenFur:
            // 金羊皮是最后一种，返回地图
            pendingGoMapExplore_ = true;
            break;
            
        default:
            pendingGoMapExplore_ = true;
            break;
    }
}

void BarterState::handleEvent(App& app, const SDL_Event& e) {
    if (backButton_) backButton_->handleEvent(e);
    
    // 如果没有毛皮，任何按键都可以返回地图
    if (!hasFur_ && e.type == SDL_KEYDOWN) {
        pendingGoMapExplore_ = true;
    }
    
    // 交易界面的卡牌点击（只在没有动画时响应）
    if (hasFur_ && !animActive_ && e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;
        
        // 检查右侧交易卡牌点击
        for (size_t i = 0; i < tradeCards_.size(); ++i) {
            const auto& rect = tradeCards_[i].rect;
            if (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h) {
                performTrade((int)i);
                break;
            }
        }
    }
    
    if (pendingGoMapExplore_) {
        if (e.type == SDL_KEYDOWN) {
            app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState())));
        }
    }
}

void BarterState::update(App& app, float dt) {
    // 更新动画
    if (animActive_) {
        animTime_ += dt;
        if (animTime_ >= animDuration_) {
            animActive_ = false;
            animTime_ = 0.0f;
            animatingTradeCard_ = -1;
            animatingPlayerCard_ = -1;
            
            // 动画结束后进入下一种毛皮交易
            nextTradeType();
        }
    }
    
    if (pendingGoMapExplore_) {
        pendingGoMapExplore_ = false;
        app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState())));
    }
}

void BarterState::render(App& app) {
    SDL_SetRenderDrawColor(app.getRenderer(), 18, 22, 32, 255);
    SDL_RenderClear(app.getRenderer());
    
    // 渲染标题
    if (titleTex_) {
        SDL_Rect titleRect = {screenW_/2 - 100, 50, 200, 60};
        SDL_RenderCopy(app.getRenderer(), titleTex_, nullptr, &titleRect);
    }
    
    // 渲染返回按钮
    if (backButton_) backButton_->render(app.getRenderer());
    
    if (hasFur_ && currentTradeType_ != TradeType::None) {
        // 渲染左侧玩家毛皮卡牌
        for (size_t i = 0; i < playerFurCards_.size(); ++i) {
            if (animActive_ && animatingPlayerCard_ == (int)i) {
                // 动画中的毛皮卡牌淡出
                float alpha = 1.0f - (animTime_ / animDuration_);
                alpha = std::max(0.0f, std::min(1.0f, alpha));
                
                SDL_SetRenderDrawBlendMode(app.getRenderer(), SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(app.getRenderer(), 100, 100, 100, (Uint8)(255 * alpha));
                SDL_RenderFillRect(app.getRenderer(), &playerFurCards_[i].rect);
                
                if (smallFont_) {
                    SDL_Color white = {255, 255, 255, (Uint8)(255 * alpha)};
                    SDL_Surface* nameSurface = TTF_RenderUTF8_Blended(smallFont_, playerFurCards_[i].card.name.c_str(), white);
                    if (nameSurface) {
                        SDL_Texture* nameTexture = SDL_CreateTextureFromSurface(app.getRenderer(), nameSurface);
                        if (nameTexture) {
                            SDL_SetTextureAlphaMod(nameTexture, (Uint8)(255 * alpha));
                            SDL_Rect nameRect = {playerFurCards_[i].rect.x, playerFurCards_[i].rect.y - 20, nameSurface->w, nameSurface->h};
                            SDL_RenderCopy(app.getRenderer(), nameTexture, nullptr, &nameRect);
                            SDL_DestroyTexture(nameTexture);
                        }
                        SDL_FreeSurface(nameSurface);
                    }
                }
                SDL_SetRenderDrawBlendMode(app.getRenderer(), SDL_BLENDMODE_NONE);
            } else {
                CardRenderer::renderCard(app, playerFurCards_[i].card, playerFurCards_[i].rect, smallFont_, smallFont_, false);
            }
        }
        
        // 渲染右侧交易卡牌
        for (size_t i = 0; i < tradeCards_.size(); ++i) {
            if (animActive_ && animatingTradeCard_ == (int)i) {
                // 动画中的交易卡牌淡出
                float alpha = 1.0f - (animTime_ / animDuration_);
                alpha = std::max(0.0f, std::min(1.0f, alpha));
                
                SDL_SetRenderDrawBlendMode(app.getRenderer(), SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(app.getRenderer(), 100, 100, 100, (Uint8)(255 * alpha));
                SDL_RenderFillRect(app.getRenderer(), &tradeCards_[i].rect);
                
                if (smallFont_) {
                    SDL_Color white = {255, 255, 255, (Uint8)(255 * alpha)};
                    SDL_Surface* nameSurface = TTF_RenderUTF8_Blended(smallFont_, tradeCards_[i].card.name.c_str(), white);
                    if (nameSurface) {
                        SDL_Texture* nameTexture = SDL_CreateTextureFromSurface(app.getRenderer(), nameSurface);
                        if (nameTexture) {
                            SDL_SetTextureAlphaMod(nameTexture, (Uint8)(255 * alpha));
                            SDL_Rect nameRect = {tradeCards_[i].rect.x, tradeCards_[i].rect.y - 20, nameSurface->w, nameSurface->h};
                            SDL_RenderCopy(app.getRenderer(), nameTexture, nullptr, &nameRect);
                            SDL_DestroyTexture(nameTexture);
                        }
                        SDL_FreeSurface(nameSurface);
                    }
                }
                SDL_SetRenderDrawBlendMode(app.getRenderer(), SDL_BLENDMODE_NONE);
            } else {
                CardRenderer::renderCard(app, tradeCards_[i].card, tradeCards_[i].rect, smallFont_, smallFont_, false);
            }
        }
        
        // 显示选择提示
        if (smallFont_ && !animActive_) {
            SDL_Color white = {255, 255, 255, 255};
            std::string hint;
            
            switch (currentTradeType_) {
                case TradeType::RabbitFur:
                    hint = u8"兔皮交易：点击右侧卡牌进行交易";
                    break;
                case TradeType::WolfFur:
                    hint = u8"狼皮交易：右侧卡牌带有额外印记";
                    break;
                case TradeType::GoldenFur:
                    hint = u8"金羊皮交易：只能交易特定卡牌";
                    break;
                default:
                    hint = u8"点击右侧卡牌进行交易";
                    break;
            }
            
            SDL_Surface* hintSurface = TTF_RenderUTF8_Blended(smallFont_, hint.c_str(), white);
            if (hintSurface) {
                SDL_Texture* hintTexture = SDL_CreateTextureFromSurface(app.getRenderer(), hintSurface);
                if (hintTexture) {
                    SDL_Rect hintRect = {screenW_/2 - hintSurface->w/2, 150, hintSurface->w, hintSurface->h};
                    SDL_RenderCopy(app.getRenderer(), hintTexture, nullptr, &hintRect);
                    SDL_DestroyTexture(hintTexture);
                }
                SDL_FreeSurface(hintSurface);
            }
        }
    } else {
        // 显示消息（无毛皮情况）
        SDL_Color white = {255, 255, 255, 255};
        if (smallFont_) {
            SDL_Surface* textSurface = TTF_RenderUTF8_Blended(smallFont_, message_.c_str(), white);
            if (textSurface) {
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(app.getRenderer(), textSurface);
                if (textTexture) {
                    SDL_Rect textRect = {screenW_/2 - textSurface->w/2, screenH_/2, textSurface->w, textSurface->h};
                    SDL_RenderCopy(app.getRenderer(), textTexture, nullptr, &textRect);
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
        }
        
        if (pendingGoMapExplore_) {
            // 显示返回提示
            std::string returnMsg = u8"按任意键返回地图";
            if (smallFont_) {
                SDL_Surface* returnSurface = TTF_RenderUTF8_Blended(smallFont_, returnMsg.c_str(), white);
                if (returnSurface) {
                    SDL_Texture* returnTexture = SDL_CreateTextureFromSurface(app.getRenderer(), returnSurface);
                    if (returnTexture) {
                        SDL_Rect returnRect = {screenW_/2 - returnSurface->w/2, screenH_/2 + 50, returnSurface->w, returnSurface->h};
                        SDL_RenderCopy(app.getRenderer(), returnTexture, nullptr, &returnRect);
                        SDL_DestroyTexture(returnTexture);
                    }
                    SDL_FreeSurface(returnSurface);
                }
            }
        }
    }
}
