#include "InkWorkshopState.h"
#include "../core/App.h"
#include "../core/Deck.h"
#include "../core/Cards.h"
#include "MapExploreState.h"
#include "../core/WenMaiStore.h"
#include "../ui/Button.h"
#include "../ui/CardRenderer.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <algorithm>
#include <algorithm>

InkWorkshopState::InkWorkshopState(int mapLayer) : mapLayer_(mapLayer) {
    backButton_ = new Button();
}

InkWorkshopState::~InkWorkshopState() {
    if (backButton_) {
        delete backButton_;
    }
    if (titleFont_) TTF_CloseFont(titleFont_);
    if (smallFont_) TTF_CloseFont(smallFont_);
    if (cardNameFont_) TTF_CloseFont(cardNameFont_);
    if (cardStatFont_) TTF_CloseFont(cardStatFont_);
}

void InkWorkshopState::onEnter(App& app) {
    SDL_GetWindowSize(app.getWindow(), &screenW_, &screenH_);
    
    // 加载字体
    titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 32);
    smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 16);
    cardNameFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 14);
    cardStatFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 12);
    
    if (!titleFont_ || !smallFont_ || !cardNameFont_ || !cardStatFont_) {
        SDL_Log("TTF_OpenFont failed: %s", TTF_GetError());
    }
    
    // 根据地图层级设置毛皮价格
    if (mapLayer_ == 3) {
        // 第三层：兔皮2、狼皮6、金羊皮11
        rabbitCost_ = 2;
        wolfCost_ = 6;
        goldSheepCost_ = 11;
    } else {
        // 其他层级：默认价格
        rabbitCost_ = 2;
        wolfCost_ = 4;
        goldSheepCost_ = 7;
    }
    
    // 设置返回按钮
    if (backButton_) {
        SDL_Rect backButtonRect{20, 20, 80, 40};
        backButton_->setRect(backButtonRect);
        backButton_->setText(u8"返回地图");
        if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer());
        backButton_->setOnClick([&app]() {
            app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState())));
        });
    }
    
    // 初始化牌位位置
    int slotSize = 150;
    int slotGap = 30;
    int totalWidth = 3 * slotSize + 2 * slotGap;
    int startX = (screenW_ - totalWidth) / 2;
    int slotY = 120;
    
    rabbitSlotRect_ = {startX, slotY, slotSize, static_cast<int>(slotSize * 1.4f)};
    wolfSlotRect_ = {startX + slotSize + slotGap, slotY, slotSize, static_cast<int>(slotSize * 1.4f)};
    goldSheepSlotRect_ = {startX + 2 * (slotSize + slotGap), slotY, slotSize, static_cast<int>(slotSize * 1.4f)};
    
    // 道具位置（右上角）
    toolRect_ = {screenW_ - 120, 20, 100, 80};
    
    // 初始化可获得的毛皮（固定有一张兔皮）
    availableSkins_.clear();
    Card rabbitSkin;
    rabbitSkin.id = "rabbit_skin";
    rabbitSkin.name = u8"兔皮";
    rabbitSkin.category = u8"毛皮";
    rabbitSkin.health = 1;
    rabbitSkin.attack = 0;
    availableSkins_.push_back(rabbitSkin);
    
    // 布局毛皮
    layoutSkins();
    
    // 重置状态
    hoveredSkinIndex_ = -1;
    hoverTime_ = 0.0f;
    isAnimating_ = false;
    statusMessage_ = "";
    messageTime_ = 0.0f;
}

void InkWorkshopState::onExit(App& app) {
    if (titleFont_) {
        TTF_CloseFont(titleFont_);
        titleFont_ = nullptr;
    }
    if (smallFont_) {
        TTF_CloseFont(smallFont_);
        smallFont_ = nullptr;
    }
}

void InkWorkshopState::update(App& app, float deltaTime) {
    hoverTime_ += deltaTime;
    
    // 处理消息显示时间
    if (messageTime_ > 0.0f) {
        messageTime_ -= deltaTime;
        if (messageTime_ <= 0.0f) {
            statusMessage_ = "";
        }
    }
    
    // 处理动画
    if (isAnimating_) {
        animTime_ += deltaTime;
        if (animTime_ >= animDuration_) {
            isAnimating_ = false;
            animTime_ = 0.0f;
            // 动画完成后返回地图
            app.setState(std::make_unique<MapExploreState>());
        }
    }
}

void InkWorkshopState::render(App& app) {
    SDL_Renderer* r = app.getRenderer();
    
    // 清屏
    SDL_SetRenderDrawColor(r, 30, 25, 20, 255);
    SDL_RenderClear(r);
    
    // 渲染返回按钮
    if (backButton_) backButton_->render(r);
    
    // 渲染标题
    renderTitle(app);
    
    // 渲染文脉显示
    renderWenMai(app);
    
    // 渲染牌位
    renderSkinSlots(app);
    
    // 渲染道具
    renderTool(app);
    
    // 渲染可获得的毛皮
    renderAvailableSkins(app);
    
    // 渲染状态消息
    if (!statusMessage_.empty() && smallFont_) {
        SDL_Color messageColor{255, 200, 100, 255};
        SDL_Surface* messageSurface = TTF_RenderUTF8_Blended(smallFont_, statusMessage_.c_str(), messageColor);
        if (messageSurface) {
            SDL_Texture* messageTexture = SDL_CreateTextureFromSurface(r, messageSurface);
            SDL_Rect messageRect{(screenW_ - messageSurface->w) / 2, screenH_ - 100, messageSurface->w, messageSurface->h};
            SDL_RenderCopy(r, messageTexture, nullptr, &messageRect);
            SDL_DestroyTexture(messageTexture);
            SDL_FreeSurface(messageSurface);
        }
    }
}

void InkWorkshopState::handleEvent(App& app, const SDL_Event& event) {
    // 处理返回按钮事件
    if (backButton_) backButton_->handleEvent(event);
    
    if (event.type == SDL_MOUSEMOTION) {
        int mx = event.motion.x;
        int my = event.motion.y;
        
        // 检查毛皮悬停
        hoveredSkinIndex_ = -1;
        for (int i = 0; i < (int)skinRects_.size(); ++i) {
            if (mx >= skinRects_[i].x && mx <= skinRects_[i].x + skinRects_[i].w &&
                my >= skinRects_[i].y && my <= skinRects_[i].y + skinRects_[i].h) {
                hoveredSkinIndex_ = i;
                break;
            }
        }
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mx = event.button.x;
        int my = event.button.y;
        
        // 检查牌位点击
        if (mx >= rabbitSlotRect_.x && mx <= rabbitSlotRect_.x + rabbitSlotRect_.w &&
            my >= rabbitSlotRect_.y && my <= rabbitSlotRect_.y + rabbitSlotRect_.h) {
            tryGetSkin(0); // 兔皮
        }
        else if (mx >= wolfSlotRect_.x && mx <= wolfSlotRect_.x + wolfSlotRect_.w &&
                 my >= wolfSlotRect_.y && my <= wolfSlotRect_.y + wolfSlotRect_.h) {
            tryGetSkin(1); // 狼皮
        }
        else if (mx >= goldSheepSlotRect_.x && mx <= goldSheepSlotRect_.x + goldSheepSlotRect_.w &&
                 my >= goldSheepSlotRect_.y && my <= goldSheepSlotRect_.y + goldSheepSlotRect_.h) {
            tryGetSkin(2); // 金羊皮
        }
        // 检查道具点击
        else if (mx >= toolRect_.x && mx <= toolRect_.x + toolRect_.w &&
                 my >= toolRect_.y && my <= toolRect_.y + toolRect_.h) {
            tryGetTool();
        }
        // 检查毛皮点击
        else if (hoveredSkinIndex_ >= 0 && hoveredSkinIndex_ < (int)availableSkins_.size()) {
            collectAllSkins();
        }
    }
    else if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_ESCAPE) {
            app.setState(std::make_unique<MapExploreState>());
        }
    }
}

void InkWorkshopState::renderTitle(App& app) {
    if (titleFont_) {
        SDL_Color titleColor{255, 255, 200, 255};
        SDL_Surface* titleSurface = TTF_RenderUTF8_Blended(titleFont_, u8"墨坊", titleColor);
        if (titleSurface) {
            SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(app.getRenderer(), titleSurface);
            SDL_Rect titleRect{(screenW_ - titleSurface->w) / 2, 50, titleSurface->w, titleSurface->h};
            SDL_RenderCopy(app.getRenderer(), titleTexture, nullptr, &titleRect);
            SDL_DestroyTexture(titleTexture);
            SDL_FreeSurface(titleSurface);
        }
    }
}

void InkWorkshopState::renderWenMai(App& app) {
    // 获取当前文脉数量（这里需要实现文脉系统）
    int wenMaiCount = WenMaiStore::instance().get();
    
    if (smallFont_) {
        SDL_Color wenMaiColor{200, 200, 255, 255};
        std::string wenMaiText = u8"文脉: " + std::to_string(wenMaiCount);
        SDL_Surface* wenMaiSurface = TTF_RenderUTF8_Blended(smallFont_, wenMaiText.c_str(), wenMaiColor);
        if (wenMaiSurface) {
            SDL_Texture* wenMaiTexture = SDL_CreateTextureFromSurface(app.getRenderer(), wenMaiSurface);
            SDL_Rect wenMaiRect{20, (screenH_ - wenMaiSurface->h) / 2, wenMaiSurface->w, wenMaiSurface->h};
            SDL_RenderCopy(app.getRenderer(), wenMaiTexture, nullptr, &wenMaiRect);
            SDL_DestroyTexture(wenMaiTexture);
            SDL_FreeSurface(wenMaiSurface);
        }
    }
}

void InkWorkshopState::renderSkinSlots(App& app) {
    SDL_Renderer* r = app.getRenderer();
    
    // 渲染兔皮卡牌
    Card rabbitCard = CardDB::instance().make("tuopi_mao");
    CardRenderer::renderCard(app, rabbitCard, rabbitSlotRect_, cardNameFont_, cardStatFont_, false);
    
    // 渲染狼皮卡牌
    Card wolfCard = CardDB::instance().make("langpi");
    CardRenderer::renderCard(app, wolfCard, wolfSlotRect_, cardNameFont_, cardStatFont_, false);
    
    // 渲染金羊皮卡牌（金色背景）
    Card goldSheepCard = CardDB::instance().make("jinang_mao");
    renderGoldenCard(app, goldSheepCard, goldSheepSlotRect_);
    
    // 渲染牌位标签
    if (smallFont_) {
        SDL_Color labelColor{255, 255, 255, 255};
        
        // 兔皮标签
        SDL_Surface* rabbitLabel = TTF_RenderUTF8_Blended(smallFont_, u8"兔皮 (2文脉)", labelColor);
        if (rabbitLabel) {
            SDL_Texture* rabbitTexture = SDL_CreateTextureFromSurface(r, rabbitLabel);
            SDL_Rect rabbitLabelRect{rabbitSlotRect_.x + (rabbitSlotRect_.w - rabbitLabel->w) / 2, 
                                    rabbitSlotRect_.y + rabbitSlotRect_.h + 10, rabbitLabel->w, rabbitLabel->h};
            SDL_RenderCopy(r, rabbitTexture, nullptr, &rabbitLabelRect);
            SDL_DestroyTexture(rabbitTexture);
            SDL_FreeSurface(rabbitLabel);
        }
        
        // 狼皮标签
        SDL_Surface* wolfLabel = TTF_RenderUTF8_Blended(smallFont_, u8"狼皮 (4文脉)", labelColor);
        if (wolfLabel) {
            SDL_Texture* wolfTexture = SDL_CreateTextureFromSurface(r, wolfLabel);
            SDL_Rect wolfLabelRect{wolfSlotRect_.x + (wolfSlotRect_.w - wolfLabel->w) / 2, 
                                  wolfSlotRect_.y + wolfSlotRect_.h + 10, wolfLabel->w, wolfLabel->h};
            SDL_RenderCopy(r, wolfTexture, nullptr, &wolfLabelRect);
            SDL_DestroyTexture(wolfTexture);
            SDL_FreeSurface(wolfLabel);
        }
        
        // 金羊皮标签
        SDL_Surface* goldSheepLabel = TTF_RenderUTF8_Blended(smallFont_, u8"金羊皮 (7文脉)", labelColor);
        if (goldSheepLabel) {
            SDL_Texture* goldSheepTexture = SDL_CreateTextureFromSurface(r, goldSheepLabel);
            SDL_Rect goldSheepLabelRect{goldSheepSlotRect_.x + (goldSheepSlotRect_.w - goldSheepLabel->w) / 2, 
                                       goldSheepSlotRect_.y + goldSheepSlotRect_.h + 10, goldSheepLabel->w, goldSheepLabel->h};
            SDL_RenderCopy(r, goldSheepTexture, nullptr, &goldSheepLabelRect);
            SDL_DestroyTexture(goldSheepTexture);
            SDL_FreeSurface(goldSheepLabel);
        }
    }
}

void InkWorkshopState::renderTool(App& app) {
    SDL_Renderer* r = app.getRenderer();
    
    // 渲染道具背景
    SDL_SetRenderDrawColor(r, 80, 60, 40, 255);
    SDL_RenderFillRect(r, &toolRect_);
    SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
    SDL_RenderDrawRect(r, &toolRect_);
    
    // 渲染道具标签
    if (smallFont_) {
        SDL_Color labelColor{255, 255, 255, 255};
        SDL_Surface* toolLabel = TTF_RenderUTF8_Blended(smallFont_, u8"吞墨毫\n(7文脉)", labelColor);
        if (toolLabel) {
            SDL_Texture* toolTexture = SDL_CreateTextureFromSurface(r, toolLabel);
            SDL_Rect toolLabelRect{toolRect_.x + (toolRect_.w - toolLabel->w) / 2, 
                                  toolRect_.y + (toolRect_.h - toolLabel->h) / 2, toolLabel->w, toolLabel->h};
            SDL_RenderCopy(r, toolTexture, nullptr, &toolLabelRect);
            SDL_DestroyTexture(toolTexture);
            SDL_FreeSurface(toolLabel);
        }
    }
}

void InkWorkshopState::renderAvailableSkins(App& app) {
    SDL_Renderer* r = app.getRenderer();
    
    // 渲染毛皮区域背景
    if (!availableSkins_.empty()) {
        SDL_SetRenderDrawColor(r, 60, 55, 50, 100);
        SDL_Rect skinArea{0, screenH_ - 200, screenW_, 200};
        SDL_RenderFillRect(r, &skinArea);
        
        // 渲染毛皮数量提示
        if (smallFont_) {
            SDL_Color countColor{255, 255, 255, 255};
            std::string countText = u8"可获得的毛皮: " + std::to_string(availableSkins_.size()) + u8" (点击获取)";
            SDL_Surface* countSurface = TTF_RenderUTF8_Blended(smallFont_, countText.c_str(), countColor);
            if (countSurface) {
                SDL_Texture* countTexture = SDL_CreateTextureFromSurface(r, countSurface);
                SDL_Rect countRect{20, screenH_ - 180, countSurface->w, countSurface->h};
                SDL_RenderCopy(r, countTexture, nullptr, &countRect);
                SDL_DestroyTexture(countTexture);
                SDL_FreeSurface(countSurface);
            }
        }
    }
    
    // 渲染毛皮
    for (int i = 0; i < (int)availableSkins_.size(); ++i) {
        const Card& skin = availableSkins_[i];
        const SDL_Rect& rect = skinRects_[i];
        
        // 悬停动画
        SDL_Rect renderRect = rect;
        if (i == hoveredSkinIndex_) {
            float hoverOffset = sinf(hoverTime_ * 5.0f) * 5.0f;
            renderRect.y += (int)hoverOffset;
        }
        
        // 渲染毛皮卡牌
        CardRenderer::renderCard(app, skin, renderRect, smallFont_, smallFont_, false);
    }
}

void InkWorkshopState::tryGetSkin(int skinType) {
    // 获取当前文脉数量（这里需要实现文脉系统）
    int wenMaiCount = WenMaiStore::instance().get();
    
    int cost = 0;
    std::string skinName;
    std::string skinId;
    
    switch (skinType) {
        case 0: // 兔皮
            cost = rabbitCost_;
            skinName = u8"兔皮";
            skinId = "tuopi_mao";
            break;
        case 1: // 狼皮
            cost = wolfCost_;
            skinName = u8"狼皮";
            skinId = "langpi";
            break;
        case 2: // 金羊皮
            cost = goldSheepCost_;
            skinName = u8"金羊皮";
            skinId = "jinang_mao";
            break;
    }
    
    if (wenMaiCount >= cost) {
        // 文脉足够，添加毛皮到收集区
        Card newSkin = CardDB::instance().make(skinId);
        if (newSkin.id.empty()) {
            // 如果找不到卡牌，使用默认值
            newSkin.id = skinId;
            newSkin.name = skinName;
            newSkin.category = u8"其他";
            newSkin.health = skinType + 1; // 兔皮1血，狼皮2血，金羊皮3血
            newSkin.attack = skinType;     // 兔皮0攻，狼皮1攻，金羊皮2攻
        }
        
        availableSkins_.push_back(newSkin);
        layoutSkins();
        
        // 扣除文脉
        WenMaiStore::instance().spend(cost);
        
        statusMessage_ = u8"获得 " + skinName + u8"！";
        messageTime_ = messageDuration_;
    } else {
        statusMessage_ = u8"文脉不足！需要 " + std::to_string(cost) + u8" 点文脉";
        messageTime_ = messageDuration_;
    }
}

void InkWorkshopState::tryGetTool() {
    // 获取当前文脉数量
    int wenMaiCount = WenMaiStore::instance().get();
    
    if (wenMaiCount >= toolCost_) {
        // 检查道具栏是否已满
        if (playerItems_.size() < MAX_ITEMS) {
            // 添加吞墨毫到道具栏
            addItem("tunmohao", 1);
            // 扣除文脉
            WenMaiStore::instance().spend(toolCost_);
            
            statusMessage_ = u8"获得 吞墨毫！";
            messageTime_ = messageDuration_;
        } else {
            statusMessage_ = u8"道具栏已满！";
            messageTime_ = messageDuration_;
        }
    } else {
        statusMessage_ = u8"文脉不足！需要 " + std::to_string(toolCost_) + u8" 点文脉";
        messageTime_ = messageDuration_;
    }
}

void InkWorkshopState::collectAllSkins() {
    if (availableSkins_.empty()) return;
    
    // 将所有毛皮添加到全局牌库
    auto& store = DeckStore::instance();
    for (const auto& skin : availableSkins_) {
        store.addToLibrary(skin);
    }
    
    // 清空可获得的毛皮
    availableSkins_.clear();
    layoutSkins();
    
    // 开始动画
    isAnimating_ = true;
    animTime_ = 0.0f;
    animDuration_ = 1.0f;
    
    statusMessage_ = u8"已获得所有毛皮！";
    messageTime_ = messageDuration_;
}

void InkWorkshopState::layoutSkins() {
    skinRects_.clear();
    
    if (availableSkins_.empty()) return;
    
    int cardWidth = 120;
    int cardHeight = 168;
    int cardGap = 10;
    int startY = screenH_ - cardHeight - 50;
    
    // 计算总宽度
    int totalWidth = (int)availableSkins_.size() * cardWidth + ((int)availableSkins_.size() - 1) * cardGap;
    int startX = (screenW_ - totalWidth) / 2;
    
    for (int i = 0; i < (int)availableSkins_.size(); ++i) {
        SDL_Rect rect{startX + i * (cardWidth + cardGap), startY, cardWidth, cardHeight};
        skinRects_.push_back(rect);
    }
}

// 道具系统方法实现
void InkWorkshopState::addItem(const std::string& itemId, int count) {
    // 检查道具栏是否已满
    if (playerItems_.size() >= MAX_ITEMS) {
        return;
    }
    
    // 查找是否已存在该道具
    for (auto& item : playerItems_) {
        if (item.id == itemId) {
            item.count += count;
            return;
        }
    }
    
    // 创建新道具
    Item newItem;
    if (itemId == "tunmohao") {
        newItem = Item("tunmohao", u8"吞墨毫", u8"使用后摧毁敌人第二行任意一张牌，摧毁后手牌获得一张狼皮", count);
    } else {
        newItem = Item(itemId, itemId, "", count);
    }
    
    playerItems_.push_back(newItem);
}

bool InkWorkshopState::removeItem(const std::string& itemId, int count) {
    for (auto it = playerItems_.begin(); it != playerItems_.end(); ++it) {
        if (it->id == itemId) {
            if (it->count >= count) {
                it->count -= count;
                if (it->count <= 0) {
                    playerItems_.erase(it);
                }
                return true;
            }
            return false;
        }
    }
    return false;
}

bool InkWorkshopState::hasItem(const std::string& itemId) const {
    for (const auto& item : playerItems_) {
        if (item.id == itemId && item.count > 0) {
            return true;
        }
    }
    return false;
}

int InkWorkshopState::getItemCount(const std::string& itemId) const {
    for (const auto& item : playerItems_) {
        if (item.id == itemId) {
            return item.count;
        }
    }
    return 0;
}

void InkWorkshopState::renderGoldenCard(App& app, const Card& card, const SDL_Rect& rect) {
    SDL_Renderer* r = app.getRenderer();

    // 淡金色背景
    SDL_SetRenderDrawColor(r, 255, 235, 150, 255);  // 淡金色
    SDL_RenderFillRect(r, &rect);
    SDL_SetRenderDrawColor(r, 255, 245, 180, 255);  // 淡金色边框
    SDL_RenderDrawRect(r, &rect);
    
    // 内边框增强效果
    SDL_SetRenderDrawColor(r, 255, 240, 160, 255);
    SDL_Rect innerRect = {rect.x + 2, rect.y + 2, rect.w - 4, rect.h - 4};
    SDL_RenderDrawRect(r, &innerRect);

    // 装饰性圆点
    SDL_SetRenderDrawColor(r, 255, 255, 255, 150);
    SDL_Rect dots[4] = { {rect.x+4,rect.y+4,2,2},{rect.x+rect.w-6,rect.y+4,2,2},{rect.x+4,rect.y+rect.h-6,2,2},{rect.x+rect.w-6,rect.y+rect.h-6,2,2} };
    for (const auto& d : dots) SDL_RenderFillRect(r, &d);

    // 卡牌名称
    if (cardNameFont_) {
        SDL_Color nameCol{160, 100, 60, 255};  // 淡棕色文字
        SDL_Surface* s = TTF_RenderUTF8_Blended(cardNameFont_, card.name.c_str(), nameCol);
        if (s) {
            SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
            int desiredNameH = SDL_max(12, (int)(rect.h * 0.16f));
            float scaleN = (float)desiredNameH / (float)s->h;
            int scaledW = (int)(s->w * scaleN);
            SDL_Rect ndst{ rect.x + (rect.w - scaledW)/2, rect.y + (int)(rect.h*0.04f), scaledW, desiredNameH };
            SDL_RenderCopy(r, t, nullptr, &ndst);
            SDL_DestroyTexture(t);
            
            // 名称下方装饰线
            int lineBase = ndst.y + ndst.h + SDL_max(2, (int)(rect.h * 0.015f));
            int thickness = SDL_max(1, (int)(rect.h * 0.007f));
            SDL_SetRenderDrawColor(r, 160, 100, 60, 180);
            for (int i = 0; i < thickness; ++i) SDL_RenderDrawLine(r, rect.x + 6, lineBase + i, rect.x + rect.w - 6, lineBase + i);
            
            SDL_FreeSurface(s);
        }
    }

    // 攻击力和生命值
    if (cardStatFont_) {
        SDL_Color statCol{160, 100, 60, 255};  // 淡棕色文字
        int desiredStatH = SDL_max(12, (int)(rect.h * 0.18f));
        int margin = SDL_max(6, (int)(rect.h * 0.035f));
        
        // 攻击力
        std::string attackText = std::to_string(card.attack);
        SDL_Surface* sa = TTF_RenderUTF8_Blended(cardStatFont_, attackText.c_str(), statCol);
        if (sa) {
            SDL_Texture* ta = SDL_CreateTextureFromSurface(r, sa);
            float scaleA = (float)desiredStatH / (float)sa->h;
            int aW = (int)(sa->w * scaleA);
            SDL_Rect adst{ rect.x + margin, rect.y + rect.h - desiredStatH - margin, aW, desiredStatH };
            SDL_RenderCopy(r, ta, nullptr, &adst);
            SDL_DestroyTexture(ta);
            
            // 攻击力装饰线
            SDL_SetRenderDrawColor(r, 160, 100, 60, 100);
            int swordY = adst.y + adst.h + SDL_max(1, (int)(rect.h * 0.006f));
            int swordW = SDL_max(adst.w, (int)(rect.w * 0.22f));
            int swordX = adst.x;
            SDL_RenderDrawLine(r, swordX, swordY, swordX + swordW, swordY);
            SDL_RenderDrawLine(r, swordX + swordW/3, swordY-2, swordX + swordW/3, swordY+2);
            SDL_RenderDrawLine(r, swordX + swordW, swordY, swordX + swordW - 4, swordY - 3);
            SDL_RenderDrawLine(r, swordX + swordW, swordY, swordX + swordW - 4, swordY + 3);
            
            SDL_FreeSurface(sa);
        }
        
        // 生命值
        std::string healthText = std::to_string(card.health);
        SDL_Color hpCol{160, 100, 60, 255};  // 淡棕色文字
        SDL_Surface* sh = TTF_RenderUTF8_Blended(cardStatFont_, healthText.c_str(), hpCol);
        if (sh) {
            SDL_Texture* th = SDL_CreateTextureFromSurface(r, sh);
            float scaleH = (float)desiredStatH / (float)sh->h;
            int hW = (int)(sh->w * scaleH);
            SDL_Rect hdst{ rect.x + rect.w - margin - hW, rect.y + rect.h - desiredStatH - margin, hW, desiredStatH };
            SDL_RenderCopy(r, th, nullptr, &hdst);
            SDL_DestroyTexture(th);
            
            // 生命值装饰线
            SDL_SetRenderDrawColor(r, 160, 100, 60, 100);
            int heartY = hdst.y + hdst.h + SDL_max(1, (int)(rect.h * 0.006f));
            int heartW = SDL_max(hdst.w, (int)(rect.w * 0.22f));
            int heartX = hdst.x + hdst.w - heartW;
            SDL_RenderDrawLine(r, heartX, heartY, heartX + heartW, heartY);
            SDL_RenderDrawLine(r, heartX + heartW/3, heartY-2, heartX + heartW/3, heartY+2);
            SDL_RenderDrawLine(r, heartX + heartW*2/3, heartY-2, heartX + heartW*2/3, heartY+2);
            
            SDL_FreeSurface(sh);
        }
    }
}
