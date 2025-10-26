# 文本编辑指南

本指南面向不熟悉代码的文案编辑人员，介绍如何编辑"溯洄遗梦"游戏中的文本内容。游戏的文本内容存储在特定格式的文本文件中，通过文本播放器系统在游戏中播放。

## 文本文件位置

所有游戏文本文件都存储在以下位置：
```
Tracer/assets/narrative/all_narratives.txt
```

## 文本文件基本结构

文本文件采用预设段落的组织方式，每个预设段落包含：
1. 预设标识符（定义段落名称）
2. 多行文本内容
3. 结束标记

### 基本格式示例

```
[PRESET:预设名称]
第一行文本内容
第二行文本内容
第三行文本内容
end
```

## 预设标识符

预设标识符用于标识不同的文本段落，格式为：
```
[PRESET:预设名称]
```

预设名称是开发者预先定义的标识符，对应游戏中的特定场景。常见的预设名称包括：

- `main_menu_to_deck_select` - 从主菜单到牌组选择界面
- `deck_select_to_map_explore` - 从牌组选择到地图探索界面
- `map_explore_to_battle` - 从地图探索到战斗界面
- `battle_to_map_explore` - 从战斗到地图探索界面
- `map_explore_to_node` - 从地图探索到节点事件
- `node_to_map_explore` - 从节点事件到地图探索界面

## 文本内容编辑

### 基本文本行

在预设标识符和结束标记之间，每一行都是要显示的文本内容：
```
[PRESET:example_preset]
欢迎来到溯洄遗梦的世界！
这是一个充满诗意的武侠世界。
在这里，你将体验独特的卡牌战斗系统。
end
```

### 背景切换功能

文本播放器支持在播放过程中动态切换背景图片，为玩家提供更丰富的视觉体验。可以在任意文本行前添加背景图片标签来实现背景切换。

#### 背景图片标签格式
```
[BG:图片路径]文本内容
```

#### 背景切换机制
- 当播放器遇到带有背景标签的文本行时，会自动加载并显示指定的背景图片
- 如果下一行没有背景标签，则继续使用当前背景图片
- 如果下一行有新的背景标签，则切换到新背景图片
- 背景图片会持续显示直到遇到新的背景标签或播放结束
- 如果某行只有背景标签而没有文本内容，则只切换背景不显示文本

#### 示例
```
[PRESET:example_preset]
欢迎来到溯洄遗梦的世界！
[BG:assets/background1.png]这是一个充满诗意的武侠世界。
在这里，你将体验独特的卡牌战斗系统。
[BG:assets/background2.png]准备开始你的冒险之旅吧！
记住，每一个选择都可能影响你的命运。
[BG:assets/background3.png]让我们踏上这段充满诗意的旅程吧！
end
```

在上述示例中：
1. 初始显示默认背景
2. 播放第二行时切换到`assets/background1.png`
3. 第三行继续使用`assets/background1.png`
4. 播放第四行时切换到`assets/background2.png`
5. 第五行继续使用`assets/background2.png`
6. 播放第六行时切换到`assets/background3.png`
7. 第七行继续使用`assets/background3.png`

从实际的文本文件中可以看到，背景切换是游戏叙事的重要组成部分，可以为不同的文本内容搭配相应的视觉背景，增强玩家的沉浸感。

#### 常用背景图片路径
- `assets/background1.png`
- `assets/background2.png`
- `assets/background3.png`

## 编辑步骤

### 1. 打开文本文件

使用文本编辑器（如记事本、Notepad++、VS Code等）打开：
```
Tracer/assets/narrative/all_narratives.txt
```

### 2. 找到要编辑的预设段落

在文件中查找对应的预设标识符，例如：
```
[PRESET:main_menu_to_deck_select]
```

### 3. 修改文本内容

在预设标识符和`end`标记之间修改文本内容，注意：
- 不要删除或修改预设标识符
- 不要删除`end`标记
- 可以添加、删除或修改文本行
- 可以在文本行前添加背景图片标签

### 4. 创建新的预设段落（可选）

如果需要添加全新的文本段落，可以在文件末尾添加新的预设段落：
1. 添加预设标识符：`[PRESET:新的预设名称]`
2. 添加文本内容行
3. 添加背景图片标签（可选）
4. 添加结束标记：`end`

注意：新的预设名称需要在代码中注册才能在游戏中使用，仅添加文本预设不会自动在游戏中生效。

### 5. 保存文件

编辑完成后保存文件，确保编码格式为UTF-8。

## 注意事项

### 文本格式要求

1. **预设标识符**：必须保持完整，不要修改或删除
2. **结束标记**：每个预设段落必须以`end`标记结束
3. **空行**：预设段落之间可以有空行，但段落内部的空行会被忽略
4. **特殊字符**：避免使用特殊字符，如制表符、换行符等

### 背景图片使用

1. **图片路径**：使用相对路径，相对于项目根目录
2. **图片格式**：支持PNG格式图片
3. **图片文件**：确保引用的图片文件存在且可访问

### 文本长度建议

1. **单行长度**：建议每行文本不要过长，以便在游戏界面中正确显示
2. **段落长度**：建议每个预设段落包含3-8行文本，避免过长或过短

## 常见预设名称对照表

| 预设名称 | 游戏场景 |
|---------|---------|
| main_menu_to_deck_select | 主菜单 → 牌组选择 |
| deck_select_to_map_explore | 牌组选择 → 地图探索 |
| map_explore_to_battle | 地图探索 → 战斗 |
| battle_to_map_explore | 战斗 → 地图探索 |
| map_explore_to_node | 地图探索 → 各个节点 |
| node_to_map_explore | 节点 → 地图探索 |
| battle_to_victory | 战斗 → 胜利 |
| battle_to_memory_repair | 战斗 → 记忆修复 |
| map_explore_to_heritage | 地图探索 → 文脉传承 |
| map_explore_to_engrave | 地图探索 → 意境刻画 |
| map_explore_to_ink_workshop | 地图探索 → 墨工坊 |
| map_explore_to_ink_ghost | 地图探索 → 墨魂 |
| map_explore_to_barter | 地图探索 → 以物易物 |
| map_explore_to_memory_repair | 地图探索 → 记忆修复 |
| map_explore_to_relic_pickup | 地图探索 → 墨宝拾遗 |
| map_explore_to_seeker | 地图探索 → 寻物人 |
| map_explore_to_temper | 地图探索 → 淬炼 |
| map_explore_to_burn | 地图探索 → 燃烧 |
| map_explore_to_combine | 地图探索 → 合卷 |
| map_explore_to_ink_shop | 地图探索 → 墨坊 |
| heritage_to_map_explore | 文脉传承 → 地图探索 |
| engrave_to_map_explore | 意境刻画 → 地图探索 |
| ink_workshop_to_map_explore | 墨工坊 → 地图探索 |
| ink_ghost_to_map_explore | 墨魂 → 地图探索 |
| barter_to_map_explore | 以物易物 → 地图探索 |
| memory_repair_to_map_explore | 记忆修复 → 地图探索 |
| relic_pickup_to_map_explore | 墨宝拾遗 → 地图探索 |
| seeker_to_map_explore | 寻物人 → 地图探索 |
| temper_to_map_explore | 淬炼 → 地图探索 |
| burn_to_map_explore | 燃烧 → 地图探索 |
| combine_to_map_explore | 合卷 → 地图探索 |
| ink_shop_to_map_explore | 墨坊 → 地图探索 |

## 编辑示例

### 修改前
```
[PRESET:main_menu_to_deck_select]
欢迎来到溯源遗梦的世界！
这是一个充满诗意的武侠世界。
在这里，你将体验独特的卡牌战斗系统。
[BG:assets/background1.png]让我们开始你的冒险之旅吧！
end
```

### 修改后
```
[PRESET:main_menu_to_deck_select]
欢迎来到溯洄遗梦的奇幻世界！
[BG:assets/background1.png]在这里，你将踏上一段充满挑战与惊喜的旅程。
准备好选择你的初始牌组了吗？
[BG:assets/background2.png]每个牌组都有独特的战斗风格和策略。
[BG:assets/background3.png]让我们开始你的冒险之旅吧！
end
```

在修改后的示例中：
- 第一行使用默认背景
- 第二行开始切换到`assets/background1.png`
- 第四行切换到`assets/background2.png`
- 第五行切换到`assets/background3.png`

## 测试修改效果

修改文本文件后，需要重新运行游戏才能看到效果：
1. 保存文本文件
2. 重新编译并运行游戏
3. 进入对应的场景查看文本显示效果

如果文本没有正确显示，请检查：
1. 预设名称是否正确
2. 是否遗漏了`end`标记
3. 文本文件编码是否为UTF-8
4. 背景图片路径是否正确

通过遵循本指南，文案编辑人员可以轻松地修改游戏中的文本内容，为玩家提供更好的叙事体验。