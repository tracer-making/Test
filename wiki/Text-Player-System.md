# 文本播放系统

文本播放系统是"溯洄遗梦"游戏中用于播放叙事文本的功能模块，通过[TextPlayerState](../Tracer/src/states/TextPlayerState.h)类实现。该系统支持预设文本段落播放、背景图片切换、多种播放模式等功能，为游戏提供丰富的叙事体验。

## 系统概述

文本播放系统通过状态机架构实现，作为一个独立的游戏状态运行。它可以从指定的文本文件中加载预设的文本段落，并支持在播放过程中切换背景图片，提供手动、自动和跳过等多种播放模式。

## 核心特性

1. **预设文本播放** - 支持按预设名称播放特定文本段落
2. **背景图片切换** - 支持在文本播放过程中切换背景图片
3. **多种播放模式** - 支持手动、自动和跳过播放模式
4. **状态跳转** - 播放完成后可跳转到指定游戏状态

## TextPlayerState 类详解

[TextPlayerState](../Tracer/src/states/TextPlayerState.h)是文本播放系统的核心类，继承自[State](../Tracer/src/core/State.h)基类。

### 主要方法

#### 公共方法

- `loadTextData(const std::string& filePath)` - 加载文本数据
- `setTextFile(const std::string& filePath)` - 设置文本文件路径
- `setPresetName(const std::string& presetName)` - 设置预设名称
- `setNextStateFactory(std::function<std::unique_ptr<State>()> factory)` - 设置下一个状态工厂函数
- `startPlayback()` - 开始播放
- `stopPlayback()` - 停止播放

#### 状态方法

- `onEnter(App& app)` - 状态进入时初始化界面和加载资源
- `handleEvent(App& app, const SDL_Event& e)` - 处理用户输入事件
- `update(App& app, float dt)` - 更新播放状态和计时器
- `render(App& app)` - 渲染界面元素

### 核心数据结构

#### TextLine 结构体

```cpp
struct TextLine {
    std::string text;              // 文本内容
    std::string backgroundPath;    // 背景图片路径
    bool hasBackground = false;    // 是否有背景图片
};
```

### 私有成员变量

- `textLines_` - 存储加载的文本行
- `currentLineIndex_` - 当前播放的文本行索引
- `isPlaying_` - 播放状态标志
- `autoMode_` - 自动播放模式标志
- `skipMode_` - 跳过模式标志
- `autoTimer_` - 自动播放计时器
- `autoInterval_` - 自动播放间隔（默认2.0秒）
- `autoExitTimer_` - 自动退出计时器
- `nextStateFactory_` - 下一个状态的工厂函数
- `textFile_` - 文本文件路径
- `presetName_` - 预设名称

## 文本文件格式

文本播放器支持特定格式的文本文件，包含预设段落、背景图片和结束标记。

### 基本结构

```
[PRESET:preset_name]
文本行1
文本行2
[BG:assets/background.png]带背景的文本行
文本行3
end
```

### 标签说明

1. **[PRESET:name]** - 定义一个预设段落，name为预设名称
2. **[BG:path]** - 在文本行前添加背景图片路径
3. **end** - 结束标记，播放器遇到此标记会自动结束播放并跳转

### 示例文本文件

```
[PRESET:main_menu_to_deck_select]
欢迎来到溯源遗梦的世界！
这是一个充满诗意的武侠世界。
[BG:assets/background1.png]让我们开始你的冒险之旅吧！
end

[PRESET:deck_select_to_map_explore]
很好！你已经选择了你的初始牌组。
[BG:assets/background2.png]现在，让我们进入这个神秘的世界。
end
```

## 使用方法

### 通过NarrativeManager使用（推荐）

项目中推荐通过[NarrativeManager](../Tracer/src/core/NarrativeManager.h)来使用文本播放器：

```cpp
// 在App.cpp中初始化叙事跳转
NarrativeManager::setNarrativeTransition(
    NarrativeManager::NarrativeType::MainMenuToDeckSelect,
    "assets/narrative/all_narratives.txt",
    []() -> std::unique_ptr<State> { return std::make_unique<DeckSelectState>(); }
);

// 执行跳转
NarrativeManager::performNarrativeTransition(app, NarrativeManager::NarrativeType::MainMenuToDeckSelect);
```

### 直接使用TextPlayerState

也可以直接创建和使用TextPlayerState：

```cpp
auto textPlayer = std::make_unique<TextPlayerState>();
textPlayer->setTextFile("assets/narrative/custom_narrative.txt");
textPlayer->setPresetName("custom_preset");
textPlayer->setNextStateFactory([]() -> std::unique_ptr<State> { 
    return std::make_unique<CustomState>(); 
});
app.setState(std::move(textPlayer));
```

## 界面元素

文本播放器界面包含以下元素：

1. **背景图片** - 可以动态切换的背景
2. **文本显示区域** - 位于屏幕下方的文本显示区域
3. **控制按钮** - 位于文本区域右上角的控制按钮
   - 自动按钮 - 切换自动播放模式
   - 跳过按钮 - 切换跳过模式
   - 下一句按钮 - 手动播放下一句
4. **返回按钮** - 位于屏幕左上角的返回按钮

## 操作控制

### 键盘控制

- **空格键** - 下一句（在非自动模式下）
- **A键** - 切换自动播放模式
- **S键** - 切换跳过模式
- **ESC键** - 返回测试界面

### 鼠标控制

- **点击文本区域** - 下一句（在非自动模式下）
- **点击按钮** - 控制播放模式

## 播放模式

### 手动模式（默认）

- 用户需要手动点击或按空格键播放下一句
- 适合仔细阅读文本内容

### 自动模式

- 文本会按设定的时间间隔自动播放
- 默认间隔为2秒
- 可通过点击自动按钮或按A键切换

### 跳过模式

- 文本会快速播放
- 间隔为0.1秒
- 可通过点击跳过按钮或按S键切换

## 自定义配置

### 设置文本文件路径

```cpp
textPlayer->setTextFile("path/to/your/textfile.txt");
```

### 设置预设名称

```cpp
textPlayer->setPresetName("your_preset_name");
```

### 设置下一个状态

```cpp
textPlayer->setNextStateFactory([]() -> std::unique_ptr<State> { 
    return std::make_unique<NextState>(); 
});
```

### 调整自动播放间隔

在TextPlayerState.cpp中修改`autoInterval_`成员变量的值：

```cpp
autoInterval_ = 2.0f;  // 自动播放间隔（秒）
```

## 集成到游戏流程

### 1. 在NarrativeManager中注册跳转

在[App.cpp](../Tracer/src/core/App.cpp)的`initializeNarrativeTransitions`函数中添加新的叙事跳转：

```cpp
NarrativeManager::setNarrativeTransition(
    NarrativeManager::NarrativeType::CustomTransition,
    "assets/narrative/all_narratives.txt",
    []() -> std::unique_ptr<State> { return std::make_unique<CustomState>(); }
);
```

### 2. 在游戏状态中触发跳转

在需要播放文本的地方调用：

```cpp
NarrativeManager::performNarrativeTransition(app, NarrativeManager::NarrativeType::CustomTransition);
```

## 注意事项

1. **文本文件编码** - 确保文本文件使用UTF-8编码以正确显示中文
2. **背景图片路径** - 背景图片路径应相对于项目根目录
3. **预设名称匹配** - 确保预设名称与文本文件中的定义完全匹配
4. **状态跳转** - 如果未设置下一个状态工厂函数，播放完成后会默认跳转到测试界面
5. **资源管理** - 文本播放器会自动管理字体和纹理资源，无需手动释放

## 扩展功能

### 添加新的控制按钮

可以通过修改[TextPlayerState.cpp](../Tracer/src/states/TextPlayerState.cpp)的`onEnter`函数来添加新的控制按钮：

```cpp
// 创建新按钮
Button* newButton = new Button();
SDL_Rect newRect{buttonX, buttonY, buttonWidth, buttonHeight};
newButton->setRect(newRect);
newButton->setText(u8"新功能");
if (smallFont_) newButton->setFont(smallFont_, app.getRenderer());
newButton->setOnClick([this]() { 
    // 实现按钮功能
});
```

### 自定义文本渲染

可以通过修改[TextPlayerState.cpp](../Tracer/src/states/TextPlayerState.cpp)的`render`函数来自定义文本渲染效果：

```cpp
// 在渲染当前文本部分添加自定义效果
if (currentTextTexture_) {
    // 添加自定义渲染代码
    // 例如：添加文字阴影、发光效果等
}
```

## 故障排除

### 文本无法显示

1. 检查文本文件路径是否正确
2. 检查预设名称是否匹配
3. 确认字体文件是否存在

### 背景图片无法加载

1. 检查背景图片路径是否正确
2. 确认图片文件是否存在
3. 检查图片格式是否支持（PNG格式推荐）

### 按钮无响应

1. 检查按钮事件处理是否正确
2. 确认按钮区域设置是否正确
3. 验证按钮回调函数是否正确设置

文本播放系统为"溯洄遗梦"提供了丰富的叙事功能，通过精心设计的界面和灵活的配置选项，能够满足游戏中各种文本播放需求。