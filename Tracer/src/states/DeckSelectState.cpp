#include "DeckSelectState.h"
#include "MapExploreState.h"
#include "../core/App.h"
#include "../core/Cards.h"
#include "../ui/CardRenderer.h"
#include <SDL.h>
#include <SDL_ttf.h>

DeckSelectState::DeckSelectState() = default;

DeckSelectState::~DeckSelectState() {
    if (titleTex_) SDL_DestroyTexture(titleTex_);
    if (titleFont_) TTF_CloseFont(titleFont_);
    if (smallFont_) TTF_CloseFont(smallFont_);
    if (deckNameFont_) TTF_CloseFont(deckNameFont_);
    if (deckDescFont_) TTF_CloseFont(deckDescFont_);
}

void DeckSelectState::onEnter(App& app) {
    // 获取屏幕尺寸
    int w, h;
    SDL_GetWindowSize(app.getWindow(), &w, &h);
    screenW_ = w;
    screenH_ = h;

    // 加载字体
    titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 48);
    smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 16);
    deckNameFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 40);  // 牌组名称字体（更大）
    deckDescFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 20);  // 牌组描述字体（更大）
    if (!titleFont_ || !smallFont_ || !deckNameFont_ || !deckDescFont_) {
        SDL_Log("TTF_OpenFont failed: %s", TTF_GetError());
    }
    else {
        SDL_Color titleCol{ 200, 230, 255, 255 };
        SDL_Surface* ts = TTF_RenderUTF8_Blended(titleFont_, u8"选择初始牌组", titleCol);
        if (ts) {
            titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), ts);
            titleW_ = ts->w;
            titleH_ = ts->h;
            SDL_FreeSurface(ts);
        }
    }

    // 初始化六大初始牌组
    initializeDecks();
    layoutDecks();
}

void DeckSelectState::onExit(App& app) {}

void DeckSelectState::handleEvent(App& app, const SDL_Event& e) {
    if (viewingDeck_) {
        // 查看牌组内容时的处理
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_s) {
            // S键记住选择的牌组并进入地图
            if (selectedDeckIndex_ >= 0 && selectedDeckIndex_ < static_cast<int>(initialDecks_.size())) {
                App::setSelectedInitialDeck(initialDecks_[selectedDeckIndex_].id);
            }
            pendingGoMapExplore_ = true;
            return;
        }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
            // 右键返回选择界面
            viewingDeck_ = false;
            selectedDeckIndex_ = -1;
            return;
        }
    } else {
        // 选择牌组时的处理
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int mx = e.button.x, my = e.button.y;
            
            // 检查点击了哪个牌组
            for (size_t i = 0; i < initialDecks_.size(); ++i) {
                const SDL_Rect& rect = initialDecks_[i].rect;
                if (mx >= rect.x && mx <= rect.x + rect.w && 
                    my >= rect.y && my <= rect.y + rect.h) {
                    selectedDeckIndex_ = static_cast<int>(i);
                    viewingDeck_ = true;
                    break;
                }
            }
        }
        else if (e.type == SDL_MOUSEMOTION) {
            // 处理鼠标悬停
            int mx = e.motion.x, my = e.motion.y;
            hoveredDeckIndex_ = -1;
            
            for (size_t i = 0; i < initialDecks_.size(); ++i) {
                const SDL_Rect& rect = initialDecks_[i].rect;
                if (mx >= rect.x && mx <= rect.x + rect.w && 
                    my >= rect.y && my <= rect.y + rect.h) {
                    hoveredDeckIndex_ = static_cast<int>(i);
                    break;
                }
            }
        }
    }
}

void DeckSelectState::update(App& app, float dt) {
    if (pendingGoMapExplore_) {
        pendingGoMapExplore_ = false;
        app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState())));
    }
}

void DeckSelectState::render(App& app) {
    SDL_Renderer* r = app.getRenderer();
    SDL_SetRenderDrawColor(r, 18, 22, 32, 255);
    SDL_RenderClear(r);

    if (viewingDeck_) {
        // 渲染牌组内容查看界面
        renderDeckView(app);
    } else {
        // 渲染牌组选择界面
        // 渲染标题
        if (titleTex_) {
            SDL_Rect titleRect{ (screenW_ - titleW_) / 2, 20, titleW_, titleH_ };
            SDL_RenderCopy(r, titleTex_, nullptr, &titleRect);
        }

        // 渲染六大初始牌组
        for (size_t i = 0; i < initialDecks_.size(); ++i) {
            const InitialDeck& deck = initialDecks_[i];
            const SDL_Rect& rect = deck.rect;
            bool isHovered = (hoveredDeckIndex_ == static_cast<int>(i));
            
            // 绘制牌组背景（悬停时高亮）
            if (isHovered) {
                SDL_SetRenderDrawColor(r, 80, 100, 120, 220);  // 悬停时更亮的背景
            } else {
                SDL_SetRenderDrawColor(r, 60, 70, 80, 200);
            }
            SDL_RenderFillRect(r, &rect);
            
            // 绘制边框（悬停时更亮的边框）
            if (isHovered) {
                SDL_SetRenderDrawColor(r, 160, 180, 200, 255);  // 悬停时更亮的边框
            } else {
                SDL_SetRenderDrawColor(r, 120, 140, 160, 255);
            }
            SDL_RenderDrawRect(r, &rect);
            
            // 绘制牌组名称（居中显示，占满牌位宽度）
            if (deckNameFont_) {
                SDL_Color textCol = isHovered ? SDL_Color{ 255, 255, 255, 255 } : SDL_Color{ 220, 230, 240, 255 };
                SDL_Surface* nameSurf = TTF_RenderUTF8_Blended(deckNameFont_, deck.name.c_str(), textCol);
                if (nameSurf) {
                    SDL_Texture* nameTex = SDL_CreateTextureFromSurface(r, nameSurf);
                    // 居中显示牌组名称
                    SDL_Rect nameRect{ rect.x + (rect.w - nameSurf->w) / 2, rect.y + 15, nameSurf->w, nameSurf->h };
                    SDL_RenderCopy(r, nameTex, nullptr, &nameRect);
                    SDL_DestroyTexture(nameTex);
                    SDL_FreeSurface(nameSurf);
                }
            }
            
            // 绘制牌组描述（每张牌的名字占一行）
            if (deckDescFont_) {
                SDL_Color descCol = isHovered ? SDL_Color{ 200, 210, 220, 255 } : SDL_Color{ 180, 190, 200, 255 };
                SDL_Surface* descSurf = TTF_RenderUTF8_Blended_Wrapped(deckDescFont_, deck.description.c_str(), descCol, rect.w - 20);
                if (descSurf) {
                    SDL_Texture* descTex = SDL_CreateTextureFromSurface(r, descSurf);
                    // 居中显示描述文本
                    SDL_Rect descRect{ rect.x + (rect.w - descSurf->w) / 2, rect.y + 60, descSurf->w, descSurf->h };
                    SDL_RenderCopy(r, descTex, nullptr, &descRect);
                    SDL_DestroyTexture(descTex);
                    SDL_FreeSurface(descSurf);
                }
            }
        }
    }
}

void DeckSelectState::initializeDecks() {
    initialDecks_.clear();
    
    // 默认牌组
    InitialDeck deck1;
    deck1.id = "default_deck";
    deck1.name = u8"默认牌组";
    deck1.description = u8"雪尾鼬生\n碧蟾\n朔漠苍狼";
    deck1.cardIds = {"xuewei_yousheng", "bichan", "shuomuo_canglang"};
    initialDecks_.push_back(deck1);
    
    // 玄牧牌组
    InitialDeck deck2;
    deck2.id = "xuanmu_deck";
    deck2.name = u8"玄牧牌组";
    deck2.description = u8"玄牧\n千峰驼鹿\n穿坟鼹子";
    deck2.cardIds = {"xuanmu", "qianfeng_tuolu", "chuanfen_yanzi"};
    initialDecks_.push_back(deck2);
    
    // 蚂蚁牌组
    InitialDeck deck3;
    deck3.id = "ant_deck";
    deck3.name = u8"蚂蚁牌组";
    deck3.description = u8"典诰蚁后\n驿飞蚁\n黄鼬臭尉";
    deck3.cardIds = {"diangao_yihou", "yifei_yi", "huangyou_chouwei"};
    initialDecks_.push_back(deck3);
    
    // 刀笔吏牌组
    InitialDeck deck4;
    deck4.id = "daobi_deck";
    deck4.name = u8"刀笔吏牌组";
    deck4.description = u8"刀笔吏\n卷册螟蛉\n卷册螟蛉";
    deck4.cardIds = {"daobi_li", "juance_mingling", "juance_mingling"};
    initialDecks_.push_back(deck4);
    
    // 魂骨牌组
    InitialDeck deck5;
    deck5.id = "bone_deck";
    deck5.name = u8"魂骨牌组";
    deck5.description = u8"浣沙溪生\n雪原狼胚\n野皋犺狗";
    deck5.cardIds = {"huansha_xisheng", "xueyuan_langpei", "yegao_kangou"};
    initialDecks_.push_back(deck5);
    
    // 弱小牌组
    InitialDeck deck6;
    deck6.id = "weak_deck";
    deck6.name = u8"弱小牌组";
    deck6.description = u8"白毫仔\n玄贝蚪\n守宫";
    deck6.cardIds = {"baimao_zi", "xuanbei_dou", "shougong"};
    initialDecks_.push_back(deck6);
}

void DeckSelectState::layoutDecks() {
    if (initialDecks_.empty()) return;
    
    // 2行3列的布局 - 进一步增大尺寸和间距
    int rows = 2;
    int cols = 3;
    int deckWidth = 240;   // 进一步增大宽度
    int deckHeight = 300;  // 进一步增大高度
    int spacingX = 120;    // 进一步增加水平间距
    int spacingY = 80;     // 进一步增加垂直间距
    
    int totalWidth = cols * deckWidth + (cols - 1) * spacingX;
    int totalHeight = rows * deckHeight + (rows - 1) * spacingY;
    
    int startX = (screenW_ - totalWidth) / 2;
    int startY = (screenH_ - totalHeight) / 2 + 80; // 为标题留出更多空间
    
    for (size_t i = 0; i < initialDecks_.size(); ++i) {
        int row = static_cast<int>(i) / cols;
        int col = static_cast<int>(i) % cols;
        
        int x = startX + col * (deckWidth + spacingX);
        int y = startY + row * (deckHeight + spacingY);
        
        initialDecks_[i].rect = {x, y, deckWidth, deckHeight};
    }
}

void DeckSelectState::renderDeckView(App& app) {
    if (selectedDeckIndex_ < 0 || selectedDeckIndex_ >= static_cast<int>(initialDecks_.size())) return;
    
    SDL_Renderer* r = app.getRenderer();
    const InitialDeck& deck = initialDecks_[selectedDeckIndex_];
    
    // 渲染标题
    if (titleTex_) {
        SDL_Rect titleRect{ (screenW_ - titleW_) / 2, 20, titleW_, titleH_ };
        SDL_RenderCopy(r, titleTex_, nullptr, &titleRect);
    }
    
    // 渲染牌组名称（居中显示）
    if (deckNameFont_) {
        SDL_Color nameCol{ 200, 230, 255, 255 };
        SDL_Surface* nameSurf = TTF_RenderUTF8_Blended(deckNameFont_, deck.name.c_str(), nameCol);
        if (nameSurf) {
            SDL_Texture* nameTex = SDL_CreateTextureFromSurface(r, nameSurf);
            SDL_Rect nameRect{ (screenW_ - nameSurf->w) / 2, 150, nameSurf->w, nameSurf->h };
            SDL_RenderCopy(r, nameTex, nullptr, &nameRect);
            SDL_DestroyTexture(nameTex);
            SDL_FreeSurface(nameSurf);
        }
    }
    
    // 渲染牌组描述
    if (smallFont_) {
        SDL_Color descCol{ 180, 190, 200, 255 };
        SDL_Surface* descSurf = TTF_RenderUTF8_Blended_Wrapped(smallFont_, deck.description.c_str(), descCol, screenW_ - 40);
        if (descSurf) {
            SDL_Texture* descTex = SDL_CreateTextureFromSurface(r, descSurf);
            SDL_Rect descRect{ 20, 110, descSurf->w, descSurf->h };
            SDL_RenderCopy(r, descTex, nullptr, &descRect);
            SDL_DestroyTexture(descTex);
            SDL_FreeSurface(descSurf);
        }
    }
    
    // 渲染卡牌列表 - 完美居中显示
    int cardWidth = 120;   // 标准牌位宽度
    int cardHeight = 180;   // 标准牌位高度
    int cardSpacing = 20;  // 卡牌间距
    int cardsPerRow = 5;   // 每行卡牌数量
    
    // 计算实际需要的行数
    int totalCards = static_cast<int>(deck.cardIds.size());
    int totalRows = (totalCards + cardsPerRow - 1) / cardsPerRow;
    
    // 计算卡牌区域的总尺寸
    int totalCardsWidth = cardsPerRow * cardWidth + (cardsPerRow - 1) * cardSpacing;
    int totalCardsHeight = totalRows * cardHeight + (totalRows - 1) * cardSpacing;
    
    // 计算屏幕中心位置
    int centerX = screenW_ / 2;
    int centerY = screenH_ / 2;
    
    // 计算卡牌区域的起始位置（让第二张牌在中心）
    // 第二张牌的X坐标应该是 centerX - cardWidth/2
    // 所以第一张牌的X坐标应该是 centerX - cardWidth/2 - (cardWidth + cardSpacing)
    int startX = centerX - cardWidth/2 - (cardWidth + cardSpacing);
    int startY = centerY - totalCardsHeight / 2;
    
    for (size_t i = 0; i < deck.cardIds.size(); ++i) {
        int row = static_cast<int>(i) / cardsPerRow;
        int col = static_cast<int>(i) % cardsPerRow;
        
        int x = startX + col * (cardWidth + cardSpacing);
        int y = startY + row * (cardHeight + cardSpacing);
        
        SDL_Rect cardRect{ x, y, cardWidth, cardHeight };
        
        // 绘制卡牌背景
        SDL_SetRenderDrawColor(r, 40, 50, 60, 200);
        SDL_RenderFillRect(r, &cardRect);
        SDL_SetRenderDrawColor(r, 100, 120, 140, 255);
        SDL_RenderDrawRect(r, &cardRect);
        
        // 创建卡牌并渲染
        Card card = CardDB::instance().make(deck.cardIds[i]);
        if (!card.id.empty()) {
            CardRenderer::renderCard(app, card, cardRect, smallFont_, smallFont_, false);
        }
    }
    
    // 渲染操作提示（居中显示）
    if (deckDescFont_) {
        SDL_Color hintCol{ 150, 160, 170, 255 };
        SDL_Surface* hintSurf = TTF_RenderUTF8_Blended(deckDescFont_, u8"按S键进入地图，按右键返回选择", hintCol);
        if (hintSurf) {
            SDL_Texture* hintTex = SDL_CreateTextureFromSurface(r, hintSurf);
            SDL_Rect hintRect{ (screenW_ - hintSurf->w) / 2, screenH_ - 60, hintSurf->w, hintSurf->h };
            SDL_RenderCopy(r, hintTex, nullptr, &hintRect);
            SDL_DestroyTexture(hintTex);
            SDL_FreeSurface(hintSurf);
        }
    }
}
