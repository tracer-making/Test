#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include "../core/MapStore.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <random>
#include <string>

class MapExploreState : public State {
public:
    MapExploreState();
    ~MapExploreState();

    void onEnter(App& app) override;
    void onExit(App& app) override;
    void handleEvent(App& app, const SDL_Event& e) override;
    void update(App& app, float dt) override;
    void render(App& app) override;

private:
    // 屏幕尺寸
    int screenW_ = 1280;
    int screenH_ = 720;
    
    // UI按钮
    Button* regenerateButton_ = nullptr;
    std::vector<Button*> difficultyButtons_;
    Button* backToTestButton_ = nullptr;
    bool pendingGoTest_ = false;
    
    // 状态切换
    bool pendingGoBattle_ = false;
    bool pendingGoBarter_ = false;
    bool pendingGoEngrave_ = false;
    bool pendingGoHeritage_ = false;
    bool pendingGoInkShop_ = false;
    bool pendingGoMemoryRepair_ = false;
    bool pendingGoRelicPickup_ = false;
    bool pendingGoSeeker_ = false;
    bool pendingGoTemper_ = false;
    bool pendingGoInkGhost_ = false;
    bool pendingGoInkWorkshop_ = false;
    
    // 字体
    _TTF_Font* font_ = nullptr;
    _TTF_Font* smallFont_ = nullptr;
    
    // 标题
    SDL_Texture* titleTex_ = nullptr;
    int titleW_ = 0;
    int titleH_ = 0;
    
    // 地图节点
    struct MapNode {
        int layer;                   // 所在层数
        int x, y;                    // 屏幕坐标
        enum class NodeType { START, NORMAL, BOSS, ELITE, SHOP, EVENT } type; // 节点类型
        bool visited = false;        // 是否已访问
        bool accessible = false;     // 是否可达
        std::vector<int> connections; // 连接的节点索引
        int size = 44;               // 渲染大小（放大）
        std::string label;           // 显示用文字标签（按行随机分配）
    };
    
    // 分层地图结构
    std::vector<std::vector<MapNode>> layerNodes_; // 每层的节点列表
    int numLayers_ = 10;             // 总层数
    int startNodeIdx_ = -1;          // 起点索引
    int bossNodeIdx_ = -1;           // BOSS点索引
    
    // 玩家状态
    int playerCurrentNode_ = -1;     // 玩家当前所在节点的全局索引
    std::vector<int> accessibleNodes_; // 玩家可移动到的节点列表
    
    // 地图选择
    int currentMapLayer_ = 1;        // 当前选择的地图层（1-4）
    static bool s_mapGenerated_;     // 静态标志，跟踪地图是否已经生成
    
    // 地图生成
    int maxNodesPerLayer_ = 3;       // 复杂度：每层最多节点数（1-4）
    void generateLayeredMap(bool shouldBuildOffsets = true);
    void generateMapForLayer(int layer);
    void generateFirstLayerPattern(std::mt19937& gen);
    void generateSecondThirdLayerPattern(std::mt19937& gen);
    void generateLastLayerPattern(std::mt19937& gen);
    void connectPaths();
    void createEndNode();
    void validatePaths();
    bool canReachFromStart(int targetNodeIndex);
    bool canReachFromEnd(int targetNodeIndex);
    int countPathsFromStartToEnd();
    int dfsCountPaths(int current, int target, std::vector<bool>& visited, int depth);
    void connectNodes(int fromLayer, int fromIndex, int toLayer, int toIndex);
    void createBranching(int fromLayer, int fromIndex, int toLayer);
    void createMerging(int fromLayer, int toLayer, int toIndex);
    void createComplexConnections(int fromLayer, int toLayer);
    int selectBestParentNode(int layer, int nextNodeIndex, int parentNodeCount);
    void connectStartAndBoss();
    void validateAndTrimPaths();
    bool hasPathFromStartToBoss();
    bool allPathsFromStartReachBoss();
    bool dfsCheckAllPaths(int current, int target, std::vector<bool>& visited);
    void removeOrphanNodes();
    
    // 辅助方法
    int getGlobalNodeIndex(int layer, int localIndex) const;
    MapNode* getNodeByGlobalIndex(int globalIndex);
    const MapNode* getNodeByGlobalIndex(int globalIndex) const;
    int getTotalNodeCount() const;
    
    // 渲染方法
    void renderTitle(SDL_Renderer* renderer);
    void renderMap(SDL_Renderer* renderer);
    void renderNode(SDL_Renderer* renderer, const MapNode& node, int index);
    void renderConnection(SDL_Renderer* renderer, int fromGlobalIndex, int toGlobalIndex);
    void renderGodModeIndicator(SDL_Renderer* renderer);
    
    // 玩家移动相关
    void initializePlayer();
    void updateAccessibleNodes();
    bool isNodeAccessible(int nodeIndex);
    void movePlayerToNode(int nodeIndex);
    void startMoveAnimation(int targetNodeIndex);
    
    // 坐标转换
    SDL_Point gridToScreen(int gridX, int gridY) const;
    int screenToGrid(int screenX, int screenY) const;

    // 视图偏移与滚动
    int mapOffsetX_ = 80;
    int mapOffsetY_ = 960;
    int scrollY_ = 0;               // 垂直滚动（像素）
    int maxScrollY_ = 2500;            // 向下滚动的最大值（像素）
    int maxScrollYCap_ = 2500;        // 向下滚动的上限（可配置，单位像素）
    int minWorldY_ = 0;             // 地图最小Y（世界坐标）
    int maxWorldY_ = 0;             // 地图最大Y（世界坐标）
    int visibleRowCount_ = 5;       // 屏幕最多显示的行数（用于滚动步长）
    int scrollStep_ = 80;           // 滚动步长（像素）
    
    // 上帝模式
    bool godMode_ = false;          // 上帝模式开关
    
    // 地图滚动控制
    static bool s_firstMapEnter_;   // 是否第一次进入地图
    

    // 工具：将节点世界坐标转换为屏幕坐标（应用偏移与滚动）
    inline void nodeToScreenXY(const MapNode& node, int& sx, int& sy) const {
        sx = mapOffsetX_ + node.x;
        sy = mapOffsetY_ + (node.y + scrollY_);
    }
    void getScreenXYForGlobalIndex(int globalIndex, int& sx, int& sy) const;
    void updateScrollBounds();
    void assignRowEventLabels();
    void updateHorizontalCenter();

    // 仅影响显示的偏移（不改变逻辑坐标）
    std::vector<SDL_Point> nodeDisplayOffset_; // 按全局索引存储额外偏移
    void buildDisplayOffsets();

    // 移动动画
    bool isMoving_ = false;
    int moveFromNode_ = -1;
    int moveToNode_ = -1;
    float moveT_ = 0.0f;
    float moveDuration_ = 0.5f; // 秒
    SDL_FPoint moveStartPos_{0.f, 0.f};
    SDL_FPoint moveEndPos_{0.f, 0.f};
};