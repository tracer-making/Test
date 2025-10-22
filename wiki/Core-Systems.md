# 核心系统

## App 类

[App](../Tracer/src/core/App.h) 是应用程序的主类，负责管理整个游戏的生命周期。

### 主要职责

1. **窗口和渲染管理** - 创建和管理SDL窗口和渲染器
2. **状态管理** - 管理当前游戏状态的切换
3. **全局状态存储** - 存储游戏的全局状态，如上帝模式、淬炼祝福、蜡烛数量等

### 关键方法

- `init()` - 初始化应用程序
- `run()` - 运行主循环
- `setState()` - 切换游戏状态
- `shutdown()` - 关闭应用程序

### 全局状态

App类维护多个全局状态变量：

- `godMode_` - 上帝模式开关
- `temperBlessing_` - 淬炼系统全局状态
- `selectedInitialDeck_` - 初始牌组选择
- `remainingCandles_` - 蜡烛系统（游戏生命值）

## State 基类

[State](../Tracer/src/core/State.h) 是所有游戏状态的基类，定义了状态的基本接口。

### 虚函数接口

- `onEnter()` - 状态进入时调用
- `onExit()` - 状态退出时调用
- `handleEvent()` - 处理SDL事件
- `update()` - 更新游戏逻辑
- `render()` - 渲染状态内容

## Card 类

[Card](../Tracer/src/core/Card.h) 是卡牌数据结构，包含卡牌的所有属性信息。

### 主要属性

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

## CardDB 类

[CardDB](../Tracer/src/core/Cards.h) 是全局卡牌数据库，负责注册和管理所有卡牌原型。

### 主要功能

- `registerCard()` - 注册卡牌原型
- `make()` - 生成卡牌实例
- `find()` - 查找卡牌原型
- `loadBuiltinCards()` - 加载内置卡牌

## DeckStore 类

[DeckStore](../Tracer/src/core/Deck.h) 是牌组管理系统，管理玩家的牌库、手牌和弃牌堆。

### 主要功能

- 管理手牌、牌库、弃牌堆和墨牌堆
- 提供抽牌、弃牌等操作
- 管理文脉传承功能
- 跟踪骨数和墨数

## 存储系统

项目包含多个专门的存储系统：

### EngraveStore

[EngraveStore](../Tracer/src/core/EngraveStore.h) 管理意境刻画系统，存储意（印记）和境（部类）的绑定关系。

### ItemStore

[ItemStore](../Tracer/src/core/ItemStore.h) 管理全局道具存储。

### MapStore

[MapStore](../Tracer/src/core/MapStore.h) 管理地图数据的持久化存储。

### WenMaiStore

[WenMaiStore](../Tracer/src/core/WenMaiStore.h) 管理文脉值，记录战斗中对敌人本体造成的溢出伤害总量。