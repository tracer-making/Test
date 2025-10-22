# 道具系统

"溯洄遗梦"包含丰富的道具系统，为玩家提供额外的能力和策略选择。

## Item 数据结构

道具通过简单的结构体定义：

```cpp
struct Item {
    std::string id;           // 道具ID
    std::string name;         // 道具名称
    std::string description;  // 道具描述
    int count;               // 道具数量
};
```

## ItemStore 全局道具存储

[ItemStore](../Tracer/src/core/ItemStore.h)是全局道具存储系统，负责管理玩家拥有的所有道具：

### 主要方法

- `addItem()` - 添加道具
- `removeItem()` - 移除道具
- `hasItem()` - 检查是否拥有指定道具
- `getItemCount()` - 获取道具数量
- `clear()` - 清空所有道具

## 道具获取和管理状态

### RelicPickupState - 墨宝拾遗

[RelicPickupState](../Tracer/src/states/RelicPickupState.h)是道具收集界面，当玩家拥有的道具不足三个时，会随机补齐至三个。

主要特性：
- 随机生成候选道具
- 玩家可以选择拾取一个道具
- 拾取动画效果

### BattleState - 战斗中的道具使用

在[BattleState](../Tracer/src/states/BattleState.h)中，玩家可以在战斗中使用道具：

- 最大道具数量限制为3个
- 道具槽位显示在战斗界面
- 道具使用效果影响战斗进程

## 道具使用机制

### InkWorkshopState - 墨工坊

[InkWorkshopState](../Tracer/src/states/InkWorkshopState.h)允许玩家消耗文脉获取毛皮道具：

- 兔皮（消耗2文脉）
- 狼皮（消耗4文脉）
- 金羊皮（消耗7文脉）
- 道具（消耗7文脉）

### 道具效果

道具可以在不同状态下提供不同的效果：

1. **战斗增益** - 在BattleState中提供额外能力
2. **地图探索增益** - 在MapExploreState中提供便利
3. **商店折扣** - 在InkShopState中降低购买价格
4. **特殊能力** - 解锁新的游戏机制或状态

## 道具类型

### 消耗品道具

一次性使用的道具，使用后消失：

- 临时攻击力提升
- 临时生命值恢复
- 额外抽牌机会

### 持续效果道具

提供持续效果的道具：

- 每场战斗开始时获得额外魂骨
- 增加墨尺偏向玩家的初始位置
- 降低卡牌献祭消耗

### 被动增益道具

提供被动能力的道具：

- 增加手牌上限
- 提高抽牌数量
- 增加印记效果强度

## 道具管理和存储

### 最大持有数量

玩家最多可以持有3个道具，这一限制在多个状态中都有体现：

- [BattleState](../Tracer/src/states/BattleState.h) - 战斗中的道具槽位
- [RelicPickupState](../Tracer/src/states/RelicPickupState.h) - 道具拾取界面
- [InkWorkshopState](../Tracer/src/states/InkWorkshopState.h) - 道具获取界面

### 道具显示

道具在不同界面中以统一的方式显示：

- 道具图标或名称
- 道具描述
- 持有数量（如果适用）

## 道具平衡设计

道具系统设计考虑了以下平衡因素：

1. **稀有度平衡** - 不同稀有度的道具提供不同强度的效果
2. **获取难度** - 稀有道具需要更高的代价或更难的条件获取
3. **使用策略** - 道具效果鼓励不同的游戏策略
4. **组合效果** - 多个道具之间可能存在协同效应

## 道具与游戏进程

道具在游戏的不同阶段发挥重要作用：

### 早期游戏

- 帮助玩家建立基础卡组
- 提供生存能力
- 加速资源获取

### 中期游戏

- 增强战斗能力
- 解锁新的战术选择
- 提供策略多样性

### 后期游戏

- 对抗强大敌人和Boss
- 提供通关必需的能力
- 影响游戏结局

道具系统为"溯洄遗梦"增加了深度和重玩价值，玩家可以通过不同的道具组合尝试不同的游戏策略。