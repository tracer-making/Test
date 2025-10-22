# 地图系统

"溯洄遗梦"的地图系统是游戏进程管理的核心，采用分层节点结构，为玩家提供丰富的探索体验和多样的游戏内容。

## 系统概述

地图系统通过[MapStore](../Tracer/src/core/MapStore.h)进行数据存储，通过[MapExploreState](../Tracer/src/states/MapExploreState.h)提供用户界面交互。

## 地图结构

### 分层设计

地图采用分层结构，总共10层：
- 第1层：起始层
- 第2-9层：中间层
- 第10层：Boss层

### 节点类型

每层包含不同类型的节点：

1. **START** - 起点节点
2. **NORMAL** - 普通节点
3. **BOSS** - Boss战节点
4. **ELITE** - 精英战节点
5. **SHOP** - 商店节点
6. **EVENT** - 事件节点

### 节点连接

节点之间通过连接关系形成路径：
- 起始层节点连接到中间层节点
- 中间层节点可连接到同层或下一层节点
- 最终连接到Boss节点

## MapStore 类

[MapStore](../Tracer/src/core/MapStore.h)负责地图数据的持久化存储。

### 主要数据结构

```cpp
struct MapNodeData {
    int layer = 0;                          // 所在层数
    int x = 0;                              // X坐标
    int y = 0;                              // Y坐标
    enum class NodeType { START, NORMAL, BOSS, ELITE, SHOP, EVENT } type = NodeType::NORMAL;
    bool visited = false;                   // 是否已访问
    bool accessible = false;                // 是否可达
    std::vector<int> connections;           // 连接的节点索引
    int size = 44;                          // 渲染大小
    std::string label;                      // 显示用文字标签
    int displayOffsetX = 0;                 // 随机位移X
    int displayOffsetY = 0;                 // 随机位移Y
};
```

### 核心功能

1. **地图数据管理**：
   - `layerNodes()` - 分层节点数据
   - `layerBiomes()` - 每层环境标签
   - `numLayers()` - 总层数
   - `startNodeIdx()` - 起点索引
   - `bossNodeIdx()` - Boss节点索引

2. **状态管理**：
   - `hasMap()` - 是否存在地图
   - `clear()` - 清空地图数据
   - `generated()` - 地图生成状态

3. **玩家状态**：
   - `playerCurrentNode()` - 玩家当前位置
   - `accessibleNodes()` - 可访问节点列表
   - `currentMapLayer()` - 当前地图层

## MapExploreState 状态

[MapExploreState](../Tracer/src/states/MapExploreState.h)是地图探索的用户界面状态。

### 主要特性

1. **地图生成**：
   - `generateLayeredMap()` - 生成分层地图
   - `generateMapForLayer()` - 生成指定层地图
   - 支持不同复杂度的地图生成

2. **节点渲染**：
   - 按照节点类型使用不同视觉表现
   - 支持连接线显示
   - 节点状态可视化（已访问、可访问等）

3. **玩家移动**：
   - `startPlayerMoveAnimation()` - 玩家移动动画
   - `updatePlayerMoveAnimation()` - 更新移动动画
   - `movePlayerToNode()` - 移动玩家到指定节点

4. **滚动机制**：
   - 支持垂直滚动查看地图
   - 自动滚动到玩家位置
   - 滚动边界控制

### 地图生成算法

1. **起始层生成**：
   - 创建单一起点节点
   - 设置为已访问状态

2. **中间层生成**：
   - 每层最多3个节点
   - 支持分支和合并结构
   - 复杂连接模式

3. **Boss层生成**：
   - 创建Boss节点
   - 确保从起始点可达

4. **路径验证**：
   - `validatePaths()` - 验证路径有效性
   - `canReachFromStart()` - 检查从起点可达性
   - `hasPathFromStartToBoss()` - 检查是否存在到Boss的路径

## 节点事件系统

不同类型的节点会触发不同的游戏事件：

### NORMAL 节点
- 普通战斗
- 随机事件

### ELITE 节点
- 精英敌人战斗
- 更好的奖励

### SHOP 节点
- 进入商店状态
- 购买道具或卡牌

### EVENT 节点
- 特殊事件
- 随机奖励或挑战

### BOSS 节点
- Boss战
- 重要奖励
- 层级进度推进

## 环境系统

每层地图具有特定的环境标签：
- 林地
- 湿地
- 雪原
- 其他主题环境

环境可能影响：
- 敌人类型
- 事件内容
- 视觉表现

## 可达性系统

### 访问控制
- 玩家只能移动到可达的节点
- 可达性基于已访问节点的连接关系
- 新节点在访问后标记为已访问

### 路径算法
- 使用图遍历算法计算可达节点
- 确保所有节点最终都能到达Boss节点

## 视觉表现

### 节点渲染
- 不同节点类型使用不同颜色和图标
- 已访问节点和未访问节点有明显区别
- 当前位置高亮显示

### 连接线
- 显示节点间的连接关系
- 使用不同样式表示不同连接类型

### 动画效果
- 玩家移动动画
- 节点状态变化动画
- 滚动平滑动画

## 上帝模式

地图系统支持上帝模式，提供额外功能：
- 可以访问任何节点
- 跳过战斗和事件
- 快速测试不同游戏内容

## 数据持久化

### 存储内容
- 节点状态（访问情况）
- 玩家位置
- 地图结构
- 连接关系

### 存储时机
- 节点访问后更新状态
- 玩家移动后更新位置
- 地图生成后保存结构

## 扩展性设计

### 未来扩展
1. **更多节点类型**：添加新的事件类型
2. **动态地图**：地图结构可能随游戏进程变化
3. **多人模式**：支持多玩家在同一地图上互动
4. **随机生成**：更复杂的随机地图生成算法

### 系统集成
地图系统与以下系统紧密集成：
1. **战斗系统**：节点事件触发战斗
2. **商店系统**：SHOP节点进入商店
3. **道具系统**：EVENT节点可能提供道具
4. **进度系统**：通过地图层数追踪游戏进度

地图系统为"溯洄遗梦"提供了结构化的游戏进程管理，让玩家在探索中体验丰富的内容和挑战。