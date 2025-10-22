# 游戏状态

"溯洄遗梦"使用状态机架构管理游戏流程，每个状态代表游戏中的一个界面或模式。

## 状态基类

所有状态都继承自[State](../Tracer/src/core/State.h)基类，实现统一的接口：

- `onEnter()` - 状态进入时调用
- `onExit()` - 状态退出时调用
- `handleEvent()` - 处理输入事件
- `update()` - 更新逻辑
- `render()` - 渲染界面

## 主要游戏状态

### MainMenuState - 主菜单状态

[MainMenuState](../Tracer/src/states/MainMenuState.h)是游戏的起始界面，提供开始游戏等选项。

### MapExploreState - 地图探索状态

[MapExploreState](../Tracer/src/states/MapExploreState.h)是游戏的主要导航界面，玩家在此选择前往不同地点。

主要特性：
- 分层地图结构
- 节点类型：起点、普通、Boss、精英、商店、事件
- 玩家移动和路径选择

### BattleState - 战斗状态

[BattleState](../Tracer/src/states/BattleState.h)是游戏的核心战斗系统，包含复杂的战斗机制。

主要特性：
- 3x4战场网格布局
- 回合制战斗系统
- 丰富的印记系统
- 多种攻击类型（普通、双向、三向、双重等）
- Boss战系统（多阶段Boss）
- 道具系统
- 墨尺平衡机制
- 献祭系统

### DeckState - 牌组查看状态

[DeckState](../Tracer/src/states/DeckState.h)用于查看玩家当前的完整牌组。

### CombineState - 合卷状态

[CombineState](../Tracer/src/states/CombineState.h)允许玩家将两张相同的卡牌合成为一张更强的卡牌。

### InkShopState - 墨坊状态

[InkShopState](../Tracer/src/states/InkShopState.h)是商店系统，玩家可以使用文脉购买墨。

### InkWorkshopState - 墨工坊状态

[InkWorkshopState](../Tracer/src/states/InkWorkshopState.h)允许玩家使用毛皮换取卡牌或道具。

### MemoryRepairState - 记忆修复状态

[MemoryRepairState](../Tracer/src/states/MemoryRepairState.h)提供卡牌选择机制，玩家可以从中选择一张卡牌加入牌组。

### RelicPickupState - 墨宝拾遗状态

[RelicPickupState](../Tracer/src/states/RelicPickupState.h)是道具收集界面，玩家可以拾取新的道具。

### BarterState - 以物易物状态

[BarterState](../Tracer/src/states/BarterState.h)允许玩家使用毛皮交换其他卡牌。

### EngraveState - 意境刻画状态

[EngraveState](../Tracer/src/states/EngraveState.h)管理意境系统，玩家可以选择意（印记）和境（部类）进行组合。

### HeritageState - 文脉传承状态

[HeritageState](../Tracer/src/states/HeritageState.h)允许玩家将一张卡牌的印记传承给另一张卡牌。

### SeekerState - 寻物人状态

[SeekerState](../Tracer/src/states/SeekerState.h)是一个卡牌发现机制，玩家从几张卡牌中选择一张。

### TemperState - 淬炼状态

[TemperState](../Tracer/src/states/TemperState.h)允许玩家强化卡牌，增加攻击力或生命值。

### InkGhostState - 墨魂状态

[InkGhostState](../Tracer/src/states/InkGhostState.h)是一个卡牌生成机制。

### WenxinTrialState - 问心试炼状态

[WenxinTrialState](../Tracer/src/states/WenxinTrialState.h)提供多种试炼挑战，成功后可获得奖励卡牌。

### VictoryState - 胜利状态

[VictoryState](../Tracer/src/states/VictoryState.h)在玩家击败最终Boss后显示。

## 状态切换机制

游戏状态通过在当前状态中设置标志位并在主循环中检查这些标志位来实现切换。例如，在[BattleState](../Tracer/src/states/BattleState.h)中：

```cpp
bool pendingGoMapExplore_ = false;  // 返回地图探索
```

当这个标志位被设置为true时，主循环会切换到[MapExploreState](../Tracer/src/states/MapExploreState.h)状态。