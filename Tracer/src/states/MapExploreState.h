#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>

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
        int size = 30;               // 渲染大小
    };
    
    // 分层地图结构
    std::vector<std::vector<MapNode>> layerNodes_; // 每层的节点列表
    int numLayers_ = 10;             // 总层数
    int startNodeIdx_ = -1;          // 起点索引
    int bossNodeIdx_ = -1;           // BOSS点索引
    
    // 玩家状态
    int playerCurrentNode_ = -1;     // 玩家当前所在节点的全局索引
    std::vector<int> accessibleNodes_; // 玩家可移动到的节点列表
    
    // 地图生成
    int maxNodesPerLayer_ = 3;       // 复杂度：每层最多节点数（1-4）
    void generateLayeredMap();
    void generatePathNodes();
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
    int getTotalNodeCount() const;
    
    // 渲染方法
    void renderTitle(SDL_Renderer* renderer);
    void renderMap(SDL_Renderer* renderer);
    void renderNode(SDL_Renderer* renderer, const MapNode& node, int index);
    void renderConnection(SDL_Renderer* renderer, const MapNode& from, const MapNode& to);
    
    // 玩家移动相关
    void initializePlayer();
    void updateAccessibleNodes();
    bool isNodeAccessible(int nodeIndex);
    void movePlayerToNode(int nodeIndex);
    
    // 坐标转换
    SDL_Point gridToScreen(int gridX, int gridY) const;
    int screenToGrid(int screenX, int screenY) const;
};