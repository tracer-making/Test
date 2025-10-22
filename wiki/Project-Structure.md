# 项目结构

## 目录结构

```
Tracer/
├── src/
│   ├── core/           # 核心系统和基础类
│   ├── states/         # 游戏状态实现
│   ├── ui/             # 用户界面组件
│   └── main.cpp        # 程序入口点
├── build.bat           # 构建脚本
├── compile.bat         # 编译脚本
└── Tracer.sln          # Visual Studio 解决方案文件
```

## 核心目录详解

### core/ - 核心系统

包含游戏的基础类和核心系统：

- [App.h](../Tracer/src/core/App.h) - 主应用程序类，管理窗口、渲染器和全局状态
- [State.h](../Tracer/src/core/State.h) - 游戏状态基类
- [Card.h](../Tracer/src/core/Card.h) - 卡牌数据结构
- [Cards.h](../Tracer/src/core/Cards.h) - 全局卡牌数据库
- [Deck.h](../Tracer/src/core/Deck.h) - 牌组管理系统
- [EngraveStore.h](../Tracer/src/core/EngraveStore.h) - 意境刻画存储系统
- [ItemStore.h](../Tracer/src/core/ItemStore.h) - 道具存储系统
- [MapStore.h](../Tracer/src/core/MapStore.h) - 地图存储系统
- [WenMaiStore.h](../Tracer/src/core/WenMaiStore.h) - 文脉存储系统

### states/ - 游戏状态

包含所有游戏状态的实现，每个状态代表游戏中的一个界面或模式：

- [BattleState.h](../Tracer/src/states/BattleState.h) - 战斗状态
- [MapExploreState.h](../Tracer/src/states/MapExploreState.h) - 地图探索状态
- [MainMenuState.h](../Tracer/src/states/MainMenuState.h) - 主菜单状态
- [InkShopState.h](../Tracer/src/states/InkShopState.h) - 墨坊状态
- [InkWorkshopState.h](../Tracer/src/states/InkWorkshopState.h) - 墨工坊状态
- [CombineState.h](../Tracer/src/states/CombineState.h) - 合卷状态
- [DeckState.h](../Tracer/src/states/DeckState.h) - 牌组查看状态
- [MemoryRepairState.h](../Tracer/src/states/MemoryRepairState.h) - 记忆修复状态
- [RelicPickupState.h](../Tracer/src/states/RelicPickupState.h) - 墨宝拾遗状态
- [BarterState.h](../Tracer/src/states/BarterState.h) - 以物易物状态
- [EngraveState.h](../Tracer/src/states/EngraveState.h) - 意境刻画状态
- [HeritageState.h](../Tracer/src/states/HeritageState.h) - 文脉传承状态
- [SeekerState.h](../Tracer/src/states/SeekerState.h) - 寻物人状态
- [TemperState.h](../Tracer/src/states/TemperState.h) - 淬炼状态
- [InkGhostState.h](../Tracer/src/states/InkGhostState.h) - 墨魂状态
- [WenxinTrialState.h](../Tracer/src/states/WenxinTrialState.h) - 问心试炼状态
- [VictoryState.h](../Tracer/src/states/VictoryState.h) - 胜利状态

### ui/ - 用户界面组件

包含用户界面相关的组件：

- [Button.h](../Tracer/src/ui/Button.h) - 按钮组件
- [CardRenderer.h](../Tracer/src/ui/CardRenderer.h) - 卡牌渲染器

## 构建相关文件

- `build.bat` - 项目构建脚本
- `compile.bat` - 项目编译脚本
- `Tracer.sln` - Visual Studio 解决方案文件

## 主要入口点

[main.cpp](../Tracer/src/main.cpp) 是程序的入口点，负责初始化应用程序并设置初始状态为 [MainMenuState](../Tracer/src/states/MainMenuState.h)。