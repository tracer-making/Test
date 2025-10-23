#include "MemoryRepairState.h"
#include "MapExploreState.h"
#include "BattleState.h"
#include "../core/ItemStore.h"
#include "../core/WenMaiStore.h"
#include "TestState.h"
#include "BattleState.h"
#include "BarterState.h"
#include "EngraveState.h"
#include "HeritageState.h"
#include "CombineState.h"
#include "InkGhostState.h"
#include "InkWorkshopState.h"
#include "InkShopState.h"
#include "RelicPickupState.h"
#include "SeekerState.h"
#include "TemperState.h"
#include "WenxinTrialState.h"
#include "DeckViewState.h"
#include "../core/App.h"
#include "../core/Deck.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <memory>
#include <random>
#include <algorithm>
#include <queue>
#include <map>
#include <set>

// 静态成员变量定义（仅控制首次自动生成）
bool MapExploreState::s_mapGenerated_ = false;
bool MapExploreState::s_firstMapEnter_ = false;
bool MapExploreState::s_bossVictoryReturn_ = false;

MapExploreState::MapExploreState() = default;
MapExploreState::~MapExploreState() {
    delete regenerateButton_;
    for (auto* b : difficultyButtons_) delete b;
    difficultyButtons_.clear();
    delete backToTestButton_;
    delete testMinerButton_;
    delete testFishermanButton_;
    delete testHunterButton_;
}

void MapExploreState::onEnter(App& app) {
    // 同步全局上帝模式状态
    godMode_ = App::isGodMode();
    
    // 初始化事件图标系统
    loadEventIcons(app.getRenderer());
    
    // 首次进入地图：将全局道具补足至3件（使用战斗界面的道具池），并删除战斗内的补充逻辑
    static bool firstEnter = true;
    if (firstEnter) {
        firstEnter = false;
        auto& store = ItemStore::instance();
        // 开局固定三件：缚魂锁、墨宝瓶、阴阳佩
        auto prettyName = [](const std::string& id) -> std::string {
            if (id == "yinyang_pei" || id == "yinyangpei") return u8"阴阳佩";
            if (id == "mobao_ping") return u8"墨宝瓶";
            if (id == "fuhunsuo") return u8"缚魂锁";
            return id;
        };
        auto prettyDesc = [](const std::string& id) -> std::string {
            if (id == "yinyang_pei" || id == "yinyangpei") return u8"平衡墨尺偏移";
            if (id == "mobao_ping") return u8"存放溢出的墨量";
            if (id == "fuhunsuo") return u8"拖拽移动敌方单位";
            return u8"神秘道具";
        };
        // 清空并设定固定三件
        store.clear();
        store.addItem("fuhunsuo", prettyName("fuhunsuo"), prettyDesc("fuhunsuo"), 1);
        store.addItem("mobao_ping", prettyName("mobao_ping"), prettyDesc("mobao_ping"), 1);
        store.addItem("yinyang_pei", prettyName("yinyang_pei"), prettyDesc("yinyang_pei"), 1);
    }
    // 初始化玩家牌堆（如果还没有初始化）
    auto& store = DeckStore::instance();
    if (store.library().empty() && store.hand().empty()) {
        store.initializePlayerDeck();
        SDL_Log("Player deck initialized with %d cards in library and %d cards in hand", 
                (int)store.library().size(), (int)store.hand().size());
    }
    
    // 获取屏幕尺寸
    int w, h;
    SDL_GetWindowSize(app.getWindow(), &w, &h);
    screenW_ = w;
    screenH_ = h;

    // 加载字体
    font_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 60);
    smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 16);
    if (!font_ || !smallFont_) {
        SDL_Log("TTF_OpenFont failed: %s", TTF_GetError());
    }
    else {
        SDL_Color titleCol{ 200, 230, 255, 255 };
        SDL_Surface* ts = TTF_RenderUTF8_Blended(font_, u8"地图探索", titleCol);
        if (ts) {
            titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), ts);
            titleW_ = ts->w;
            titleH_ = ts->h;
            SDL_FreeSurface(ts);
        }
    }

    // 只在第一次进入时生成地图；否则从全局MapStore恢复
    auto& ms = MapStore::instance();
    
    // 检查是否从Boss战胜利返回
    std::cout << "[MAP EXPLORE] 进入地图，检查s_bossVictoryReturn_: " << s_bossVictoryReturn_ << std::endl;
    if (s_bossVictoryReturn_) {
        std::cout << "[BOSS VICTORY] 从Boss战胜利返回，直接跳转到下一层" << std::endl;
        s_bossVictoryReturn_ = false; // 重置标志
        
        std::cout << "[BOSS VICTORY] 当前层: " << currentMapLayer_ << std::endl;
        std::cout << "[BOSS VICTORY] 玩家当前节点: " << playerCurrentNode_ << std::endl;
        
        // 直接跳转到下一层，不添加节点
        if (currentMapLayer_ < 4) {
            currentMapLayer_++;
            numLayers_++;
            std::cout << "[BOSS VICTORY] 进入第" << currentMapLayer_ << "层" << std::endl;
            
            // 生成新层的地图
            generateMapForLayer(currentMapLayer_);
            s_mapGenerated_ = true;
            
            // 更新全局当前层
            ms.currentMapLayer() = currentMapLayer_;
            
            // 将新地图数据写入全局存储
            ms.layerNodes().clear();
            ms.layerNodes().resize(layerNodes_.size());
            for (size_t i = 0; i < layerNodes_.size(); ++i) {
                ms.layerNodes()[i].clear();
                ms.layerNodes()[i].reserve(layerNodes_[i].size());
                for (size_t j = 0; j < layerNodes_[i].size(); ++j) {
                    const auto& n = layerNodes_[i][j];
                    MapStore::MapNodeData d;
                    d.layer = n.layer; d.x = n.x; d.y = n.y; d.size = n.size; d.label = n.label;
                    d.visited = n.visited; d.accessible = n.accessible; d.connections = n.connections;
                    // 保存随机位移数据
                    int globalIdx = getGlobalNodeIndex(static_cast<int>(i), static_cast<int>(j));
                    if (globalIdx >= 0 && globalIdx < static_cast<int>(nodeDisplayOffset_.size())) {
                        d.displayOffsetX = nodeDisplayOffset_[globalIdx].x;
                        d.displayOffsetY = nodeDisplayOffset_[globalIdx].y;
                    }
                    switch (n.type) {
                    case MapNode::NodeType::START: d.type = MapStore::MapNodeData::NodeType::START; break;
                    case MapNode::NodeType::NORMAL: d.type = MapStore::MapNodeData::NodeType::NORMAL; break;
                    case MapNode::NodeType::BOSS: d.type = MapStore::MapNodeData::NodeType::BOSS; break;
                    case MapNode::NodeType::ELITE: d.type = MapStore::MapNodeData::NodeType::ELITE; break;
                    case MapNode::NodeType::SHOP: d.type = MapStore::MapNodeData::NodeType::SHOP; break;
                    case MapNode::NodeType::EVENT: d.type = MapStore::MapNodeData::NodeType::EVENT; break;
                    }
                    ms.layerNodes()[i].push_back(d);
                }
            }
            ms.numLayers() = numLayers_;
            ms.startNodeIdx() = startNodeIdx_;
            ms.bossNodeIdx() = bossNodeIdx_;
            ms.playerCurrentNode() = playerCurrentNode_;
            ms.accessibleNodes() = accessibleNodes_;
            ms.generated() = true;
            
            // 初始化玩家位置（新层的起点）
            initializePlayer();
            updateScrollBounds();
            
            // 动态水平居中
            if (layerNodes_.size() > 1 && !layerNodes_[1].empty()) {
                int minX = layerNodes_[1][0].x;
                int maxX = layerNodes_[1][0].x;
                for (const auto& n : layerNodes_[1]) {
                    if (n.x < minX) minX = n.x;
                    if (n.x > maxX) maxX = n.x;
                }
                int contentWidth = (maxX - minX) + 400;
                if (contentWidth < 1) contentWidth = 1;
                int desiredOffsetX = (screenW_ - contentWidth) / 2 - minX + 200;
                if (desiredOffsetX < 0) desiredOffsetX = 0;
                mapOffsetX_ = desiredOffsetX;
            }
            
            return; // 直接返回，不执行下面的常规逻辑
        } else {
            std::cout << "[BOSS VICTORY] 已经是最后一层，无法继续前进" << std::endl;
        }
    }
    
    // 首先确保biome已经设置
    std::cout << "[BIOME SETUP] 检查layerBiomes是否为空: " << ms.layerBiomes().empty() << std::endl;
    if (ms.layerBiomes().empty()) {
        std::cout << "[BIOME SETUP] 开始设置biome" << std::endl;
        // 为第1-4层随机分配环境：林地/湿地/雪原，且互不重复
        std::vector<std::string> biomes = { u8"林地", u8"湿地", u8"雪原" };
        std::random_device rd; std::mt19937 g(rd());
        std::shuffle(biomes.begin(), biomes.end(), g);
        ms.layerBiomes().clear();
        ms.layerBiomes().resize(5); // 预留到索引4
        ms.layerBiomes()[1] = biomes[0];
        ms.layerBiomes()[2] = biomes[1];
        ms.layerBiomes()[3] = biomes[2];
        ms.layerBiomes()[4] = biomes[0]; // 第4层默认林地
        std::cout << "[BIOME SETUP] 设置完成，layerBiomes大小: " << ms.layerBiomes().size() << std::endl;
        std::cout << "[BIOME SETUP] 第1层: " << ms.layerBiomes()[1] << std::endl;
        std::cout << "[BIOME SETUP] 第2层: " << ms.layerBiomes()[2] << std::endl;
        std::cout << "[BIOME SETUP] 第3层: " << ms.layerBiomes()[3] << std::endl;
        std::cout << "[BIOME SETUP] 第4层: " << ms.layerBiomes()[4] << std::endl;
    } else {
        std::cout << "[BIOME SETUP] layerBiomes不为空，跳过设置" << std::endl;
    }
    
    if (!s_mapGenerated_ || !ms.hasMap()) {
        generateLayeredMap(true); // 第一次生成，需要构建偏移
        s_mapGenerated_ = true;
        // 将本地数据写入全局
        ms.layerNodes().clear();
        ms.layerNodes().resize(layerNodes_.size());
        for (size_t i = 0; i < layerNodes_.size(); ++i) {
            ms.layerNodes()[i].clear();
            ms.layerNodes()[i].reserve(layerNodes_[i].size());
            for (size_t j = 0; j < layerNodes_[i].size(); ++j) {
                const auto& n = layerNodes_[i][j];
                MapStore::MapNodeData d;
                d.layer = n.layer; d.x = n.x; d.y = n.y; d.size = n.size; d.label = n.label;
                d.visited = n.visited; d.accessible = n.accessible; d.connections = n.connections;
                // 保存随机位移数据
                int globalIdx = getGlobalNodeIndex(static_cast<int>(i), static_cast<int>(j));
                if (globalIdx >= 0 && globalIdx < static_cast<int>(nodeDisplayOffset_.size())) {
                    d.displayOffsetX = nodeDisplayOffset_[globalIdx].x;
                    d.displayOffsetY = nodeDisplayOffset_[globalIdx].y;
                }
                switch (n.type) {
                case MapNode::NodeType::START: d.type = MapStore::MapNodeData::NodeType::START; break;
                case MapNode::NodeType::NORMAL: d.type = MapStore::MapNodeData::NodeType::NORMAL; break;
                case MapNode::NodeType::BOSS: d.type = MapStore::MapNodeData::NodeType::BOSS; break;
                case MapNode::NodeType::ELITE: d.type = MapStore::MapNodeData::NodeType::ELITE; break;
                case MapNode::NodeType::SHOP: d.type = MapStore::MapNodeData::NodeType::SHOP; break;
                case MapNode::NodeType::EVENT: d.type = MapStore::MapNodeData::NodeType::EVENT; break;
                }
                ms.layerNodes()[i].push_back(d);
            }
        }
        ms.numLayers() = numLayers_;
        ms.startNodeIdx() = startNodeIdx_;
        ms.bossNodeIdx() = bossNodeIdx_;
        ms.currentMapLayer() = currentMapLayer_;
        ms.playerCurrentNode() = playerCurrentNode_;
        ms.accessibleNodes() = accessibleNodes_;
        ms.generated() = true;
    } else {
        // 从全局读回
        layerNodes_.clear();
        layerNodes_.resize(ms.layerNodes().size());
        
        // 先初始化nodeDisplayOffset_数组
        int totalNodes = 0;
        for (const auto& layer : ms.layerNodes()) {
            totalNodes += static_cast<int>(layer.size());
        }
        nodeDisplayOffset_.clear();
        nodeDisplayOffset_.resize(totalNodes, SDL_Point{0, 0});
        
        // 使用简单的计数器来恢复随机位移
        int globalOffsetIndex = 0;
        for (size_t i = 0; i < ms.layerNodes().size(); ++i) {
            layerNodes_[i].clear();
            layerNodes_[i].reserve(ms.layerNodes()[i].size());
            for (size_t j = 0; j < ms.layerNodes()[i].size(); ++j) {
                const auto& d = ms.layerNodes()[i][j];
                MapNode n;
                n.layer = d.layer; n.x = d.x; n.y = d.y; n.size = d.size; n.label = d.label;
                n.visited = d.visited; n.accessible = d.accessible; n.connections = d.connections;
                // 恢复随机位移数据
                if (globalOffsetIndex < static_cast<int>(nodeDisplayOffset_.size())) {
                    nodeDisplayOffset_[globalOffsetIndex].x = d.displayOffsetX;
                    nodeDisplayOffset_[globalOffsetIndex].y = d.displayOffsetY;
                    globalOffsetIndex++;
                }
                switch (d.type) {
                case MapStore::MapNodeData::NodeType::START: n.type = MapNode::NodeType::START; break;
                case MapStore::MapNodeData::NodeType::NORMAL: n.type = MapNode::NodeType::NORMAL; break;
                case MapStore::MapNodeData::NodeType::BOSS: n.type = MapNode::NodeType::BOSS; break;
                case MapStore::MapNodeData::NodeType::ELITE: n.type = MapNode::NodeType::ELITE; break;
                case MapStore::MapNodeData::NodeType::SHOP: n.type = MapNode::NodeType::SHOP; break;
                case MapStore::MapNodeData::NodeType::EVENT: n.type = MapNode::NodeType::EVENT; break;
                }
                layerNodes_[i].push_back(n);
            }
        }
        numLayers_ = ms.numLayers();
        startNodeIdx_ = ms.startNodeIdx();
        bossNodeIdx_ = ms.bossNodeIdx();
        currentMapLayer_ = ms.currentMapLayer();
        playerCurrentNode_ = ms.playerCurrentNode();
        accessibleNodes_ = ms.accessibleNodes();
        // 保险：如果全局没有可达节点而当前位置有效，则重新计算可达节点
        if (playerCurrentNode_ != -1 && accessibleNodes_.empty()) {
            updateAccessibleNodes();
            ms.accessibleNodes() = accessibleNodes_;
        }
    }
    
    // 创建重新生成按钮（右上角）
    regenerateButton_ = new Button();
    if (regenerateButton_) {
        int regenButtonWidth = 120;
        int regenButtonHeight = 40;
        SDL_Rect regenButtonRect{ screenW_ - regenButtonWidth - 20, 20, regenButtonWidth, regenButtonHeight };
        regenerateButton_->setRect(regenButtonRect);
        regenerateButton_->setText(u8"重新生成");
        if (smallFont_) {
            regenerateButton_->setFont(smallFont_, app.getRenderer());
        }
        regenerateButton_->setOnClick([this]() {
            generateLayeredMap(true); // 重新生成，需要构建偏移
            s_mapGenerated_ = true; // 确保标志被设置
            // 更新全局
            auto& ms = MapStore::instance();
            ms.generated() = false; // 强制重写
            // 复用 onEnter 的保存流程：这里直接保存
            ms.layerNodes().clear();
            ms.layerNodes().resize(layerNodes_.size());
            for (size_t i = 0; i < layerNodes_.size(); ++i) {
                ms.layerNodes()[i].clear();
                ms.layerNodes()[i].reserve(layerNodes_[i].size());
                for (const auto& n : layerNodes_[i]) {
                    MapStore::MapNodeData d;
                    d.layer = n.layer; d.x = n.x; d.y = n.y; d.size = n.size; d.label = n.label;
                    d.visited = n.visited; d.accessible = n.accessible; d.connections = n.connections;
                    switch (n.type) {
                    case MapNode::NodeType::START: d.type = MapStore::MapNodeData::NodeType::START; break;
                    case MapNode::NodeType::NORMAL: d.type = MapStore::MapNodeData::NodeType::NORMAL; break;
                    case MapNode::NodeType::BOSS: d.type = MapStore::MapNodeData::NodeType::BOSS; break;
                    case MapNode::NodeType::ELITE: d.type = MapStore::MapNodeData::NodeType::ELITE; break;
                    case MapNode::NodeType::SHOP: d.type = MapStore::MapNodeData::NodeType::SHOP; break;
                    case MapNode::NodeType::EVENT: d.type = MapStore::MapNodeData::NodeType::EVENT; break;
                    }
                    ms.layerNodes()[i].push_back(d);
                }
            }
            ms.numLayers() = numLayers_;
            ms.startNodeIdx() = startNodeIdx_;
            ms.bossNodeIdx() = bossNodeIdx_;
            ms.currentMapLayer() = currentMapLayer_;
            ms.playerCurrentNode() = playerCurrentNode_;
            ms.accessibleNodes() = accessibleNodes_;
            ms.generated() = true;
            SDL_Log("Map regenerated by button click!");
        });
    }

    // 创建地图选择按钮（1-4）- 左上角排布
    difficultyButtons_.clear();
    const int btnW = 90;
    const int btnH = 34;
    const int btnGap = 10;
    const int startX = 20;
    const int startY = 20;
    
    std::vector<std::string> mapNames = {
        u8"第一层",
        u8"第二层", 
        u8"第三层",
        u8"第四层"
    };
    
    for (int i = 1; i <= 4; ++i) {
        Button* b = new Button();
        SDL_Rect r{ startX + (i-1)*(btnW + btnGap), startY, btnW, btnH };
        b->setRect(r);
        b->setText(mapNames[i-1]);
        if (smallFont_) b->setFont(smallFont_, app.getRenderer());
        b->setOnClick([this, i]() {
            if (currentMapLayer_ != i) {
                // 切换到新层时生成地图
                currentMapLayer_ = i; // 1..4
                
                // 确保biome已经设置
                auto& ms = MapStore::instance();
                std::cout << "[LAYER SWITCH] 切换到层 " << i << "，检查layerBiomes是否为空: " << ms.layerBiomes().empty() << std::endl;
                if (ms.layerBiomes().empty()) {
                    std::cout << "[LAYER SWITCH] 开始设置biome" << std::endl;
                    // 为第1-4层随机分配环境：林地/湿地/雪原，且互不重复
                    std::vector<std::string> biomes = { u8"林地", u8"湿地", u8"雪原" };
                    std::random_device rd; std::mt19937 g(rd());
                    std::shuffle(biomes.begin(), biomes.end(), g);
                    ms.layerBiomes().clear();
                    ms.layerBiomes().resize(5); // 预留到索引4
                    ms.layerBiomes()[1] = biomes[0];
                    ms.layerBiomes()[2] = biomes[1];
                    ms.layerBiomes()[3] = biomes[2];
                    ms.layerBiomes()[4] = biomes[0]; // 第4层默认林地
                    std::cout << "[LAYER SWITCH] 设置完成，layerBiomes大小: " << ms.layerBiomes().size() << std::endl;
                } else {
                    std::cout << "[LAYER SWITCH] layerBiomes不为空，跳过设置" << std::endl;
                }
                
                generateMapForLayer(i);
                s_mapGenerated_ = true; // 确保标志被设置
                // 更新全局当前层与结构
                ms.currentMapLayer() = currentMapLayer_;
                // 不重建所有层，仅覆盖本层
                if (ms.layerNodes().size() != layerNodes_.size()) {
                    ms.layerNodes().clear();
                    ms.layerNodes().resize(layerNodes_.size());
                }
                if (i >= 0 && i < (int)layerNodes_.size()) {
                    ms.layerNodes()[i].clear();
                    ms.layerNodes()[i].reserve(layerNodes_[i].size());
                    for (const auto& n : layerNodes_[i]) {
                        MapStore::MapNodeData d;
                        d.layer = n.layer; d.x = n.x; d.y = n.y; d.size = n.size; d.label = n.label;
                        d.visited = n.visited; d.accessible = n.accessible; d.connections = n.connections;
                        switch (n.type) {
                        case MapNode::NodeType::START: d.type = MapStore::MapNodeData::NodeType::START; break;
                        case MapNode::NodeType::NORMAL: d.type = MapStore::MapNodeData::NodeType::NORMAL; break;
                        case MapNode::NodeType::BOSS: d.type = MapStore::MapNodeData::NodeType::BOSS; break;
                        case MapNode::NodeType::ELITE: d.type = MapStore::MapNodeData::NodeType::ELITE; break;
                        case MapNode::NodeType::SHOP: d.type = MapStore::MapNodeData::NodeType::SHOP; break;
                        case MapNode::NodeType::EVENT: d.type = MapStore::MapNodeData::NodeType::EVENT; break;
                        }
                        ms.layerNodes()[i].push_back(d);
                    }
                }
                ms.numLayers() = numLayers_;
                ms.startNodeIdx() = startNodeIdx_;
                ms.bossNodeIdx() = bossNodeIdx_;
                ms.playerCurrentNode() = playerCurrentNode_;
                ms.accessibleNodes() = accessibleNodes_;
                ms.generated() = true;
            }
            // 恢复视图到最下方
            scrollY_ = 0;
            SDL_Log("Selected map layer %d", i);
        });
        difficultyButtons_.push_back(b);
    }

    // 返回测试按钮（左上角下方）
    backToTestButton_ = new Button();
    if (backToTestButton_) {
        SDL_Rect r{ startX, startY + btnH + 12, 120, 36 };
        backToTestButton_->setRect(r);
        backToTestButton_->setText(u8"返回测试");
        if (smallFont_) backToTestButton_->setFont(smallFont_, app.getRenderer());
        backToTestButton_->setOnClick([this]() {
            pendingGoTest_ = true;
        });
    }

    // 矿工测试按钮（左上角下方）
    testMinerButton_ = new Button();
    if (testMinerButton_) {
        SDL_Rect r{ startX + 130, startY + btnH + 12, 120, 36 };
        testMinerButton_->setRect(r);
        testMinerButton_->setText(u8"矿工测试");
        if (smallFont_) testMinerButton_->setFont(smallFont_, app.getRenderer());
        testMinerButton_->setOnClick([this]() {
            pendingGoMinerBoss_ = true;
        });
    }
    
    // 渔夫测试按钮（矿工测试按钮下方）
    testFishermanButton_ = new Button();
    if (testFishermanButton_) {
        SDL_Rect r{ startX + 130, startY + btnH + 12 + 40, 120, 36 };
        testFishermanButton_->setRect(r);
        testFishermanButton_->setText(u8"渔夫测试");
        if (smallFont_) testFishermanButton_->setFont(smallFont_, app.getRenderer());
        testFishermanButton_->setOnClick([this]() {
            pendingGoFishermanBoss_ = true;
        });
    }
    
    // 猎人测试按钮（渔夫测试按钮下方）
    testHunterButton_ = new Button();
    if (testHunterButton_) {
        SDL_Rect r{ startX + 130, startY + btnH + 12 + 80, 120, 36 };
        testHunterButton_->setRect(r);
        testHunterButton_->setText(u8"猎人测试");
        if (smallFont_) testHunterButton_->setFont(smallFont_, app.getRenderer());
        testHunterButton_->setOnClick([this]() {
            pendingGoHunterBoss_ = true;
        });
    }
    
    // 最终Boss测试按钮（猎人测试按钮下方）
    testFinalBossButton_ = new Button();
    if (testFinalBossButton_) {
        SDL_Rect r{ startX + 130, startY + btnH + 12 + 120, 120, 36 };
        testFinalBossButton_->setRect(r);
        testFinalBossButton_->setText(u8"最终Boss");
        if (smallFont_) testFinalBossButton_->setFont(smallFont_, app.getRenderer());
        testFinalBossButton_->setOnClick([this]() {
            pendingGoFinalBoss_ = true;
        });
    }
    // 进入时更新滚动边界
    updateScrollBounds();
    
    // 从MapStore加载scrollY_值
    scrollY_ = ms.scrollY();
    
    // 删除自动移动逻辑，改为基于玩家位置的自动滚动
    SDL_Log("Map scroll position loaded: scrollY_=%d", scrollY_);
    
    // 应用滚动边界限制
    if (scrollY_ < 0) scrollY_ = 0;
    if (scrollY_ > maxScrollY_) scrollY_ = maxScrollY_;
    
    // 保存scrollY_值到MapStore
    ms.scrollY() = scrollY_;
    
    SDL_Log("Final map scroll position: scrollY_=%d, maxScrollY_=%d", scrollY_, maxScrollY_);
}

void MapExploreState::onExit(App& app) {
    // 清理资源
    if (titleTex_) {
        SDL_DestroyTexture(titleTex_);
        titleTex_ = nullptr;
    }
    if (font_) {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }
    if (smallFont_) {
        TTF_CloseFont(smallFont_);
        smallFont_ = nullptr;
    }
}

void MapExploreState::handleEvent(App& app, const SDL_Event& e) {
    // 检查是否在猎人Boss毛皮交换状态，如果是则禁用所有交互
    // 注意：这里需要从BattleState获取状态，但MapExploreState无法直接访问
    // 所以我们需要通过其他方式来判断，比如检查当前状态或添加全局标志
    
    // 处理返回按钮（ESC键）
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDL_KeyCode::SDLK_ESCAPE) {
        app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState())));
        return;
    }
    
    // 处理上帝模式切换（T键）
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDL_KeyCode::SDLK_t) {
        godMode_ = !godMode_;
        App::setGodMode(godMode_); // 同步到全局状态
        SDL_Log("God mode %s", godMode_ ? "enabled" : "disabled");
        return;
    }
    
    // 处理牌库查看（W键）
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDL_KeyCode::SDLK_w) {
        app.setState(std::unique_ptr<State>(static_cast<State*>(new DeckViewState())));
        return;
    }
    
    // 处理重新生成地图（R键）
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDL_KeyCode::SDLK_r) {
        generateLayeredMap(true); // 重新生成，需要构建偏移
        s_mapGenerated_ = true; // 确保标志被设置
        // 更新全局
        auto& ms = MapStore::instance();
        ms.layerNodes().clear();
        ms.layerNodes().resize(layerNodes_.size());
        for (size_t i = 0; i < layerNodes_.size(); ++i) {
            ms.layerNodes()[i].clear();
            ms.layerNodes()[i].reserve(layerNodes_[i].size());
            for (size_t j = 0; j < layerNodes_[i].size(); ++j) {
                const auto& n = layerNodes_[i][j];
                MapStore::MapNodeData d;
                d.layer = n.layer; d.x = n.x; d.y = n.y; d.size = n.size; d.label = n.label;
                d.visited = n.visited; d.accessible = n.accessible; d.connections = n.connections;
                // 保存随机位移数据
                int globalIdx = getGlobalNodeIndex(static_cast<int>(i), static_cast<int>(j));
                if (globalIdx >= 0 && globalIdx < static_cast<int>(nodeDisplayOffset_.size())) {
                    d.displayOffsetX = nodeDisplayOffset_[globalIdx].x;
                    d.displayOffsetY = nodeDisplayOffset_[globalIdx].y;
                }
                switch (n.type) {
                case MapNode::NodeType::START: d.type = MapStore::MapNodeData::NodeType::START; break;
                case MapNode::NodeType::NORMAL: d.type = MapStore::MapNodeData::NodeType::NORMAL; break;
                case MapNode::NodeType::BOSS: d.type = MapStore::MapNodeData::NodeType::BOSS; break;
                case MapNode::NodeType::ELITE: d.type = MapStore::MapNodeData::NodeType::ELITE; break;
                case MapNode::NodeType::SHOP: d.type = MapStore::MapNodeData::NodeType::SHOP; break;
                case MapNode::NodeType::EVENT: d.type = MapStore::MapNodeData::NodeType::EVENT; break;
                }
                ms.layerNodes()[i].push_back(d);
            }
        }
        ms.numLayers() = numLayers_;
        ms.startNodeIdx() = startNodeIdx_;
        ms.bossNodeIdx() = bossNodeIdx_;
        ms.currentMapLayer() = currentMapLayer_;
        ms.playerCurrentNode() = playerCurrentNode_;
        ms.accessibleNodes() = accessibleNodes_;
        ms.generated() = true;
        SDL_Log("Map regenerated!");
        return;
    }
    
    // 分发到UI按钮（悬停与点击）- 只在上帝模式下处理
    if (godMode_) {
        if (regenerateButton_) {
            regenerateButton_->handleEvent(e);
        }
        for (auto* b : difficultyButtons_) {
            if (b) b->handleEvent(e);
        }
        if (backToTestButton_) backToTestButton_->handleEvent(e);
        if (testMinerButton_) testMinerButton_->handleEvent(e);
        if (testFishermanButton_) testFishermanButton_->handleEvent(e);
        if (testHunterButton_) testHunterButton_->handleEvent(e);
        if (testFinalBossButton_) testFinalBossButton_->handleEvent(e);
    }

    // 滚轮滚动地图（仅在上帝模式下生效）
    if (e.type == SDL_MOUSEWHEEL && godMode_) {
        // 定义：scrollY_ = 0 表示顶部对齐；scrollY_ 增大表示向下滚动
        // 期望行为：鼠标向上滚，地图向上移动 ⇒ scrollY_ 减小；
        // SDL 中 e.wheel.y>0 表示向上滚，因此使用相反方向：scrollY_ -= e.wheel.y * scrollStep_
        int oldScrollY = scrollY_;
        scrollY_ += e.wheel.y * scrollStep_;
        if (scrollY_ < 0) scrollY_ = 0;              // 向上极限
        if (scrollY_ > maxScrollY_) scrollY_ = maxScrollY_; // 向下极限
        
        // 计算玩家当前位置的绝对坐标和屏幕相对坐标
        int playerAbsoluteY = 0;  // 玩家绝对Y坐标（以起点为0）
        int playerScreenY = 0;    // 玩家屏幕相对坐标（真实渲染坐标）
        
        if (playerCurrentNode_ >= 0 && playerCurrentNode_ < getTotalNodeCount()) {
            // 获取玩家当前节点
            const MapNode* playerNode = getNodeByGlobalIndex(playerCurrentNode_);
            if (playerNode) {
                // 绝对坐标：以起点为0的绝对位置
                playerAbsoluteY = playerNode->y;
                
                // 屏幕相对坐标：真实渲染坐标（应用偏移和滚动）
                playerScreenY = mapOffsetY_ + (playerNode->y + scrollY_);
            }
        }
        
        // 输出滚轮移动信息和坐标信息
        SDL_Log("Wheel scroll: wheel.y=%d, oldScrollY=%d, newScrollY=%d, maxScrollY=%d", 
                e.wheel.y, oldScrollY, scrollY_, maxScrollY_);
        SDL_Log("Player coordinates: AbsoluteY=%d, ScreenY=%d", playerAbsoluteY, playerScreenY);
    }

    // 处理鼠标点击到节点
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;
        // 检查是否点击了节点
        for (int layer = 0; layer <= numLayers_; ++layer) {
            for (size_t i = 0; i < layerNodes_[layer].size(); ++i) {
                const MapNode& node = layerNodes_[layer][i];
                int sx, sy; nodeToScreenXY(node, sx, sy);
                int dx = mx - sx;
                int dy = my - sy;
                if (dx * dx + dy * dy <= node.size * node.size) {
                    int globalIndex = getGlobalNodeIndex(layer, static_cast<int>(i));
                    SDL_Log("Clicked node %d (layer %d, local %zu)", globalIndex, layer, i);
                    
                    // 如果点击的是可访问的节点，移动玩家
                    if (isNodeAccessible(globalIndex)) {
                        startMoveAnimation(globalIndex);
                        SDL_Log("Start moving to accessible node %d", globalIndex);
                    } else if (globalIndex == playerCurrentNode_) {
                        SDL_Log("Player is already at node %d", globalIndex);
                    } else {
                        SDL_Log("Node %d is not accessible from current position", globalIndex);
                    }
                    break;
                }
            }
        }
    }
}

void MapExploreState::update(App& app, float dt) {
    // 更新玩家移动动画
    updatePlayerMoveAnimation(dt);
    
    // 更新事件图标动画
    updateEventIcons(dt);
    
    // 移动动画推进
    if (isMoving_) {
        moveT_ += dt / moveDuration_;
        if (moveT_ >= 1.0f) {
            moveT_ = 1.0f;
        }
        if (moveT_ >= 1.0f) {
            isMoving_ = false;
            // 到达目标节点后，正式移动并触发事件
            if (moveToNode_ != -1) {
                movePlayerToNode(moveToNode_);
                // 写回全局存储当前位置
                auto& ms = MapStore::instance();
                ms.playerCurrentNode() = playerCurrentNode_;
            }
            moveFromNode_ = -1;
            moveToNode_ = -1;
        }
    }

    // 更新逻辑（如果需要）
    if (pendingGoTest_) {
        pendingGoTest_ = false;
        app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState())));
        return;
    }
    
    // 处理各种状态切换
    if (pendingGoBattle_) {
        pendingGoBattle_ = false;
        
        // 从MapStore获取当前层的地形信息
        auto& ms = MapStore::instance();
        std::string currentBiome = u8"林地"; // 默认地形
        int currentLayer = currentMapLayer_;
        
        if (!ms.layerBiomes().empty() && currentLayer < (int)ms.layerBiomes().size()) {
            currentBiome = ms.layerBiomes()[currentLayer];
        }
        
        std::cout << "[MAPEXPLORE] 当前层: " << currentLayer << ", 地形: " << currentBiome << std::endl;
        
        // 使用随机战斗选择系统
        auto& presetManager = EnemyPresetManager::instance();
        auto availableBattles = presetManager.getRandomBattleSequence(currentBiome, currentLayer, 1);
        
        int battleId = 1; // 默认战斗ID
        if (!availableBattles.empty()) {
            battleId = availableBattles[0];
        }
        
        std::cout << "[MAPEXPLORE] 随机选择战斗: 地形=" << currentBiome 
                  << ", 层数=" << currentLayer << ", 战斗ID=" << battleId << ", 战斗类型=" << currentBattleType_ << std::endl;
        
        // 根据战斗类型决定是否为意境之斗
        bool isEngraveBattle = (currentBattleType_ == u8"意境之斗");
        app.setState(std::unique_ptr<State>(static_cast<State*>(new BattleState(battleId, isEngraveBattle))));
        return;
    }
    
    if (pendingGoMinerBoss_) {
        pendingGoMinerBoss_ = false;
        std::cout << "[MINER TEST] 矿工测试按钮被点击，进入Boss战（ID: 100）" << std::endl;
        std::cout << "[MINER TEST] 当前蜡烛数量: " << App::getRemainingCandles() << std::endl;
        // 设置矿工Boss战（ID: 100）
        auto* battleState = new BattleState(100);
        app.setState(std::unique_ptr<State>(static_cast<State*>(battleState)));
        return;
    }
    
    if (pendingGoFishermanBoss_) {
        pendingGoFishermanBoss_ = false;
        std::cout << "[FISHERMAN TEST] 渔夫测试按钮被点击，进入Boss战（ID: 102）" << std::endl;
        std::cout << "[FISHERMAN TEST] 当前蜡烛数量: " << App::getRemainingCandles() << std::endl;
        // 设置渔夫Boss战（ID: 102）
        auto* battleState = new BattleState(102);
        app.setState(std::unique_ptr<State>(static_cast<State*>(battleState)));
        return;
    }
    
    if (pendingGoHunterBoss_) {
        pendingGoHunterBoss_ = false;
        std::cout << "[HUNTER TEST] 猎人测试按钮被点击，进入Boss战（ID: 104）" << std::endl;
        std::cout << "[HUNTER TEST] 当前蜡烛数量: " << App::getRemainingCandles() << std::endl;
        std::cout << "[HUNTER TEST] 正在创建BattleState(104)..." << std::endl;
        // 设置猎人Boss战（ID: 104）
        auto* battleState = new BattleState(104);
        std::cout << "[HUNTER TEST] BattleState创建完成，正在切换状态..." << std::endl;
        app.setState(std::unique_ptr<State>(static_cast<State*>(battleState)));
        std::cout << "[HUNTER TEST] 状态切换完成！" << std::endl;
        return;
    }
    
    if (pendingGoFinalBoss_) {
        pendingGoFinalBoss_ = false;
        std::cout << "[FINAL BOSS] 最终Boss按钮被点击，进入Boss战（ID: 105）" << std::endl;
        std::cout << "[FINAL BOSS] 当前蜡烛数量: " << App::getRemainingCandles() << std::endl;
        std::cout << "[FINAL BOSS] 正在创建BattleState(105)..." << std::endl;
        // 设置最终Boss战（ID: 105）
        auto* battleState = new BattleState(105);
        std::cout << "[FINAL BOSS] BattleState创建完成，正在切换状态..." << std::endl;
        app.setState(std::unique_ptr<State>(static_cast<State*>(battleState)));
        std::cout << "[FINAL BOSS] 状态切换完成！" << std::endl;
        return;
    }
    
    if (pendingGoBarter_) {
        pendingGoBarter_ = false;
        app.setState(std::unique_ptr<State>(static_cast<State*>(new BarterState())));
        return;
    }
    
    if (pendingGoEngrave_) {
        pendingGoEngrave_ = false;
        app.setState(std::unique_ptr<State>(static_cast<State*>(new EngraveState())));
        return;
    }
    
    if (pendingGoHeritage_) {
        pendingGoHeritage_ = false;
        app.setState(std::unique_ptr<State>(static_cast<State*>(new HeritageState())));
        return;
    }
    
    if (pendingGoInkGhost_) {
        pendingGoInkGhost_ = false;
        app.setState(std::unique_ptr<State>(static_cast<State*>(new InkGhostState())));
        return;
    }
    
    if (pendingGoInkWorkshop_) {
        pendingGoInkWorkshop_ = false;
        app.setState(std::unique_ptr<State>(static_cast<State*>(new InkWorkshopState(currentMapLayer_))));
        return;
    }
    
    if (pendingGoInkShop_) {
        pendingGoInkShop_ = false;
        app.setState(std::unique_ptr<State>(static_cast<State*>(new InkShopState())));
        return;
    }
    
    if (pendingGoMemoryRepair_) {
        pendingGoMemoryRepair_ = false;
        app.setState(std::unique_ptr<State>(static_cast<State*>(new MemoryRepairState())));
        return;
    }
    
    if (pendingGoRelicPickup_) {
        pendingGoRelicPickup_ = false;
        app.setState(std::unique_ptr<State>(static_cast<State*>(new RelicPickupState())));
        return;
    }
    
    if (pendingGoSeeker_) {
        pendingGoSeeker_ = false;
        app.setState(std::unique_ptr<State>(static_cast<State*>(new SeekerState())));
        return;
    }
    
    if (pendingGoTemper_) {
        pendingGoTemper_ = false;
        app.setState(std::unique_ptr<State>(static_cast<State*>(new TemperState())));
        // 新增：合卷入口（可根据你的地图节点绑定按键/区域触发）
        // 这里示例：按键'H'进入合卷（请替换为你的真实事件逻辑）
        return;
    }
}

void MapExploreState::render(App& app) {
    SDL_Renderer* r = app.getRenderer();
    
    // 清屏 - 灰白色背景
    SDL_SetRenderDrawColor(r, 180, 180, 180, 255);
    SDL_RenderClear(r);
    
    // 渲染标题（已删除"地图探索"标题）
    renderTitle(r);
    
    // 渲染地图
    renderMap(r);
    
    // 渲染左侧UI信息
    renderLeftSideUI(r);
    
    // 渲染按钮（只在上帝模式下显示）
    if (godMode_) {
        if (regenerateButton_) {
            try {
                regenerateButton_->render(r);
            } catch (...) {
                // 如果按钮渲染失败，忽略错误
            }
        }
        
        // 渲染复杂度按钮
        for (auto* b : difficultyButtons_) {
            if (!b) continue;
            try {
                b->render(r);
            } catch (...) {
            }
        }
        if (backToTestButton_) {
            try { backToTestButton_->render(r); } catch (...) {}
        }
        if (testMinerButton_) {
            try { testMinerButton_->render(r); } catch (...) {}
        }
        if (testFishermanButton_) {
            try { testFishermanButton_->render(r); } catch (...) {}
        }
        if (testHunterButton_) {
            try { testHunterButton_->render(r); } catch (...) {}
        }
        if (testFinalBossButton_) {
            try { testFinalBossButton_->render(r); } catch (...) {}
        }
    }
    
    // 渲染上帝模式提示
    if (godMode_) {
        renderGodModeIndicator(r);
    }
    
    // 绘制操作说明（右下角）
    if (smallFont_) {
        SDL_Color helpColor{ 180, 180, 180, 255 }; // 灰色文字
        const char* helpLines[] = {
            u8"地图探索操作说明：",
            u8"• 点击节点进入战斗/商店/其他功能",
            u8"• 不同颜色节点代表不同类型",
            u8"• 战斗节点：红色（普通战斗）",
            u8"• 商店节点：蓝色（购买道具）",
            u8"• 特殊节点：绿色（特殊功能）",
            u8"• 按ESC返回主菜单"
        };
        
        int lineCount = sizeof(helpLines) / sizeof(helpLines[0]);
        int x = screenW_ - 300;
        int y = screenH_ - 200;
        
        for (int i = 0; i < lineCount; ++i) {
            SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, helpLines[i], helpColor);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                if (t) {
                    SDL_Rect dst{ x, y, s->w, s->h };
                    SDL_RenderCopy(r, t, nullptr, &dst);
                    SDL_DestroyTexture(t);
                }
                SDL_FreeSurface(s);
            }
            y += 18;
        }
    }
}

void MapExploreState::renderTitle(SDL_Renderer* renderer) {
    // 已删除"地图探索"标题渲染
    // 显示当前层环境
    if (smallFont_) {
        auto& ms = MapStore::instance();
        std::string biome;
        if ((int)ms.layerBiomes().size() > currentMapLayer_) {
            biome = ms.layerBiomes()[currentMapLayer_];
        }
        if (!biome.empty()) {
            SDL_Color col{200,230,255,255};
            std::string text = u8"当前层环境：" + biome;
            SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, text.c_str(), col);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
                SDL_Rect d{ 20, 20, s->w, s->h };
                SDL_RenderCopy(renderer, t, nullptr, &d);
                SDL_DestroyTexture(t);
                SDL_FreeSurface(s);
            }
        }
    }
}

void MapExploreState::renderGodModeIndicator(SDL_Renderer* renderer) {
    // 在屏幕右上角显示上帝模式指示器
    SDL_Rect indicatorRect = {
        screenW_ - 200,
        20,
        180,
        30
    };
    
    // 绘制半透明背景
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 128); // 黄色半透明
    SDL_RenderFillRect(renderer, &indicatorRect);
    
    // 绘制边框
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // 黄色
    SDL_RenderDrawRect(renderer, &indicatorRect);
    
    // 这里可以添加文字渲染，但需要字体支持
    // 暂时用简单的视觉指示器
}


void MapExploreState::generateLayeredMap(bool shouldBuildOffsets) {
    SDL_Log("Generating map for layer %d", currentMapLayer_);
    generateMapForLayer(currentMapLayer_);
    
    // 根据参数决定是否构建显示偏移
    if (shouldBuildOffsets) {
        buildDisplayOffsets();
    }
}

void MapExploreState::generateMapForLayer(int layer) {
    SDL_Log("Generating map for layer %d", layer);
    
    // 确保biome已经设置
    auto& ms = MapStore::instance();
    if (ms.layerBiomes().empty()) {
        std::cout << "[GENERATE MAP] 在generateMapForLayer中设置biome" << std::endl;
        // 为第1-4层随机分配环境：林地/湿地/雪原，且互不重复
        std::vector<std::string> biomes = { u8"林地", u8"湿地", u8"雪原" };
        std::random_device rd; std::mt19937 g(rd());
        std::shuffle(biomes.begin(), biomes.end(), g);
        ms.layerBiomes().clear();
        ms.layerBiomes().resize(5); // 预留到索引4
        ms.layerBiomes()[1] = biomes[0];
        ms.layerBiomes()[2] = biomes[1];
        ms.layerBiomes()[3] = biomes[2];
        ms.layerBiomes()[4] = biomes[0]; // 第4层默认林地
        std::cout << "[GENERATE MAP] 设置完成，layerBiomes大小: " << ms.layerBiomes().size() << std::endl;
    }
    
    // 1. 初始化参数 - 单层地图结构
    numLayers_ = 2; // 终点在第2层
    layerNodes_.clear();
    layerNodes_.resize(3); // 第0层(起点) + 第1层(中间节点) + 第2层(终点)
    
    // 创建起点（第0层，世界坐标原点）
    MapNode startNode;
    startNode.layer = 0;
    startNode.x = this->screenW_ / 2; // 水平居中
    startNode.y = 0; // 世界坐标：y=0
    startNode.type = MapNode::NodeType::START;
    startNode.accessible = true;
    layerNodes_[0].push_back(startNode);
    startNodeIdx_ = 0;
    
    // 2. 根据层数生成对应的节点模式
    std::random_device rd;
    std::mt19937 gen(rd());
    
    switch (layer) {
        case 1:
            generateFirstLayerPattern(gen);
            break;
        case 2:
        case 3:
            generateSecondThirdLayerPattern(gen);
            break;
        case 4:
            generateLastLayerPattern(gen);
            break;
        default:
            generateFirstLayerPattern(gen);
            break;
    }
    
    // 3. 构建仅用于显示的偏移（不改变逻辑坐标/连接）
    // 注意：现在由generateLayeredMap()的shouldBuildOffsets参数控制

    // 4. 为每一行节点分配不重复的事件标签（仅显示用）
    assignRowEventLabels();

    // 5. 连接路径
    connectPaths();
    
    // 6. 创建终点
    createEndNode();
    
    // 7. 验证路径
    validatePaths();
    
    // 8. 初始化玩家位置
    initializePlayer();
    // 7. 更新滚动边界
    updateScrollBounds();
    // 动态水平居中
    // 计算第1层内容的左右边界，使其在当前屏幕宽度内视觉居中
    if (layerNodes_.size() > 1 && !layerNodes_[1].empty()) {
        int minX = layerNodes_[1][0].x;
        int maxX = layerNodes_[1][0].x;
        for (const auto& n : layerNodes_[1]) {
            if (n.x < minX) minX = n.x;
            if (n.x > maxX) maxX = n.x;
        }
        int contentWidth = (maxX - minX) + 400; // 左右各留200像素
        if (contentWidth < 1) contentWidth = 1;
        int desiredOffsetX = (screenW_ - contentWidth) / 2 - minX + 200; // 加回左边距
        if (desiredOffsetX < 0) desiredOffsetX = 0;
        mapOffsetX_ = desiredOffsetX;
    }
    
    SDL_Log("Map for layer %d generated successfully", layer);
}

void MapExploreState::generateFirstLayerPattern(std::mt19937& gen) {
    // 第一层：第一行必定是"以物易物"且只有1个节点，然后按23123123122模式
    std::vector<int> pattern = {1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 2}; // 12个节点类型
    
    // 使用世界坐标：起点y=0，向上为负，每行固定间距（进一步加大垂直间距）
    const int rowSpacing = 220;
    
    // 为每个节点类型生成1-3个节点（概率3:4:3）
    std::uniform_int_distribution<> nodeCountDist(1, 10); // 1..10 → 1:3, 2:4, 3:3
    
    int currentY = -rowSpacing; // 第一行在y=-rowSpacing
    
    for (size_t i = 0; i < pattern.size(); ++i) {
        int nodeCount;
        
        // 第一行（索引0）必定只有1个节点
        if (i == 0) {
            nodeCount = 1;
        } else {
            int roll = nodeCountDist(gen);
            if (roll <= 3) {
                nodeCount = 1; // 概率 3/10
            } else if (roll <= 7) {
                nodeCount = 2; // 概率 4/10
            } else {
                nodeCount = 3; // 概率 3/10
            }
        }
        
        // 为这个类型生成1-3个节点
        for (int j = 0; j < nodeCount; ++j) {
            MapNode newNode;
            newNode.layer = 1;
            
            // 水平位置：根据节点数量分布（进一步加大水平间距）
            if (nodeCount == 1) {
                newNode.x = this->screenW_ / 2; // 居中
            } else if (nodeCount == 2) {
                newNode.x = this->screenW_ / 2 + (j == 0 ? -140 : 140); // 左右分布
            } else { // nodeCount == 3
                newNode.x = this->screenW_ / 2 + (j - 1) * 180; // 左中右分布（更大间距）
            }
            
            newNode.y = currentY;
            
            // 根据模式设置节点类型
            switch (pattern[i]) {
                case 1: newNode.type = MapNode::NodeType::SHOP; break; // 卡牌获取
                case 2: newNode.type = MapNode::NodeType::EVENT; break; // 增益事件
                case 3: newNode.type = MapNode::NodeType::ELITE; break; // 战斗
            }
            
            newNode.accessible = true;
            this->layerNodes_[1].push_back(newNode);
        }
        
        // 移动到下一行
        currentY -= rowSpacing;
    }
    
    SDL_Log("First layer pattern generated: %zu nodes", this->layerNodes_[1].size());
}

void MapExploreState::generateSecondThirdLayerPattern(std::mt19937& gen) {
    // 第二、三层：按123123123122模式
    std::vector<int> pattern = {1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 2}; // 12个节点类型
    
    // 使用世界坐标：起点y=0，向上为负，每行固定间距（进一步加大垂直间距）
    const int rowSpacing = 220;
    
    // 为每个节点类型生成1-3个节点（概率3:4:3）
    std::uniform_int_distribution<> nodeCountDist(1, 10); // 1..10 → 1:3, 2:4, 3:3
    
    int currentY = -rowSpacing;
    
    for (size_t i = 0; i < pattern.size(); ++i) {
        int roll = nodeCountDist(gen);
        int nodeCount;
        if (roll <= 3) {
            nodeCount = 1; // 概率 3/10
        } else if (roll <= 7) {
            nodeCount = 2; // 概率 4/10
        } else {
            nodeCount = 3; // 概率 3/10
        }
        
        // 为这个类型生成1-3个节点
        for (int j = 0; j < nodeCount; ++j) {
            MapNode newNode;
            newNode.layer = 1;
            
            // 水平位置：根据节点数量分布（进一步加大水平间距）
            if (nodeCount == 1) {
                newNode.x = this->screenW_ / 2; // 居中
            } else if (nodeCount == 2) {
                newNode.x = this->screenW_ / 2 + (j == 0 ? -140 : 140); // 左右分布
            } else { // nodeCount == 3
                newNode.x = this->screenW_ / 2 + (j - 1) * 180; // 左中右分布（更大间距）
            }
            
            newNode.y = currentY;
            
            // 根据模式设置节点类型
            switch (pattern[i]) {
                case 1: newNode.type = MapNode::NodeType::SHOP; break; // 卡牌获取
                case 2: newNode.type = MapNode::NodeType::EVENT; break; // 增益事件
                case 3: newNode.type = MapNode::NodeType::ELITE; break; // 战斗
            }
            
            newNode.accessible = true;
            this->layerNodes_[1].push_back(newNode);
        }
        
        // 移动到下一行
        currentY -= rowSpacing;
    }
    
    SDL_Log("Second/Third layer pattern generated: %zu nodes", this->layerNodes_[1].size());
}

void MapExploreState::generateLastLayerPattern(std::mt19937& gen) {
    // 第四层：固定中间一行四个节点 + boss
    // 节点类型和标签固定为：墨宝拾遗、以物易物、意境刻画、文脉传承
    std::vector<MapNode::NodeType> rowTypes = {
        MapNode::NodeType::EVENT,  // 墨宝拾遗
        MapNode::NodeType::SHOP,   // 以物易物
        MapNode::NodeType::EVENT,  // 意境刻画
        MapNode::NodeType::EVENT   // 文脉传承
    };
    
    std::vector<std::string> rowLabels = {
        u8"墨宝拾遗",
        u8"以物易物", 
        u8"意境刻画",
        u8"文脉传承"
    };

    // 计算垂直位置：使用世界坐标的中间一行
    const int rowSpacing = 120;
    int middleY = -rowSpacing;

    // 水平位置：等间距分布四个节点（使用1/5,2/5,3/5,4/5处）
    std::vector<int> xs = {
        (this->screenW_ / 5) * 1,
        (this->screenW_ / 5) * 2,
        (this->screenW_ / 5) * 3,
        (this->screenW_ / 5) * 4
    };

    this->layerNodes_[1].reserve(this->layerNodes_[1].size() + 4);
    for (int i = 0; i < 4; ++i) {
        MapNode node;
        node.layer = 1;
        node.x = xs[i];
        node.y = middleY;
        node.type = rowTypes[i];
        node.label = rowLabels[i];  // 设置固定标签
        node.accessible = true;
        this->layerNodes_[1].push_back(node);
    }

    // 注意：boss节点在 createEndNode() 中创建
    SDL_Log("Last layer pattern generated (fixed 4 middle nodes): %zu nodes", this->layerNodes_[1].size());
}

void MapExploreState::connectPaths() {
    // 连接路径：起点 → 第一行所有节点 → 第二行所有节点 → ... → 终点
    SDL_Log("Starting multi-node path connections...");
    
    // 初始化随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // 起点连接到第一行的所有节点
    if (!layerNodes_[0].empty() && !layerNodes_[1].empty()) {
        int startGlobalIdx = getGlobalNodeIndex(0, 0);
        if (startGlobalIdx != -1) {
            MapNode* startNode = getNodeByGlobalIndex(startGlobalIdx);
            if (startNode) {
                // 找到第一行的所有节点（Y坐标相同的节点）
                int firstRowY = layerNodes_[1][0].y;
            for (size_t i = 0; i < layerNodes_[1].size(); ++i) {
                    if (layerNodes_[1][i].y == firstRowY) {
                int targetGlobalIdx = getGlobalNodeIndex(1, static_cast<int>(i));
                if (targetGlobalIdx != -1) {
                        startNode->connections.push_back(targetGlobalIdx);
                            SDL_Log("Connected start (idx %d) to first row node (idx %d)", 
                                    startGlobalIdx, targetGlobalIdx);
                        }
                    }
                }
            }
        }
    }
    
    // 连接每一行到下一行：使用单调匹配，保证无交叉且所有节点可达
    if (!layerNodes_[1].empty()) {
        // 按Y坐标分组节点
        std::map<int, std::vector<int>> rows; // Y坐标 -> 节点索引列表
        for (size_t i = 0; i < layerNodes_[1].size(); ++i) {
            int y = layerNodes_[1][i].y;
            rows[y].push_back(static_cast<int>(i));
        }
        
        // 按Y坐标排序（从下到上）
        std::vector<std::pair<int, std::vector<int>>> sortedRows(rows.begin(), rows.end());
        std::sort(sortedRows.begin(), sortedRows.end(), 
                  [](const auto& a, const auto& b) { return a.first > b.first; });
        
        // 连接相邻行 - 单向、非交叉、全可达
        for (size_t rowIdx = 0; rowIdx < sortedRows.size() - 1; ++rowIdx) {
            const auto& currentRow = sortedRows[rowIdx].second;
            const auto& nextRow = sortedRows[rowIdx + 1].second;
            
            // 1) 按X坐标排序，建立单调匹配的索引序列
            std::vector<std::pair<int, int>> orderedCurrent; // (x, idxInLayer1)
            std::vector<std::pair<int, int>> orderedNext;    // (x, idxInLayer1)
            orderedCurrent.reserve(currentRow.size());
            orderedNext.reserve(nextRow.size());

            for (int idx : currentRow) {
                orderedCurrent.push_back({ layerNodes_[1][idx].x, idx });
            }
            for (int idx : nextRow) {
                orderedNext.push_back({ layerNodes_[1][idx].x, idx });
            }
            std::sort(orderedCurrent.begin(), orderedCurrent.end(), [](const auto& a, const auto& b){ return a.first < b.first; });
            std::sort(orderedNext.begin(), orderedNext.end(), [](const auto& a, const auto& b){ return a.first < b.first; });

            const int m = static_cast<int>(orderedCurrent.size());
            const int n = static_cast<int>(orderedNext.size());

            // 2) 第一轮：为每个当前行节点分配一个下一行节点（比例映射，避免交叉）
            std::vector<int> indegree(n, 0);
            for (int i = 0; i < m; ++i) {
                int mappedJ = 0;
                if (m > 1 && n > 1) {
                    mappedJ = static_cast<int>(std::round((i * 1.0) * (n - 1) / (m - 1)));
                } else {
                    mappedJ = 0; // 退化情形
                }
                mappedJ = std::max(0, std::min(n - 1, mappedJ));

                int currentIdxInLayer = orderedCurrent[i].second;
                int nextIdxInLayer = orderedNext[mappedJ].second;

                int currentGlobalIdx = getGlobalNodeIndex(1, currentIdxInLayer);
                int nextGlobalIdx = getGlobalNodeIndex(1, nextIdxInLayer);
                if (currentGlobalIdx != -1 && nextGlobalIdx != -1) {
                    MapNode* currentNode = getNodeByGlobalIndex(currentGlobalIdx);
                    if (currentNode) {
                        currentNode->connections.push_back(nextGlobalIdx);
                        indegree[mappedJ] += 1;
                        SDL_Log("Monotone connect: row node %d (idx %d) -> next row node %d (idx %d)",
                                currentIdxInLayer, currentGlobalIdx, nextIdxInLayer, nextGlobalIdx);
                    }
                }
            }

            // 3) 第二轮：确保每个下一行节点至少有一个入边
            for (int j = 0; j < n; ++j) {
                if (indegree[j] == 0) {
                    int mappedI = 0;
                    if (m > 1 && n > 1) {
                        mappedI = static_cast<int>(std::round((j * 1.0) * (m - 1) / (n - 1)));
                    } else {
                        mappedI = 0;
                    }
                    mappedI = std::max(0, std::min(m - 1, mappedI));

                    int currentIdxInLayer = orderedCurrent[mappedI].second;
                    int nextIdxInLayer = orderedNext[j].second;

                    int currentGlobalIdx = getGlobalNodeIndex(1, currentIdxInLayer);
                    int nextGlobalIdx = getGlobalNodeIndex(1, nextIdxInLayer);
                    if (currentGlobalIdx != -1 && nextGlobalIdx != -1) {
                        MapNode* currentNode = getNodeByGlobalIndex(currentGlobalIdx);
                        if (currentNode) {
                            // 避免重复连接
                            bool alreadyConnected = false;
                            for (int existing : currentNode->connections) {
                                if (existing == nextGlobalIdx) { alreadyConnected = true; break; }
                            }
                            if (!alreadyConnected) {
                                currentNode->connections.push_back(nextGlobalIdx);
                                SDL_Log("Ensure indegree: row node %d (idx %d) -> next row node %d (idx %d)",
                                        currentIdxInLayer, currentGlobalIdx, nextIdxInLayer, nextGlobalIdx);
                            }
                        }
                    }
                }
            }
        }
    }
    
    SDL_Log("Multi-node path connections completed");
}

void MapExploreState::createEndNode() {
    // 创建终点节点（BOSS）
    MapNode endNode;
    endNode.layer = numLayers_;
    endNode.x = this->screenW_ / 2; // 水平居中
    
    // 终点位置：位于最后一行之上一个行距（世界坐标，向上为负）
    int lastRowY = 0;
    if (!layerNodes_[1].empty()) {
        lastRowY = layerNodes_[1][0].y;
        for (size_t i = 1; i < layerNodes_[1].size(); ++i) {
            if (layerNodes_[1][i].y < lastRowY) lastRowY = layerNodes_[1][i].y;
        }
    }
    const int rowSpacing = 120;
    endNode.y = lastRowY - 2 * rowSpacing;
    
    endNode.type = MapNode::NodeType::BOSS;
    endNode.accessible = true;
    
    // 根据当前层的地图类型设置对应的Boss标签
    auto& ms = MapStore::instance();
    std::string biome;
    if ((int)ms.layerBiomes().size() > currentMapLayer_) {
        biome = ms.layerBiomes()[currentMapLayer_];
    }
    
    // 调试输出
    std::cout << "[BOSS NODE] 当前层: " << currentMapLayer_ << std::endl;
    std::cout << "[BOSS NODE] layerBiomes大小: " << ms.layerBiomes().size() << std::endl;
    std::cout << "[BOSS NODE] 地图类型: '" << biome << "'" << std::endl;
    
    if (currentMapLayer_ == 4) {
        // 第四层是最终Boss
        endNode.label = u8"最终Boss";
        std::cout << "[BOSS NODE] 设置为最终Boss" << std::endl;
    } else if (biome == u8"林地") {
        endNode.label = u8"矿工Boss";
        std::cout << "[BOSS NODE] 设置为矿工Boss" << std::endl;
    } else if (biome == u8"湿地") {
        endNode.label = u8"渔夫Boss";
        std::cout << "[BOSS NODE] 设置为渔夫Boss" << std::endl;
    } else if (biome == u8"雪原") {
        endNode.label = u8"猎人Boss";
        std::cout << "[BOSS NODE] 设置为猎人Boss" << std::endl;
    } else {
        // 默认矿工Boss（如果地图类型未设置或未知）
        endNode.label = u8"矿工Boss";
        std::cout << "[BOSS NODE] 设置为默认矿工Boss，biome为空或未知" << std::endl;
    }
    
    layerNodes_[numLayers_].push_back(endNode);
    bossNodeIdx_ = 0;
    
    // 向上道路节点现在在onEnter中的boss胜利返回逻辑中创建
    
    SDL_Log("Created end node at layer %d, x=%d, y=%d (map layer %d)", 
            numLayers_, endNode.x, endNode.y, currentMapLayer_);
    
    // 连接最后一行（第12行）的所有节点到终点
    if (!layerNodes_[1].empty()) {
        int endGlobalIdx = getGlobalNodeIndex(numLayers_, 0);
        
        // 找到最后一行（Y坐标最小的行）的所有节点
        int lastRowY = layerNodes_[1][0].y;
        for (size_t i = 0; i < layerNodes_[1].size(); ++i) {
            if (layerNodes_[1][i].y < lastRowY) {
                lastRowY = layerNodes_[1][i].y;
            }
        }
        
        // 连接最后一行所有节点到终点
        for (size_t i = 0; i < layerNodes_[1].size(); ++i) {
            if (layerNodes_[1][i].y == lastRowY) {
                int lastRowGlobalIdx = getGlobalNodeIndex(1, static_cast<int>(i));
                if (endGlobalIdx != -1 && lastRowGlobalIdx != -1) {
                    MapNode* lastRowNode = getNodeByGlobalIndex(lastRowGlobalIdx);
                    if (lastRowNode) {
                        lastRowNode->connections.push_back(endGlobalIdx);
                        SDL_Log("Connected last row node (idx %d) to end node (idx %d)", 
                                lastRowGlobalIdx, endGlobalIdx);
                    }
                }
            }
        }
    }
}

void MapExploreState::validatePaths() {
    // 验证路径覆盖所有节点
    SDL_Log("Validating path coverage...");
    
    if (hasPathFromStartToBoss()) {
        SDL_Log("✓ Path from start to boss exists");
    } else {
        SDL_Log("✗ Warning: No path from start to boss");
    }
    
    // 验证所有节点都被路径覆盖
    int totalNodes = getTotalNodeCount();
    int reachableFromStart = 0;
    int reachableFromEnd = 0;
    
    for (int i = 0; i < totalNodes; ++i) {
        if (canReachFromStart(i)) {
            reachableFromStart++;
        }
        if (canReachFromEnd(i)) {
            reachableFromEnd++;
        }
    }
    
    SDL_Log("Path coverage from start: %d/%d nodes", reachableFromStart, totalNodes);
    SDL_Log("Path coverage from end: %d/%d nodes", reachableFromEnd, totalNodes);
    
    if (reachableFromStart == totalNodes) {
        SDL_Log("✓ All nodes are covered by paths from start");
    } else {
        SDL_Log("✗ Warning: %d nodes are not covered by paths from start", totalNodes - reachableFromStart);
    }
    
    if (reachableFromEnd == totalNodes) {
        SDL_Log("✓ All nodes are covered by paths from end");
    } else {
        SDL_Log("✗ Warning: %d nodes are not covered by paths from end", totalNodes - reachableFromEnd);
    }
    
    // 计算路径数量（简化版本，避免崩溃）
    int pathCount = countPathsFromStartToEnd();
    SDL_Log("Total paths from start to end: %d", pathCount);
    
    if (reachableFromStart == totalNodes && reachableFromEnd == totalNodes) {
        SDL_Log("✓ Perfect path coverage achieved! (%d paths)", pathCount);
    } else {
        SDL_Log("✗ Path coverage issues detected");
    }
}

bool MapExploreState::canReachFromStart(int targetNodeIndex) {
    // 使用BFS检查从起点是否能到达目标节点
    std::vector<bool> visited(getTotalNodeCount(), false);
    std::queue<int> queue;
    
    // 从起点开始
    int startGlobalIdx = getGlobalNodeIndex(0, 0);
    if (startGlobalIdx == -1) return false;
    
    queue.push(startGlobalIdx);
    visited[startGlobalIdx] = true;
    
    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();
        
        if (current == targetNodeIndex) {
            return true; // 找到目标节点
        }
        
        // 检查当前节点的所有连接
        MapNode* currentNode = getNodeByGlobalIndex(current);
        if (currentNode) {
            for (int nextNode : currentNode->connections) {
                if (nextNode >= 0 && nextNode < getTotalNodeCount() && !visited[nextNode]) {
                    visited[nextNode] = true;
                    queue.push(nextNode);
                }
            }
        }
    }
    
    return false; // 无法到达目标节点
}

bool MapExploreState::canReachFromEnd(int targetNodeIndex) {
    // 使用BFS检查从终点是否能到达目标节点（反向遍历）
    std::vector<bool> visited(getTotalNodeCount(), false);
    std::queue<int> queue;
    
    // 从终点开始
    int endGlobalIdx = getGlobalNodeIndex(numLayers_, 0);
    if (endGlobalIdx == -1) return false;
    
    queue.push(endGlobalIdx);
    visited[endGlobalIdx] = true;
    
    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();
        
        if (current == targetNodeIndex) {
            return true; // 找到目标节点
        }
        
        // 反向遍历：找到所有指向当前节点的节点
        for (int i = 0; i < getTotalNodeCount(); ++i) {
            if (!visited[i]) {
                MapNode* node = getNodeByGlobalIndex(i);
                if (node) {
                    // 检查这个节点是否连接到当前节点
                    for (int connectedNode : node->connections) {
                        if (connectedNode == current) {
                            visited[i] = true;
                            queue.push(i);
                            break;
                        }
                    }
                }
            }
        }
    }
    
    return false; // 无法到达目标节点
}

int MapExploreState::countPathsFromStartToEnd() {
    // 使用DFS计算从起点到终点的路径数量，添加安全检查
    int startGlobalIdx = getGlobalNodeIndex(0, 0);
    int endGlobalIdx = getGlobalNodeIndex(numLayers_, 0);
    
    if (startGlobalIdx == -1 || endGlobalIdx == -1) {
        SDL_Log("Warning: Invalid start or end node indices");
        return 0;
    }
    
    int totalNodes = getTotalNodeCount();
    if (totalNodes > 100) {
        SDL_Log("Warning: Too many nodes (%d), limiting path count to prevent stack overflow", totalNodes);
        return 1; // 返回1表示至少有一条路径，避免复杂计算
    }
    
    std::vector<bool> visited(totalNodes, false);
    int pathCount = dfsCountPaths(startGlobalIdx, endGlobalIdx, visited, 0);
    
    SDL_Log("Path count calculation completed: %d paths", pathCount);
    return pathCount;
}

int MapExploreState::dfsCountPaths(int current, int target, std::vector<bool>& visited, int depth) {
    // 防止递归过深
    if (depth > 20) {
        SDL_Log("Warning: Recursion depth limit reached (%d), stopping path count", depth);
        return 0;
    }
    
    if (current == target) {
        return 1; // 找到一条路径
    }
    
    if (current < 0 || current >= getTotalNodeCount()) {
        SDL_Log("Warning: Invalid current node index: %d", current);
        return 0;
    }
    
    visited[current] = true;
    int pathCount = 0;
    
    MapNode* currentNode = getNodeByGlobalIndex(current);
    if (currentNode) {
        for (int nextNode : currentNode->connections) {
            if (nextNode >= 0 && nextNode < getTotalNodeCount() && !visited[nextNode]) {
                pathCount += dfsCountPaths(nextNode, target, visited, depth + 1);
                
                // 限制路径数量，防止计算时间过长
                if (pathCount > 100) {
                    SDL_Log("Warning: Path count limit reached (%d), stopping calculation", pathCount);
                    visited[current] = false;
                    return pathCount;
                }
            }
        }
    }
    
    visited[current] = false; // 回溯
    return pathCount;
}

void MapExploreState::connectNodes(int fromLayer, int fromIndex, int toLayer, int toIndex) {
    // 安全检查
    if (fromLayer < 0 || fromLayer >= static_cast<int>(layerNodes_.size()) ||
        toLayer < 0 || toLayer >= static_cast<int>(layerNodes_.size()) ||
        fromIndex < 0 || toIndex < 0) {
        return;
    }
    
    if (fromIndex >= static_cast<int>(layerNodes_[fromLayer].size()) ||
        toIndex >= static_cast<int>(layerNodes_[toLayer].size())) {
        return;
    }
    
    int globalFromIdx = getGlobalNodeIndex(fromLayer, fromIndex);
    int globalToIdx = getGlobalNodeIndex(toLayer, toIndex);
    
    if (globalFromIdx != -1 && globalToIdx != -1) {
        MapNode* fromNode = getNodeByGlobalIndex(globalFromIdx);
        if (fromNode) {
            fromNode->connections.push_back(globalToIdx);
        }
    }
}

void MapExploreState::createBranching(int fromLayer, int fromIndex, int toLayer) {
    // 从单个节点分支到多个节点
    auto& nextLayerNodes = layerNodes_[toLayer];
    
    for (size_t i = 0; i < nextLayerNodes.size(); ++i) {
        connectNodes(fromLayer, fromIndex, toLayer, static_cast<int>(i));
    }
}

void MapExploreState::createMerging(int fromLayer, int toLayer, int toIndex) {
    // 从多个节点合并到单个节点
    auto& currentLayerNodes = layerNodes_[fromLayer];
    
    for (size_t i = 0; i < currentLayerNodes.size(); ++i) {
        connectNodes(fromLayer, static_cast<int>(i), toLayer, toIndex);
    }
}

void MapExploreState::createComplexConnections(int fromLayer, int toLayer) {
    // 多对多连接：避免交叉通路，使用位置选择
    
    // 安全检查
    if (fromLayer < 0 || fromLayer >= static_cast<int>(layerNodes_.size()) ||
        toLayer < 0 || toLayer >= static_cast<int>(layerNodes_.size())) {
        return;
    }
    
    auto& currentLayerNodes = layerNodes_[fromLayer];
    auto& nextLayerNodes = layerNodes_[toLayer];
    
    // 检查层是否为空
    if (currentLayerNodes.empty() || nextLayerNodes.empty()) {
        return;
    }
    
    // 为每个下一层节点选择最合适的父节点，避免交叉
    for (size_t nextIdx = 0; nextIdx < nextLayerNodes.size(); ++nextIdx) {
        // 根据位置选择最合适的父节点，避免交叉通路
        int bestParentIdx = selectBestParentNode(fromLayer, static_cast<int>(nextIdx), static_cast<int>(currentLayerNodes.size()));
        
        if (bestParentIdx != -1 && bestParentIdx < static_cast<int>(currentLayerNodes.size())) {
            connectNodes(fromLayer, bestParentIdx, toLayer, static_cast<int>(nextIdx));
        }
    }
}

int MapExploreState::selectBestParentNode(int layer, int nextNodeIndex, int parentNodeCount) {
    // 根据位置选择最合适的父节点，避免交叉通路
    // 使用位置映射规则：将下一层节点按位置映射到上一层节点
    
    // 安全检查
    if (parentNodeCount <= 0 || nextNodeIndex < 0) {
        return 0;
    }
    
    if (parentNodeCount == 1) {
        return 0; // 如果上一层只有一个节点，所有下一层节点都连接它
    }
    
    // 安全检查：确保 layer + 1 在有效范围内
    if (layer + 1 >= static_cast<int>(layerNodes_.size())) {
        return 0;
    }
    
    // 计算下一层节点的相对位置
    auto& nextLayerNodes = layerNodes_[layer + 1];
    if (nextNodeIndex >= static_cast<int>(nextLayerNodes.size())) {
        return 0;
    }
    
    // 计算下一层节点的相对位置（0.0 到 1.0）
    float nextNodeRelativePos;
    if (nextLayerNodes.size() == 1) {
        nextNodeRelativePos = 0.5f; // 如果只有一个节点，放在中间
    } else {
        nextNodeRelativePos = static_cast<float>(nextNodeIndex) / (nextLayerNodes.size() - 1);
    }
    
    // 映射到上一层节点，确保不交叉
    // 使用更精确的映射算法，避免交叉通路
    int parentIndex = static_cast<int>(nextNodeRelativePos * (parentNodeCount - 1) + 0.5f);
    parentIndex = std::max(0, std::min(parentNodeCount - 1, parentIndex));
    
    // 确保每个下一层节点只连接一个父节点，避免交叉
    return parentIndex;
}

int MapExploreState::getGlobalNodeIndex(int layer, int localIndex) const {
    if (layer < 0 || layer >= static_cast<int>(layerNodes_.size()) || 
        localIndex < 0 || localIndex >= static_cast<int>(layerNodes_[layer].size())) {
        return -1;
    }
    
    int globalIndex = 0;
    for (int i = 0; i < layer; ++i) {
        globalIndex += static_cast<int>(layerNodes_[i].size());
    }
    return globalIndex + localIndex;
}

MapExploreState::MapNode* MapExploreState::getNodeByGlobalIndex(int globalIndex) {
    int currentIndex = 0;
    for (auto& layer : layerNodes_) {
        if (globalIndex < currentIndex + static_cast<int>(layer.size())) {
            int localIndex = globalIndex - currentIndex;
            return &layer[localIndex];
        }
        currentIndex += static_cast<int>(layer.size());
    }
    return nullptr;
}

const MapExploreState::MapNode* MapExploreState::getNodeByGlobalIndex(int globalIndex) const {
    int currentIndex = 0;
    for (const auto& layer : layerNodes_) {
        if (globalIndex < currentIndex + static_cast<int>(layer.size())) {
            int localIndex = globalIndex - currentIndex;
            return &layer[localIndex];
        }
        currentIndex += static_cast<int>(layer.size());
    }
    return nullptr;
}

int MapExploreState::getTotalNodeCount() const {
    int total = 0;
    for (const auto& layer : layerNodes_) {
        total += static_cast<int>(layer.size());
    }
    return total;
}

void MapExploreState::connectStartAndBoss() {
    // 起点连接到第1层的一部分节点（控制连接数量）
    if (!layerNodes_[0].empty() && !layerNodes_[1].empty()) {
        std::random_device rd;
        std::mt19937 gen(rd());
        
        int startGlobalIdx = getGlobalNodeIndex(0, 0);
        
        // 优先连接到第1层的中心节点（主要路径）
        int centerIndex = static_cast<int>(layerNodes_[1].size()) / 2;
        int targetGlobalIdx = getGlobalNodeIndex(1, centerIndex);
        if (startGlobalIdx != -1 && targetGlobalIdx != -1) {
            getNodeByGlobalIndex(startGlobalIdx)->connections.push_back(targetGlobalIdx);
        }
        
        // 如果第1层有多个节点，有较小概率连接到其他节点
        if (layerNodes_[1].size() > 1) {
            std::uniform_int_distribution<> extraConnection(0, 100);
            
            // 20%的概率连接到其他节点（降低概率）
            if (extraConnection(gen) < 20) {
                for (size_t i = 0; i < layerNodes_[1].size(); ++i) {
                    if (static_cast<int>(i) != centerIndex) {
                        int targetGlobalIdx = getGlobalNodeIndex(1, static_cast<int>(i));
                        if (startGlobalIdx != -1 && targetGlobalIdx != -1) {
                            getNodeByGlobalIndex(startGlobalIdx)->connections.push_back(targetGlobalIdx);
                        }
                        break; // 只连接一个额外的节点
                    }
                }
            }
        }
    }
    
    // 最后一层的所有节点都连接到BOSS（确保所有路径都能到达终点）
    if (!layerNodes_[numLayers_ - 1].empty() && !layerNodes_[numLayers_].empty()) {
        int bossGlobalIdx = getGlobalNodeIndex(numLayers_, 0);
        
        // 连接最后一层的所有节点到BOSS
        for (size_t i = 0; i < layerNodes_[numLayers_ - 1].size(); ++i) {
            int sourceGlobalIdx = getGlobalNodeIndex(numLayers_ - 1, static_cast<int>(i));
            if (bossGlobalIdx != -1 && sourceGlobalIdx != -1) {
                // 单向连接：最后一层节点连接到BOSS
                getNodeByGlobalIndex(sourceGlobalIdx)->connections.push_back(bossGlobalIdx);
            }
        }
    }
}

bool MapExploreState::hasPathFromStartToBoss() {
    // 使用BFS检查路径
    std::vector<bool> visited(getTotalNodeCount(), false);
    std::queue<int> queue;
    
    int startGlobalIdx = getGlobalNodeIndex(0, 0);
    int bossGlobalIdx = getGlobalNodeIndex(numLayers_, 0);
    
    if (startGlobalIdx == -1 || bossGlobalIdx == -1) return false;
    
    queue.push(startGlobalIdx);
    visited[startGlobalIdx] = true;
    
    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();
        
        if (current == bossGlobalIdx) return true;
        
        MapNode* node = getNodeByGlobalIndex(current);
        if (node) {
            for (int connection : node->connections) {
                if (connection < static_cast<int>(visited.size()) && !visited[connection]) {
                    visited[connection] = true;
                    queue.push(connection);
                }
            }
        }
    }
    
    return false;
}

bool MapExploreState::allPathsFromStartReachBoss() {
    // 检查从起点延伸的每一条路径都能到达BOSS
    int startGlobalIdx = getGlobalNodeIndex(0, 0);
    int bossGlobalIdx = getGlobalNodeIndex(numLayers_, 0);
    
    if (startGlobalIdx == -1 || bossGlobalIdx == -1) return false;
    
    // 使用DFS检查所有从起点出发的路径
    std::vector<bool> visited(getTotalNodeCount(), false);
    return dfsCheckAllPaths(startGlobalIdx, bossGlobalIdx, visited);
}

bool MapExploreState::dfsCheckAllPaths(int current, int target, std::vector<bool>& visited) {
    if (current == target) return true;
    
    visited[current] = true;
    
    MapNode* node = getNodeByGlobalIndex(current);
    if (!node) return false;
    
    // 如果当前节点没有连接，说明路径断了
    if (node->connections.empty()) return false;
    
    // 检查所有连接是否都能到达目标
    for (int connection : node->connections) {
        if (connection < static_cast<int>(visited.size()) && !visited[connection]) {
            if (!dfsCheckAllPaths(connection, target, visited)) {
                return false; // 如果任何一条路径不能到达目标，返回false
            }
        }
    }
    
    return true;
}

void MapExploreState::validateAndTrimPaths() {
    // 检查从起点到BOSS点是否存在至少一条完整路径
    if (!hasPathFromStartToBoss()) {
        SDL_Log("Warning: No path from start to boss found, regenerating...");
        generateLayeredMap(true); // 重新生成，需要构建偏移
        s_mapGenerated_ = true; // 确保标志被设置
        return;
    }
    
    // 检查从起点延伸的每一条路径都能到达BOSS
    if (!allPathsFromStartReachBoss()) {
        SDL_Log("Warning: Not all paths from start reach boss, regenerating...");
        generateLayeredMap(true); // 重新生成，需要构建偏移
        s_mapGenerated_ = true; // 确保标志被设置
        return;
    }
    
    // 移除所有"孤儿节点"（即无法从起点到达，或者无法到达BOSS的节点）
    removeOrphanNodes();
}

void MapExploreState::removeOrphanNodes() {
    // 标记所有从起点可达的节点
    std::vector<bool> reachable(getTotalNodeCount(), false);
    std::queue<int> queue;
    
    int startGlobalIdx = getGlobalNodeIndex(0, 0);
    if (startGlobalIdx != -1) {
        queue.push(startGlobalIdx);
        reachable[startGlobalIdx] = true;
    }
    
    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();
        
        MapNode* node = getNodeByGlobalIndex(current);
        if (node) {
            for (int connection : node->connections) {
                if (connection < static_cast<int>(reachable.size()) && !reachable[connection]) {
                    reachable[connection] = true;
                    queue.push(connection);
                }
            }
        }
    }
    
    // 移除不可达的节点
    for (int layer = 0; layer <= numLayers_; ++layer) {
        auto& layerNodes = layerNodes_[layer];
        for (auto it = layerNodes.begin(); it != layerNodes.end();) {
            int globalIdx = getGlobalNodeIndex(layer, static_cast<int>(it - layerNodes.begin()));
            if (globalIdx != -1 && !reachable[globalIdx]) {
                it = layerNodes.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void MapExploreState::renderMap(SDL_Renderer* renderer) {
    // 渲染连接线
    for (int layer = 0; layer <= numLayers_; ++layer) {
        for (size_t i = 0; i < layerNodes_[layer].size(); ++i) {
            const MapNode& node = layerNodes_[layer][i];
            for (int connection : node.connections) {
                renderConnection(renderer, getGlobalNodeIndex(layer, static_cast<int>(i)), connection);
            }
        }
    }
    
    // 渲染节点
    for (int layer = 0; layer <= numLayers_; ++layer) {
        for (size_t i = 0; i < layerNodes_[layer].size(); ++i) {
            int globalIndex = getGlobalNodeIndex(layer, static_cast<int>(i));
            renderNode(renderer, layerNodes_[layer][i], globalIndex);
        }
    }

    // 绘制移动中的玩家位置（小白点）
    if (isMoving_) {
        // 插值世界坐标
        float px = moveStartPos_.x + (moveEndPos_.x - moveStartPos_.x) * moveT_;
        float py = moveStartPos_.y + (moveEndPos_.y - moveStartPos_.y) * moveT_;
        int sx = mapOffsetX_ + static_cast<int>(px);
        int sy = mapOffsetY_ + static_cast<int>(py + scrollY_);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect r{ sx - 10, sy - 10, 20, 20 };  // 从12x12增大到20x20
        SDL_RenderFillRect(renderer, &r);
        
        // 绘制移动中的倒三角标注
        drawTriangleMarker(renderer, sx, sy - 25, 30);
    }
    
    // 绘制玩家移动动画（Boss战胜利后的向上移动）
    if (isPlayerMoving_) {
        float t = std::min(1.0f, playerMoveTime_ / playerMoveDuration_);
        float tEase = (1.0f - std::cos(3.1415926f * t)) * 0.5f;
        
        // 插值世界坐标
        float px = playerMoveStartX_ + (playerMoveEndX_ - playerMoveStartX_) * tEase;
        float py = playerMoveStartY_ + (playerMoveEndY_ - playerMoveStartY_) * tEase;
        int sx = mapOffsetX_ + static_cast<int>(px);
        int sy = mapOffsetY_ + static_cast<int>(py + scrollY_);
        
        // 绘制更大的白色圆点表示玩家移动
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect r{ sx - 15, sy - 15, 30, 30 };  // 更大的圆点
        SDL_RenderFillRect(renderer, &r);
        
        // 添加一个发光效果
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
        SDL_Rect glow{ sx - 20, sy - 20, 40, 40 };
        SDL_RenderFillRect(renderer, &glow);
        
        // 绘制移动中的倒三角标注
        drawTriangleMarker(renderer, sx, sy - 25, 30);
    }
}

void MapExploreState::renderNode(SDL_Renderer* renderer, const MapNode& node, int index) {
    // 应用视图偏移与滚动
    int x, y; getScreenXYForGlobalIndex(index, x, y);
    
    // 检查节点状态（添加安全检查）
    bool isCurrentNode = (playerCurrentNode_ != -1 && index == playerCurrentNode_);
    bool isAccessible = (playerCurrentNode_ != -1 && isNodeAccessible(index));
    
    // 不再绘制节点的颜色方块，只保留图标
    
    // 渲染事件图标
    renderEventIcon(renderer, node, x, y);
    
    // 如果是当前位置，在节点上方绘制倒三角标注
    if (isCurrentNode) {
        drawTriangleMarker(renderer, x, y - node.size/2 - 25, 30);
    }
}

void MapExploreState::assignRowEventLabels() {
    // 为每一行的节点分配不重复的事件名称
    if (layerNodes_.size() <= 1) return;
    
    // 第四层有固定的节点标签，不需要重新分配
    if (currentMapLayer_ == 4) {
        std::cout << "[ASSIGN LABELS] 第四层使用固定标签，跳过随机分配" << std::endl;
        return;
    }
    
    // 只处理中间层（layer 1），因为其他层有固定的标签
    auto& nodes = layerNodes_[1];
    if (nodes.empty()) return;

    // 事件池（带权重）
    struct EventWithWeight {
        std::string name;
        int weight;
        bool allowedInFirstLayer;
    };
    
    static const std::vector<EventWithWeight> shopEvents = {
        {u8"以物易物", 1, true}, // 每层最多1个
        {u8"记忆修复", 1, true},
        {u8"寻物人", 1, true},
        {u8"文心试炼", 1, false}, // 不在第一层出现
        {u8"墨坊", 1, true} // 每层最多1个
    };
    
    static const std::vector<EventWithWeight> eventEvents = {
        {u8"文脉传承", 2, true},
        {u8"意境刻画", 2, true},
        {u8"淬炼", 2, true},
        {u8"墨宝拾遗", 2, true},
        {u8"墨鬼", 1, false}, // 不在第一层出现，权重减半
        {u8"焚书", 1, false}, // 不在第一层出现，权重减半
        {u8"合卷", 1, false}  // 不在第一层出现，权重减半
    };
    
    static const std::vector<EventWithWeight> battleEvents = {
        {u8"诗剑之争", 1, true},
        {u8"意境之斗", 1, true}
    };

    // 按y分组
    std::map<int, std::vector<int>> rows;
    for (size_t i = 0; i < nodes.size(); ++i) rows[nodes[i].y].push_back(static_cast<int>(i));

    std::random_device rd; std::mt19937 gen(rd());
    
    // 跟踪已使用的事件（用于限制"以物易物"和"墨坊"每层最多1个）
    std::set<std::string> usedEvents;
    
    for (auto& kv : rows) {
        auto& idxs = kv.second;
        if (idxs.empty()) continue;
        MapNode::NodeType rowType = nodes[idxs[0]].type;

        // 特殊处理：第一层的第一行必定是"以物易物"
        bool isFirstLayerFirstRow = (currentMapLayer_ == 1 && kv.first == -220); // 第一行的Y坐标是-220
        
        // 根据当前地图层和节点类型构建可用事件池
        std::vector<std::string> availableEvents;
        const std::vector<EventWithWeight>* eventList = nullptr;
        
        switch (rowType) {
            case MapNode::NodeType::SHOP: eventList = &shopEvents; break;
            case MapNode::NodeType::EVENT: eventList = &eventEvents; break;
            case MapNode::NodeType::ELITE: eventList = &battleEvents; break;
            default: break;
        }
        
        if (eventList) {
            // 特殊处理：第一层第一行强制分配"以物易物"
            if (isFirstLayerFirstRow) {
                availableEvents.push_back(u8"以物易物");
            } else {
                for (const auto& event : *eventList) {
                    // 如果是第一层，只允许 allowedInFirstLayer 为 true 的事件
                    if (currentMapLayer_ == 1 && !event.allowedInFirstLayer) {
                        continue;
                    }
                    
                    // 第一层除了第一个节点，其他节点不出现"以物易物"
                    if (currentMapLayer_ == 1 && event.name == u8"以物易物") {
                        continue;
                    }
                    
                    // 检查是否已经使用过（只限制"以物易物"和"墨坊"每层最多1个）
                    if ((event.name == u8"以物易物" || event.name == u8"墨坊") && 
                        usedEvents.find(event.name) != usedEvents.end()) {
                        continue;
                    }
                    
                    // 根据权重添加事件到池中
                    for (int i = 0; i < event.weight; ++i) {
                        availableEvents.push_back(event.name);
                    }
                }
            }
        }
        
        if (availableEvents.empty()) {
            // 如果没有可用事件，使用默认标签而不是清空
            for (int idx : idxs) {
                switch (rowType) {
                    case MapNode::NodeType::SHOP: nodes[idx].label = u8"商店"; break;
                    case MapNode::NodeType::EVENT: nodes[idx].label = u8"事件"; break;
                    case MapNode::NodeType::ELITE: nodes[idx].label = u8"战斗"; break;
                    default: nodes[idx].label = u8"节点"; break;
                }
            }
            continue;
        }
        
        // 从可用事件中随机选择不重复的标签
        std::shuffle(availableEvents.begin(), availableEvents.end(), gen);
        
        // 去重并保持顺序
        std::vector<std::string> uniqueEvents;
        for (const auto& event : availableEvents) {
            if (std::find(uniqueEvents.begin(), uniqueEvents.end(), event) == uniqueEvents.end()) {
                uniqueEvents.push_back(event);
            }
        }
        
        size_t take = std::min(uniqueEvents.size(), idxs.size());
        for (size_t k = 0; k < take; ++k) {
            nodes[idxs[k]].label = uniqueEvents[k];
            
            // 为记忆修复节点添加提示信息
            if (uniqueEvents[k] == u8"记忆修复") {
                // 随机选择提示类型（与MemoryRepairState中的逻辑一致）
                std::uniform_int_distribution<int> hintDist(0, 2);
                int hintType = hintDist(gen);
                if (hintType == 0) {
                    nodes[idxs[k]].label = u8"记忆修复(随机)";
                } else if (hintType == 1) {
                    nodes[idxs[k]].label = u8"记忆修复(已知部族)";
                } else {
                    nodes[idxs[k]].label = u8"记忆修复(已知消耗)";
                }
            }
            
            // 只跟踪"以物易物"和"墨坊"的使用情况
            if (uniqueEvents[k] == u8"以物易物" || uniqueEvents[k] == u8"墨坊") {
                usedEvents.insert(uniqueEvents[k]);
            }
        }
        
        // 多余的节点复用已有事件
        for (size_t k = take; k < idxs.size(); ++k) {
            std::string baseEvent = uniqueEvents[(k - take) % uniqueEvents.size()];
            nodes[idxs[k]].label = baseEvent;
            
            // 为记忆修复节点添加提示信息
            if (baseEvent == u8"记忆修复") {
                // 随机选择提示类型（与MemoryRepairState中的逻辑一致）
                std::uniform_int_distribution<int> hintDist(0, 2);
                int hintType = hintDist(gen);
                if (hintType == 0) {
                    nodes[idxs[k]].label = u8"记忆修复(随机)";
                } else if (hintType == 1) {
                    nodes[idxs[k]].label = u8"记忆修复(已知部族)";
                } else {
                    nodes[idxs[k]].label = u8"记忆修复(已知消耗)";
                }
            }
        }
    }
}

void MapExploreState::renderConnection(SDL_Renderer* renderer, int fromGlobalIndex, int toGlobalIndex) {
    // 使用全局索引获取显示坐标（含随机美化位移）
    int fx, fy; getScreenXYForGlobalIndex(fromGlobalIndex, fx, fy);
    int tx, ty; getScreenXYForGlobalIndex(toGlobalIndex, tx, ty);
    // 使用深灰色粗虚线，在灰白色背景下更清晰
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    drawDashedLine(renderer, fx, fy, tx, ty, 8, 4); // 8像素线段，4像素间隔
}

SDL_Point MapExploreState::gridToScreen(int gridX, int gridY) const {
    int nodeSpacing = 80;
    int mapOffsetX = mapOffsetX_;
    int mapOffsetY = mapOffsetY_;
    return { 
        mapOffsetX + gridX * nodeSpacing, 
        mapOffsetY + gridY * nodeSpacing + scrollY_
    };
}

int MapExploreState::screenToGrid(int screenX, int screenY) const {
    // 查找对应的节点（使用分层结构）
    for (int layer = 0; layer <= numLayers_; ++layer) {
        for (size_t i = 0; i < layerNodes_[layer].size(); ++i) {
            const MapNode& node = layerNodes_[layer][i];
            int sx, sy; getScreenXYForGlobalIndex(getGlobalNodeIndex(layer, static_cast<int>(i)), sx, sy);
            int dx = screenX - sx;
            int dy = screenY - sy;
            if (dx * dx + dy * dy <= node.size * node.size) {
                return getGlobalNodeIndex(layer, static_cast<int>(i));
            }
        }
    }
    return -1;
}

void MapExploreState::getScreenXYForGlobalIndex(int globalIndex, int& sx, int& sy) const {
    const MapNode* node = getNodeByGlobalIndex(globalIndex);
    if (!node) { sx = sy = 0; return; }
    int baseX, baseY; nodeToScreenXY(*node, baseX, baseY);
    SDL_Point extra{0,0};
    if (globalIndex >= 0 && globalIndex < static_cast<int>(nodeDisplayOffset_.size())) {
        extra = nodeDisplayOffset_[globalIndex];
    }
    sx = baseX + extra.x;
    sy = baseY + extra.y;
}

void MapExploreState::buildDisplayOffsets() {
    // 分配与全局节点数量相同的偏移数组
    nodeDisplayOffset_.clear();
    nodeDisplayOffset_.resize(getTotalNodeCount(), SDL_Point{0,0});

    // 按行（相同y的节点）生成轻微随机位移，仅影响显示
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> jx(-40, 40);
    std::uniform_int_distribution<int> jy(-20, 20);

    // 为所有层（除了起点和终点）的节点添加随机位移
    for (int layer = 0; layer <= numLayers_; ++layer) {
        if (layerNodes_.size() <= layer) continue;
        
        for (size_t i = 0; i < layerNodes_[layer].size(); ++i) {
            int g = getGlobalNodeIndex(layer, static_cast<int>(i));
            if (g != -1 && g < static_cast<int>(nodeDisplayOffset_.size())) {
                // 起点和终点保持较小抖动，其他节点正常随机位移
                if ((layer == 0 && i == 0) || (layer == numLayers_ && i == 0)) {
                    // 起点和终点：较小抖动
                    std::uniform_int_distribution<int> smallJx(-10, 10);
                    std::uniform_int_distribution<int> smallJy(-5, 5);
                    nodeDisplayOffset_[g].x = smallJx(gen);
                    nodeDisplayOffset_[g].y = smallJy(gen);
                } else {
                    // 其他节点：正常随机位移
                    nodeDisplayOffset_[g].x = jx(gen);
                    nodeDisplayOffset_[g].y = jy(gen);
                }
            }
        }
    }
}

void MapExploreState::updateScrollBounds() {
    // 计算世界坐标的最小/最大Y（不含偏移与滚动）
    bool inited = false;
    int minY = 0, maxY = 0;
    for (int layer = 0; layer <= numLayers_; ++layer) {
        for (size_t i = 0; i < layerNodes_[layer].size(); ++i) {
            const MapNode& node = layerNodes_[layer][i];
            if (!inited) { minY = maxY = node.y; inited = true; }
            if (node.y < minY) minY = node.y;
            if (node.y > maxY) maxY = node.y;
        }
    }
    minWorldY_ = minY; maxWorldY_ = maxY;
    // 根据可视行数估算屏幕允许范围，由于渲染时会加上mapOffsetY_
    // 计算在当前偏移下，能向下滚动的最大像素：
    // 地图内容高度: (maxWorldY_ - minWorldY_)；
    // 可视高度: screenH_ - mapOffsetY_ - 下边距40；
    int contentHeight = (maxWorldY_ - minWorldY_);
    int visibleHeight = screenH_ - mapOffsetY_ - 40;
    if (visibleHeight < 0) visibleHeight = 0;
    if (contentHeight <= visibleHeight) {
        maxScrollY_ = 0; // 内容不满屏，无需滚动
        scrollY_ = 0;
    } else {
        maxScrollY_ = contentHeight - visibleHeight;
        // 施加上限
        if (maxScrollY_ > maxScrollYCap_) maxScrollY_ = maxScrollYCap_;
        if (scrollY_ < 0) scrollY_ = 0;
        if (scrollY_ > maxScrollY_) scrollY_ = maxScrollY_;
    }
}

void MapExploreState::updateAutoScrollToPlayer() {
    if (playerCurrentNode_ < 0) return;

    // 获取玩家当前节点
    const MapNode* playerNode = getNodeByGlobalIndex(playerCurrentNode_);
    if (!playerNode) return;
    
    // 如果玩家位于Boss战节点，不需要自动滚动
    if (playerNode->type == MapNode::NodeType::BOSS) {
        SDL_Log("Auto scroll: player at BOSS node, no scroll needed");
        return;
    }
    
    // 计算玩家当前屏幕Y坐标
    // 使用与nodeToScreenXY相同的逻辑
    int playerScreenY = mapOffsetY_ + (playerNode->y + scrollY_);
    
    // 目标屏幕Y坐标：680像素
    int targetScreenY = 680;
    
    // 判断玩家是否在第三行及以后
    // 这里使用绝对Y坐标来判断，假设第三行及以后的节点Y坐标大于某个值
    // 或者可以根据玩家的屏幕Y坐标来判断
    bool shouldAutoScroll = (playerScreenY != targetScreenY);
    
    if (!shouldAutoScroll) {
        SDL_Log("Auto scroll: player at Y=%d, screenY=%d, already at target", 
                playerNode->y, playerScreenY);
        return; // 玩家已经在目标位置
    }
    
    // 计算需要的滚动量
    // 目标：让玩家的屏幕Y坐标 = 680
    // 当前玩家屏幕Y = mapOffsetY_ + (playerNode->y + scrollY_)
    // 目标玩家屏幕Y = mapOffsetY_ + (playerNode->y + newScrollY_) = 680
    // 所以：newScrollY = 680 - mapOffsetY_ - playerNode->y
    int newScrollY = targetScreenY - mapOffsetY_ - playerNode->y;
    
    // 应用滚动边界限制
    if (newScrollY < 0) newScrollY = 0;
    if (newScrollY > maxScrollY_) newScrollY = maxScrollY_;
    
    // 更新滚动位置
    int oldScrollY = scrollY_;
    scrollY_ = newScrollY;
    
    // 保存到MapStore
    auto& ms = MapStore::instance();
    ms.scrollY() = scrollY_;
    
    // 验证滚动后的位置
    int newPlayerScreenY = mapOffsetY_ + (playerNode->y + scrollY_);
    
    SDL_Log("Auto scroll: player at Y=%d, oldScrollY=%d, newScrollY=%d, oldScreenY=%d, newScreenY=%d, target=%d", 
            playerNode->y, oldScrollY, scrollY_, playerScreenY, newPlayerScreenY, targetScreenY);
}

void MapExploreState::initializePlayer() {
    // 玩家初始位置在起点
    playerCurrentNode_ = getGlobalNodeIndex(0, 0);
    if (playerCurrentNode_ == -1) {
        SDL_Log("Warning: Could not find start node for player initialization");
        playerCurrentNode_ = -1;
        accessibleNodes_.clear();
        return;
    }
    
    // 验证起点节点是否存在
    MapNode* startNode = getNodeByGlobalIndex(playerCurrentNode_);
    if (!startNode) {
        SDL_Log("Warning: Start node %d does not exist", playerCurrentNode_);
        playerCurrentNode_ = -1;
        accessibleNodes_.clear();
        return;
    }
    
    // 更新可移动节点列表
    updateAccessibleNodes();
    
    SDL_Log("Player initialized at node %d (start node)", playerCurrentNode_);
}

void MapExploreState::updateAccessibleNodes() {
    accessibleNodes_.clear();
    
    if (playerCurrentNode_ == -1) {
        SDL_Log("Warning: Player current node is -1, cannot update accessible nodes");
        return;
    }
    
    // 获取玩家当前节点
    MapNode* currentNode = getNodeByGlobalIndex(playerCurrentNode_);
    if (!currentNode) {
        SDL_Log("Warning: Could not get current node %d", playerCurrentNode_);
        return;
    }
    
    SDL_Log("Current node %d has %zu connections", playerCurrentNode_, currentNode->connections.size());
    
    // 添加当前节点连接的所有节点到可移动列表
    for (int connection : currentNode->connections) {
        if (connection >= 0 && connection < getTotalNodeCount()) {
            // 验证连接节点是否存在
            MapNode* connectedNode = getNodeByGlobalIndex(connection);
            if (connectedNode) {
                accessibleNodes_.push_back(connection);
                SDL_Log("Added accessible node %d (type: %d)", connection, static_cast<int>(connectedNode->type));
            } else {
                SDL_Log("Warning: Connected node %d does not exist", connection);
            }
        } else {
            SDL_Log("Warning: Invalid connection index %d", connection);
        }
    }
    
    SDL_Log("Updated accessible nodes: %zu nodes accessible from node %d", 
            accessibleNodes_.size(), playerCurrentNode_);
}

bool MapExploreState::isNodeAccessible(int nodeIndex) {
    // 安全检查
    if (nodeIndex < 0 || nodeIndex >= getTotalNodeCount()) {
        return false;
    }
    
    // 简单的线性搜索，避免使用std::find
    for (int accessibleNode : accessibleNodes_) {
        if (accessibleNode == nodeIndex) {
            return true;
        }
    }
    return false;
}

void MapExploreState::movePlayerToNode(int nodeIndex) {
    if (!isNodeAccessible(nodeIndex)) {
        SDL_Log("Warning: Cannot move to node %d, not accessible", nodeIndex);
        return;
    }
    
    playerCurrentNode_ = nodeIndex;
    updateAccessibleNodes();
    
    // 检查是否需要自动滚动到玩家位置
    updateAutoScrollToPlayer();
    
    // 将当前位置与可达节点写入全局存储，保持跨界面一致
    auto& ms = MapStore::instance();
    ms.playerCurrentNode() = playerCurrentNode_;
    ms.accessibleNodes() = accessibleNodes_;
    
    // 根据节点类型和标签进入对应界面
    const MapNode* node = getNodeByGlobalIndex(nodeIndex);
    if (node) {
        // 检查是否为Boss战节点
        if (node->type == MapNode::NodeType::BOSS) {
            // 根据Boss节点标签进入对应的Boss战
            std::cout << "[BOSS NODE] 玩家到达Boss战节点，准备进入Boss战" << std::endl;
            std::cout << "[BOSS NODE] 节点标签: '" << node->label << "'" << std::endl;
            if (!node->label.empty()) {
                if (node->label == u8"矿工Boss" || node->label == u8"矿工") {
                    std::cout << "[BOSS NODE] 匹配矿工Boss，设置pendingGoMinerBoss_" << std::endl;
                    pendingGoMinerBoss_ = true;
                } else if (node->label == u8"渔夫Boss" || node->label == u8"渔夫") {
                    std::cout << "[BOSS NODE] 匹配渔夫Boss，设置pendingGoFishermanBoss_" << std::endl;
                    pendingGoFishermanBoss_ = true;
                } else if (node->label == u8"猎人Boss" || node->label == u8"猎人") {
                    std::cout << "[BOSS NODE] 匹配猎人Boss，设置pendingGoHunterBoss_" << std::endl;
                    pendingGoHunterBoss_ = true;
                } else if (node->label == u8"最终Boss") {
                    std::cout << "[BOSS NODE] 匹配最终Boss，设置pendingGoFinalBoss_" << std::endl;
                    pendingGoFinalBoss_ = true;
                } else {
                    // 默认Boss战
                    std::cout << "[BOSS NODE] 未匹配任何Boss，设置pendingGoBattle_" << std::endl;
                    pendingGoBattle_ = true;
                }
            } else {
                // 没有标签的Boss节点，使用默认Boss战
                std::cout << "[BOSS NODE] 节点标签为空，设置pendingGoBattle_" << std::endl;
                pendingGoBattle_ = true;
            }
            return;
        }
        
        // 根据节点标签进入对应界面
        if (!node->label.empty()) {
            if (node->label == u8"诗剑之争" || node->label == u8"意境之斗") {
                currentBattleType_ = node->label;  // 存储战斗类型
                pendingGoBattle_ = true;
            } else if (node->label == u8"以物易物") {
                pendingGoBarter_ = true;
            } else if (node->label == u8"意境刻画") {
                pendingGoEngrave_ = true;
            } else if (node->label == u8"文脉传承") {
                pendingGoHeritage_ = true;
            } else if (node->label == u8"墨坊") {
                pendingGoInkWorkshop_ = true;
            } else if (node->label == u8"墨鬼") {
                pendingGoInkGhost_ = true;
            } else if (node->label == u8"记忆修复" || 
                       node->label == u8"记忆修复(随机)" || 
                       node->label == u8"记忆修复(已知部族)" || 
                       node->label == u8"记忆修复(已知消耗)") {
                // 根据地图标签设置记忆修复的提示类型
                if (node->label == u8"记忆修复(随机)") {
                    MemoryRepairState::setMapHintType(MemoryRepairState::BackHintType::Unknown);
                } else if (node->label == u8"记忆修复(已知部族)") {
                    MemoryRepairState::setMapHintType(MemoryRepairState::BackHintType::KnownTribe);
                } else if (node->label == u8"记忆修复(已知消耗)") {
                    MemoryRepairState::setMapHintType(MemoryRepairState::BackHintType::KnownCost);
                } else {
                    // 默认情况（u8"记忆修复"）随机选择
                    MemoryRepairState::setMapHintType(MemoryRepairState::BackHintType::Unknown);
                }
                pendingGoMemoryRepair_ = true;
            } else if (node->label == u8"墨宝拾遗") {
                pendingGoRelicPickup_ = true;
            } else if (node->label == u8"寻物人") {
                pendingGoSeeker_ = true;
            } else if (node->label == u8"淬炼") {
                pendingGoTemper_ = true;
            }
            // 其他事件（如"文心试炼"、"墨鬼"、"焚书"、"合卷"）暂时不处理
        }
    }
    
    SDL_Log("Player moved to node %d with label '%s'", nodeIndex, node ? node->label.c_str() : "null");
}

void MapExploreState::startMoveAnimation(int targetNodeIndex) {
    if (!isNodeAccessible(targetNodeIndex)) return;
    if (playerCurrentNode_ == -1) return;
    int sx0, sy0; getScreenXYForGlobalIndex(playerCurrentNode_, sx0, sy0);
    int sx1, sy1; getScreenXYForGlobalIndex(targetNodeIndex, sx1, sy1);
    // 转换为世界坐标反推：world = screen - offsets
    moveStartPos_.x = static_cast<float>(sx0 - mapOffsetX_);
    moveStartPos_.y = static_cast<float>(sy0 - mapOffsetY_ - scrollY_);
    moveEndPos_.x = static_cast<float>(sx1 - mapOffsetX_);
    moveEndPos_.y = static_cast<float>(sy1 - mapOffsetY_ - scrollY_);
    moveFromNode_ = playerCurrentNode_;
    moveToNode_ = targetNodeIndex;
    moveT_ = 0.0f;
    isMoving_ = true;
}

void MapExploreState::renderLeftSideUI(SDL_Renderer* renderer) {
    if (!smallFont_) return;
    
    // 左侧UI区域位置 - 更大的显示区域
    int leftX = 20;
    int startY = 100;
    int lineHeight = 40;  // 增加行高
    int spacing = 50;     // 增加间距
    
    // 背景框 - 更大
    SDL_Rect bgRect = { 10, 80, 250, 400 };
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 180); // 半透明浅灰色背景
    SDL_RenderFillRect(renderer, &bgRect);
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255); // 深灰色边框
    SDL_RenderDrawRect(renderer, &bgRect);
    
    // 标题 - 使用更大的字体
    SDL_Color titleColor{ 0, 0, 0, 255 }; // 纯黑色，更明显
    SDL_Surface* titleSurface = TTF_RenderUTF8_Blended(smallFont_, u8"当前状态", titleColor);
    if (titleSurface) {
        SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
        if (titleTexture) {
            SDL_Rect titleRect = { leftX, startY, titleSurface->w, titleSurface->h };
            SDL_RenderCopy(renderer, titleTexture, nullptr, &titleRect);
            SDL_DestroyTexture(titleTexture);
        }
        SDL_FreeSurface(titleSurface);
    }
    
    int currentY = startY + lineHeight + 20;
    
    // 显示道具数量和种类
    auto& itemStore = ItemStore::instance();
    const auto& items = itemStore.items();
    
    SDL_Color itemColor{ 0, 100, 200, 255 }; // 深蓝色，更明显
    std::string itemText = u8"道具总数: " + std::to_string(itemStore.totalCount());
    SDL_Surface* itemSurface = TTF_RenderUTF8_Blended(smallFont_, itemText.c_str(), itemColor);
    if (itemSurface) {
        SDL_Texture* itemTexture = SDL_CreateTextureFromSurface(renderer, itemSurface);
        if (itemTexture) {
            SDL_Rect itemRect = { leftX, currentY, itemSurface->w, itemSurface->h };
            SDL_RenderCopy(renderer, itemTexture, nullptr, &itemRect);
            SDL_DestroyTexture(itemTexture);
        }
        SDL_FreeSurface(itemSurface);
    }
    currentY += lineHeight;
    
    // 显示具体道具种类
    for (const auto& item : items) {
        if (item.count > 0) {
            std::string itemDetailText = item.name + u8": " + std::to_string(item.count);
            SDL_Surface* itemDetailSurface = TTF_RenderUTF8_Blended(smallFont_, itemDetailText.c_str(), itemColor);
            if (itemDetailSurface) {
                SDL_Texture* itemDetailTexture = SDL_CreateTextureFromSurface(renderer, itemDetailSurface);
                if (itemDetailTexture) {
                    SDL_Rect itemDetailRect = { leftX + 20, currentY, itemDetailSurface->w, itemDetailSurface->h };
                    SDL_RenderCopy(renderer, itemDetailTexture, nullptr, &itemDetailRect);
                    SDL_DestroyTexture(itemDetailTexture);
                }
                SDL_FreeSurface(itemDetailSurface);
            }
            currentY += lineHeight;
        }
    }
    currentY += spacing;
    
    // 显示文脉数量
    auto& wenMaiStore = WenMaiStore::instance();
    int totalWenMai = wenMaiStore.get();
    
    SDL_Color wenMaiColor{ 200, 0, 0, 255 }; // 深红色，更明显
    std::string wenMaiText = u8"文脉: " + std::to_string(totalWenMai);
    SDL_Surface* wenMaiSurface = TTF_RenderUTF8_Blended(smallFont_, wenMaiText.c_str(), wenMaiColor);
    if (wenMaiSurface) {
        SDL_Texture* wenMaiTexture = SDL_CreateTextureFromSurface(renderer, wenMaiSurface);
        if (wenMaiTexture) {
            SDL_Rect wenMaiRect = { leftX, currentY, wenMaiSurface->w, wenMaiSurface->h };
            SDL_RenderCopy(renderer, wenMaiTexture, nullptr, &wenMaiRect);
            SDL_DestroyTexture(wenMaiTexture);
        }
        SDL_FreeSurface(wenMaiSurface);
    }

    // 在文脉下方显示当前层环境提示
    currentY += lineHeight;
    auto& ms = MapStore::instance();
    std::string biome;
    if ((int)ms.layerBiomes().size() > currentMapLayer_) {
        biome = ms.layerBiomes()[currentMapLayer_];
    }
    if (!biome.empty()) {
        SDL_Color tipColor{ 0, 150, 0, 255 }; // 深绿色，更明显
        std::string tip = u8"当前层环境：" + biome;
        SDL_Surface* tipSurface = TTF_RenderUTF8_Blended(smallFont_, tip.c_str(), tipColor);
        if (tipSurface) {
            SDL_Texture* tipTexture = SDL_CreateTextureFromSurface(renderer, tipSurface);
            if (tipTexture) {
                SDL_Rect tipRect = { leftX, currentY, tipSurface->w, tipSurface->h };
                SDL_RenderCopy(renderer, tipTexture, nullptr, &tipRect);
                SDL_DestroyTexture(tipTexture);
            }
            SDL_FreeSurface(tipSurface);
        }
    }
}

// 启动玩家移动动画
void MapExploreState::startPlayerMoveAnimation(int fromNode, int toNode) {
    std::cout << "[PLAYER MOVE] 启动玩家移动动画: " << fromNode << " -> " << toNode << std::endl;
    
    isPlayerMoving_ = true;
    playerMoveTime_ = 0.0f;
    playerMoveFromNode_ = fromNode;
    playerMoveToNode_ = toNode;
    
    // 获取起始和结束坐标
    const MapNode* fromNodePtr = getNodeByGlobalIndex(fromNode);
    const MapNode* toNodePtr = getNodeByGlobalIndex(toNode);
    
    if (fromNodePtr && toNodePtr) {
        playerMoveStartX_ = static_cast<float>(fromNodePtr->x);
        playerMoveStartY_ = static_cast<float>(fromNodePtr->y);
        playerMoveEndX_ = static_cast<float>(toNodePtr->x);
        playerMoveEndY_ = static_cast<float>(toNodePtr->y);
        
        std::cout << "[PLAYER MOVE] 起始坐标: (" << playerMoveStartX_ << ", " << playerMoveStartY_ << ")" << std::endl;
        std::cout << "[PLAYER MOVE] 结束坐标: (" << playerMoveEndX_ << ", " << playerMoveEndY_ << ")" << std::endl;
    }
}

// 更新玩家移动动画
void MapExploreState::updatePlayerMoveAnimation(float dt) {
    if (!isPlayerMoving_) return;
    
    playerMoveTime_ += dt;
    float t = std::min(1.0f, playerMoveTime_ / playerMoveDuration_);
    
    // 使用缓动函数
    float tEase = (1.0f - std::cos(3.1415926f * t)) * 0.5f;
    
    if (t >= 1.0f) {
        // 动画完成
        isPlayerMoving_ = false;
        std::cout << "[PLAYER MOVE] 移动动画完成" << std::endl;
        
        // 正式移动玩家到目标节点
        if (playerMoveToNode_ != -1) {
            movePlayerToNode(playerMoveToNode_);
            std::cout << "[PLAYER MOVE] 玩家已移动到节点: " << playerMoveToNode_ << std::endl;
            
            // 检查是否到达了向上道路节点，如果是，则进入下一层
            const MapNode* targetNode = getNodeByGlobalIndex(playerMoveToNode_);
            if (targetNode && targetNode->label == u8"通往下一层") {
                std::cout << "[PLAYER MOVE] 到达向上道路节点，准备进入下一层" << std::endl;
                
                // 进入下一层
                if (currentMapLayer_ < 4) {
                    currentMapLayer_++;
                    std::cout << "[PLAYER MOVE] 进入第" << currentMapLayer_ << "层" << std::endl;
                    
                    // 生成新层的地图
                    generateMapForLayer(currentMapLayer_);
                    s_mapGenerated_ = true;
                    
                    // 更新全局当前层
                    auto& ms = MapStore::instance();
                    ms.currentMapLayer() = currentMapLayer_;
                    
                    // 将新地图数据写入全局存储
                    ms.layerNodes().clear();
                    ms.layerNodes().resize(layerNodes_.size());
                    for (size_t i = 0; i < layerNodes_.size(); ++i) {
                        ms.layerNodes()[i].clear();
                        ms.layerNodes()[i].reserve(layerNodes_[i].size());
                        for (size_t j = 0; j < layerNodes_[i].size(); ++j) {
                            const auto& n = layerNodes_[i][j];
                            MapStore::MapNodeData d;
                            d.layer = n.layer; d.x = n.x; d.y = n.y; d.size = n.size; d.label = n.label;
                            d.visited = n.visited; d.accessible = n.accessible; d.connections = n.connections;
                            // 保存随机位移数据
                            int globalIdx = getGlobalNodeIndex(static_cast<int>(i), static_cast<int>(j));
                            if (globalIdx >= 0 && globalIdx < static_cast<int>(nodeDisplayOffset_.size())) {
                                d.displayOffsetX = nodeDisplayOffset_[globalIdx].x;
                                d.displayOffsetY = nodeDisplayOffset_[globalIdx].y;
                            }
                            switch (n.type) {
                            case MapNode::NodeType::START: d.type = MapStore::MapNodeData::NodeType::START; break;
                            case MapNode::NodeType::NORMAL: d.type = MapStore::MapNodeData::NodeType::NORMAL; break;
                            case MapNode::NodeType::BOSS: d.type = MapStore::MapNodeData::NodeType::BOSS; break;
                            case MapNode::NodeType::ELITE: d.type = MapStore::MapNodeData::NodeType::ELITE; break;
                            case MapNode::NodeType::SHOP: d.type = MapStore::MapNodeData::NodeType::SHOP; break;
                            case MapNode::NodeType::EVENT: d.type = MapStore::MapNodeData::NodeType::EVENT; break;
                            }
                            ms.layerNodes()[i].push_back(d);
                        }
                    }
                    ms.numLayers() = numLayers_;
                    ms.startNodeIdx() = startNodeIdx_;
                    ms.bossNodeIdx() = bossNodeIdx_;
                    ms.playerCurrentNode() = playerCurrentNode_;
                    ms.accessibleNodes() = accessibleNodes_;
                    ms.generated() = true;
                    
                    // 初始化玩家位置（新层的起点）
                    initializePlayer();
                    updateScrollBounds();
                    
                    // 动态水平居中
                    if (layerNodes_.size() > 1 && !layerNodes_[1].empty()) {
                        int minX = layerNodes_[1][0].x;
                        int maxX = layerNodes_[1][0].x;
                        for (const auto& n : layerNodes_[1]) {
                            if (n.x < minX) minX = n.x;
                            if (n.x > maxX) maxX = n.x;
                        }
                        int contentWidth = (maxX - minX) + 400;
                        if (contentWidth < 1) contentWidth = 1;
                        int desiredOffsetX = (screenW_ - contentWidth) / 2 - minX + 200;
                        if (desiredOffsetX < 0) desiredOffsetX = 0;
                        mapOffsetX_ = desiredOffsetX;
                    }
                    
                    std::cout << "[PLAYER MOVE] 已进入下一层，玩家位置: " << playerCurrentNode_ << std::endl;
                } else {
                    std::cout << "[PLAYER MOVE] 已经是最后一层，无法继续前进" << std::endl;
                }
            }
        }
    }
}

// ==================== 事件图标系统实现 ====================

void MapExploreState::loadEventIcons(SDL_Renderer* renderer) {
    // 初始化节点类型到图标的映射
    nodeTypeToIcon_[MapNode::NodeType::START] = "animated_campfire";
    nodeTypeToIcon_[MapNode::NodeType::BOSS] = "animated_bossnode_leshy";
    nodeTypeToIcon_[MapNode::NodeType::ELITE] = "animated_cardbattlenode";
    nodeTypeToIcon_[MapNode::NodeType::SHOP] = "animated_buypelts";
    nodeTypeToIcon_[MapNode::NodeType::EVENT] = "animated_cardchoicenode";
    
    // 加载静态图标
    std::vector<std::string> staticIcons = {
        "moon_portrait"
    };
    
    for (const auto& iconName : staticIcons) {
        std::string path = "assets/events/Texture2D/" + iconName + ".png";
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                eventIcons_[iconName] = {texture, 1, 0, 0.0f, 0.5f};
                std::cout << "[ICON LOAD] 加载静态图标: " << iconName << std::endl;
            }
            SDL_FreeSurface(surface);
        }
    }
    
    // 加载动画图标
    std::vector<std::string> animatedIcons = {
        // Boss图标
        "animated_bossnode_leshy",
        "animated_bossnode_prospector", 
        "animated_bossnode_angler",
        "animated_bossnode_trappertrader",
        
        // 事件图标（直接使用您提供的名称）
        "animated_boulderchoice",
        "animated_buildtotemnode",
        "animated_buypelts",
        "animated_backpack",
        "animated_campfire",
        "animated_cardbattlenode",
        "animated_cardchoicenode",
        "animated_cardchoicenode_cost",
        "animated_cardchoicenode_tribe",
        "animated_cardmergenode",
        "animated_copycard",
        "animated_decktrialnode",
        "animated_mushrooms",
        "animated_removecardnode",
        "animated_totembattlenode",
        "animated_tradepelts"
    };
    
    for (const auto& iconName : animatedIcons) {
        // 直接加载对应的图标文件
        std::string path = "assets/events/Texture2D/" + iconName + "_1.png";
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                eventIcons_[iconName] = {texture, 4, 0, 0.0f, 0.5f};
                std::cout << "[ICON LOAD] 成功加载图标: " << iconName << " 路径: " << path << std::endl;
            } else {
                std::cout << "[ICON LOAD] 纹理创建失败: " << iconName << " 路径: " << path << std::endl;
            }
            SDL_FreeSurface(surface);
        } else {
            std::cout << "[ICON LOAD] 文件加载失败: " << iconName << " 路径: " << path << " 错误: " << IMG_GetError() << std::endl;
        }
    }
}

void MapExploreState::updateEventIcons(float dt) {
    // 静态图标不需要动画更新
    // 如果需要动画，可以在这里添加动画逻辑
}

void MapExploreState::renderEventIcon(SDL_Renderer* renderer, const MapNode& node, int x, int y) {
    std::string iconName = getIconNameForNode(node);
    if (iconName.empty()) {
        std::cout << "[ICON RENDER] 节点 " << node.label << " 没有图标" << std::endl;
        return;
    }
    
    auto it = eventIcons_.find(iconName);
    if (it == eventIcons_.end()) {
        std::cout << "[ICON RENDER] 图标 " << iconName << " 未找到，节点: " << node.label << std::endl;
        return;
    }
    
    std::cout << "[ICON RENDER] 渲染图标: " << iconName << " 节点: " << node.label << std::endl;
    
    const auto& icon = it->second;
    if (!icon.texture) return;
    
    // 获取纹理尺寸
    int texW, texH;
    SDL_QueryTexture(icon.texture, nullptr, nullptr, &texW, &texH);
    
    // 计算渲染尺寸（比节点稍大）
    int renderSize = static_cast<int>(node.size * 1.2f);
    int offsetX = (node.size - renderSize) / 2;
    int offsetY = (node.size - renderSize) / 2;
    
    SDL_Rect destRect = {
        x - renderSize/2 + offsetX,
        y - renderSize/2 + offsetY,
        renderSize,
        renderSize
    };
    
    SDL_RenderCopy(renderer, icon.texture, nullptr, &destRect);
}

std::string MapExploreState::getIconNameForNode(const MapNode& node) {
    std::cout << "[ICON SELECT] 节点标签: '" << node.label << "' 类型: " << (int)node.type << std::endl;
    
    // 根据节点标签选择图标，不区分节点类型
    if (node.label == u8"矿工Boss") {
        std::cout << "[ICON SELECT] 匹配到矿工Boss -> animated_bossnode_prospector" << std::endl;
        return "animated_bossnode_prospector";
    }
    if (node.label == u8"渔夫Boss") {
        std::cout << "[ICON SELECT] 匹配到渔夫Boss -> animated_bossnode_angler" << std::endl;
        return "animated_bossnode_angler";
    }
    if (node.label == u8"猎人Boss") {
        std::cout << "[ICON SELECT] 匹配到猎人Boss -> animated_bossnode_trappertrader" << std::endl;
        return "animated_bossnode_trappertrader";
    }
    if (node.label == u8"最终Boss") {
        std::cout << "[ICON SELECT] 匹配到最终Boss -> animated_bossnode_leshy" << std::endl;
        return "animated_bossnode_leshy";
    }
    
    // 所有事件标签
    if (node.label == u8"寻物人") {
        std::cout << "[ICON SELECT] 匹配到寻物人 -> animated_boulderchoice" << std::endl;
        return "animated_boulderchoice";
    }
    if (node.label == u8"意境刻画") {
        std::cout << "[ICON SELECT] 匹配到意境刻画 -> animated_buildtotemnode" << std::endl;
        return "animated_buildtotemnode";
    }
    if (node.label == u8"墨坊") {
        std::cout << "[ICON SELECT] 匹配到墨坊 -> animated_buypelts" << std::endl;
        return "animated_buypelts";
    }
    if (node.label == u8"墨宝拾遗") {
        std::cout << "[ICON SELECT] 匹配到墨宝拾遗 -> animated_backpack" << std::endl;
        return "animated_backpack";
    }
    if (node.label == u8"淬炼") {
        std::cout << "[ICON SELECT] 匹配到淬炼 -> animated_campfire" << std::endl;
        return "animated_campfire";
    }
    if (node.label == u8"诗剑之争") {
        std::cout << "[ICON SELECT] 匹配到诗剑之争 -> animated_cardbattlenode" << std::endl;
        return "animated_cardbattlenode";
    }
    if (node.label == u8"记忆修复(随机)") {
        std::cout << "[ICON SELECT] 匹配到记忆修复(随机) -> animated_cardchoicenode" << std::endl;
        return "animated_cardchoicenode";
    }
    if (node.label == u8"记忆修复(已知消耗)") {
        std::cout << "[ICON SELECT] 匹配到记忆修复(已知消耗) -> animated_cardchoicenode_cost" << std::endl;
        return "animated_cardchoicenode_cost";
    }
    if (node.label == u8"记忆修复(已知部族)") {
        std::cout << "[ICON SELECT] 匹配到记忆修复(已知部族) -> animated_cardchoicenode_tribe" << std::endl;
        return "animated_cardchoicenode_tribe";
    }
    if (node.label == u8"文脉传承") {
        std::cout << "[ICON SELECT] 匹配到文脉传承 -> animated_cardmergenode" << std::endl;
        return "animated_cardmergenode";
    }
    if (node.label == u8"墨鬼") {
        std::cout << "[ICON SELECT] 匹配到墨鬼 -> animated_copycard" << std::endl;
        return "animated_copycard";
    }
    if (node.label == u8"文心试炼") {
        std::cout << "[ICON SELECT] 匹配到文心试炼 -> animated_decktrialnode" << std::endl;
        return "animated_decktrialnode";
    }
    if (node.label == u8"合卷") {
        std::cout << "[ICON SELECT] 匹配到合卷 -> animated_mushrooms" << std::endl;
        return "animated_mushrooms";
    }
    if (node.label == u8"焚书") {
        std::cout << "[ICON SELECT] 匹配到焚书 -> animated_removecardnode" << std::endl;
        return "animated_removecardnode";
    }
    if (node.label == u8"意境之斗") {
        std::cout << "[ICON SELECT] 匹配到意境之斗 -> animated_totembattlenode" << std::endl;
        return "animated_totembattlenode";
    }
    if (node.label == u8"以物易物") {
        std::cout << "[ICON SELECT] 匹配到以物易物 -> animated_tradepelts" << std::endl;
        return "animated_tradepelts";
    }
    
    // 起点节点不显示图标
    if (node.type == MapNode::NodeType::START) {
        std::cout << "[ICON SELECT] 起点节点，不显示图标" << std::endl;
        return "";
    }
    
    // 默认图标
    std::cout << "[ICON SELECT] 未匹配到任何标签，使用默认图标: animated_cardchoicenode" << std::endl;
    return "animated_cardchoicenode";
}

void MapExploreState::drawDashedLine(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, int dashLength, int gapLength) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float distance = sqrtf(dx * dx + dy * dy);
    
    if (distance == 0) return;
    
    float unitX = dx / distance;
    float unitY = dy / distance;
    
    float currentDistance = 0;
    bool drawing = true;
    
    while (currentDistance < distance) {
        float startX = x1 + currentDistance * unitX;
        float startY = y1 + currentDistance * unitY;
        
        float segmentLength = drawing ? dashLength : gapLength;
        float endDistance = currentDistance + segmentLength;
        
        if (endDistance > distance) {
            endDistance = distance;
        }
        
        if (drawing) {
            float endX = x1 + endDistance * unitX;
            float endY = y1 + endDistance * unitY;
            
            // 绘制粗线条（5像素宽）
            for (int i = 0; i < 5; ++i) {
                SDL_RenderDrawLine(renderer, 
                    static_cast<int>(startX), static_cast<int>(startY + i), 
                    static_cast<int>(endX), static_cast<int>(endY + i));
            }
        }
        
        currentDistance = endDistance;
        drawing = !drawing;
    }
}

void MapExploreState::drawTriangleMarker(SDL_Renderer* renderer, int x, int y, int size) {
    // 设置倒三角颜色（深灰色，在灰白色背景下清晰可见）
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    
    // 绘制倒三角（顶点向下，底边向上）
    int halfSize = size / 2;
    int topY = y - halfSize;    // 底边在上方（最宽）
    int bottomY = y + halfSize;  // 顶点在下方（最窄）
    
    // 倒三角扫描线算法（顶点向下）
    for (int scanY = topY; scanY <= bottomY; scanY++) {
        // 计算当前行的宽度（从底边最宽逐渐变窄到顶点）
        int currentWidth = (bottomY - scanY) * 2;
        int leftX = x - currentWidth / 2;
        int rightX = x + currentWidth / 2;
        SDL_RenderDrawLine(renderer, leftX, scanY, rightX, scanY);
    }
}

void MapExploreState::drawTriangleMarkerWithAlpha(SDL_Renderer* renderer, int x, int y, int size, Uint8 alpha) {
    // 设置倒三角颜色（深灰色，支持透明度）
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, alpha);
    
    // 绘制倒三角（顶点向下，底边向上）
    int halfSize = size / 2;
    int topY = y - halfSize;    // 底边在上方（最宽）
    int bottomY = y + halfSize;  // 顶点在下方（最窄）
    
    // 倒三角扫描线算法（顶点向下）
    for (int scanY = topY; scanY <= bottomY; scanY++) {
        // 计算当前行的宽度（从底边最宽逐渐变窄到顶点）
        int currentWidth = (bottomY - scanY) * 2;
        int leftX = x - currentWidth / 2;
        int rightX = x + currentWidth / 2;
        SDL_RenderDrawLine(renderer, leftX, scanY, rightX, scanY);
    }
}