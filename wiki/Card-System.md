# 卡牌系统

"溯洄遗梦"的卡牌系统是游戏的核心机制之一，包含丰富的卡牌类型、属性和印记系统。

## Card 数据结构

[Card](../Tracer/src/core/Card.h)类定义了卡牌的基本属性：

- `id` - 全局唯一ID
- `name` - 卡牌名称
- `sacrificeCost` - 献祭消耗
- `category` - 卡牌种类（如：兵器/诗篇/墨/印/卷等）
- `attack` - 攻击力
- `health` - 生命值
- `face` - 牌面（资源ID/图片路径/样式名）
- `marks` - 印记列表
- `canInherit` - 是否可传承
- `obtainable` - 获取等级
- `canBeSacrificed` - 是否可被献祭
- `instanceId` - 实例唯一ID

## CardDB 卡牌数据库

[CardDB](../Tracer/src/core/Cards.h)是全局卡牌数据库，负责管理所有卡牌原型：

- `registerCard()` - 注册卡牌原型
- `make()` - 生成卡牌实例
- `find()` - 查找卡牌原型
- `loadBuiltinCards()` - 加载内置卡牌

## 卡牌种类（Category）

卡牌按种类分类，不同种类的卡牌可能有不同的机制和用途：

- 兵器
- 诗篇
- 墨
- 印
- 卷
- 毛皮（兔皮、狼皮、金羊皮）

## 印记系统（Marks）

印记是卡牌的核心机制，为卡牌提供特殊能力。常见的印记包括：

### 攻击类印记

- **横冲直撞** - 可以在战场上移动并攻击路径上的敌人
- **蛮力冲撞** - 可以推动其他卡牌
- **水袭** - 潜水状态下免疫攻击，浮出时造成额外伤害
- **护主** - 当同列前方卡牌死亡时，自动移动到前方位置

### 防御类印记

- **不死** - 死亡后回到手牌
- **反伤** - 受到攻击时对攻击者造成伤害
- **坚盾** - 减少受到的伤害

### 特殊机制印记

- **传承** - 可以将印记传承给其他卡牌
- **献祭** - 可以被献祭以获得资源
- **成长** - 每回合增加攻击力或生命值
- **检索** - 可以从牌库中检索特定卡牌

### 环境互动印记

- **风雅扇** - 获得空袭能力
- **日晷** - 影响墨尺平衡
- **断因剑** - 可以选择攻击目标
- **吞墨毫** - 可以消耗墨量增强攻击

## DeckStore 牌组系统

[DeckStore](../Tracer/src/core/Deck.h)管理玩家的牌组，包括：

- 牌库（library）
- 手牌（hand）
- 弃牌堆（discard）
- 墨牌堆（inkPile）

### 主要操作

- `drawToHand()` - 抽牌到手牌
- `addToLibrary()` - 添加卡牌到牌库
- `discardFromHand()` - 从手牌弃牌
- `inheritMarks()` - 文脉传承（手牌内）

## 战斗中的卡牌机制

在[BattleState](../Tracer/src/states/BattleState.h)中，卡牌在3x4的战场网格上进行战斗：

### 战场布局

```
敌方区域（第1、2行）
[0][1][2][3]
[4][5][6][7]

我方区域（第3行）
[8][9][10][11]
```

### 卡牌状态

- 生命值（health）
- 攻击力（attack）
- 是否存活（isAlive）
- 是否被献祭（isSacrificed）
- 移动方向（moveDirection）
- 放置回合数（placedTurn）

### 特殊攻击类型

- **Normal** - 普通攻击（对位）
- **Double** - 双向攻击（斜对位）
- **Triple** - 三向攻击（对位+斜对位）
- **Twice** - 双重攻击（对位两次）
- **DoubleTwice** - 双向双重攻击（斜对位各攻击两次）
- **TripleTwice** - 三向双重攻击（对位+斜对位各攻击两次）
- **AllDirection** - 全向打击（攻击对面全部卡牌）

## 卡牌强化和合成

### 淬炼系统

[TemperState](../Tracer/src/states/TemperState.h)允许玩家选择一张手牌进行强化：

- +1攻击力
- +2生命值

### 合卷系统

[CombineState](../Tracer/src/states/CombineState.h)允许玩家将两张相同的卡牌合成为一张更强的卡牌。

### 文脉传承

[HeritageState](../Tracer/src/states/HeritageState.h)允许玩家将一张卡牌的印记传承给另一张卡牌。

## 卡牌获取机制

### 记忆修复

[MemoryRepairState](../Tracer/src/states/MemoryRepairState.h)提供卡牌选择机制。

### 寻物人

[SeekerState](../Tracer/src/states/SeekerState.h)让玩家从几张卡牌中选择一张。

### 墨工坊

[InkWorkshopState](../Tracer/src/states/InkWorkshopState.h)允许玩家使用毛皮换取卡牌。

### 以物易物

[BarterState](../Tracer/src/states/BarterState.h)允许玩家使用毛皮交换其他卡牌。

### 问心试炼

[WenxinTrialState](../Tracer/src/states/WenxinTrialState.h)通过完成试炼获得奖励卡牌。