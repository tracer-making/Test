#pragma once

#include "../core/State.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>
#include <memory>

struct TextLine {
    std::string text;
    std::string backgroundPath;  // 背景图片路径，空字符串表示不切换背景
    bool hasBackground = false;
};

class TextPlayerState : public State {
public:
    TextPlayerState();
    ~TextPlayerState();

    void onEnter(App& app) override;
    void handleEvent(App& app, const SDL_Event& e) override;
    void update(App& app, float dt) override;
    void render(App& app) override;

    // 加载文本数据
    void loadTextData(const std::string& filePath);
    
    // 设置文本文件路径
    void setTextFile(const std::string& filePath);
    
    // 设置预设名称
    void setPresetName(const std::string& presetName);
    
    // 设置下一个状态工厂函数
    void setNextStateFactory(std::function<std::unique_ptr<State>()> factory);
    
    // 开始播放
    void startPlayback();
    
    // 停止播放
    void stopPlayback();

private:
    // 文本数据
    std::vector<TextLine> textLines_;
    int currentLineIndex_ = 0;
    bool isPlaying_ = false;
    bool autoMode_ = false;
    bool skipMode_ = false;
    
    // 自动播放计时器
    float autoTimer_ = 0.0f;
    float autoInterval_ = 2.0f;  // 自动播放间隔（秒）
    
    // 自动退出计时器
    float autoExitTimer_ = 0.0f;
    
    // 下一个状态工厂函数
    std::function<std::unique_ptr<State>()> nextStateFactory_;
    
    // 文本文件路径
    std::string textFile_;
    
    // 预设名称
    std::string presetName_;
    
    // UI组件
    Button* backButton_ = nullptr;
    Button* autoButton_ = nullptr;
    Button* skipButton_ = nullptr;
    Button* nextButton_ = nullptr;
    
    // 字体
    TTF_Font* font_ = nullptr;
    TTF_Font* smallFont_ = nullptr;
    
    // 屏幕尺寸
    int screenW_ = 1280;
    int screenH_ = 720;
    
    // 背景图片
    SDL_Texture* backgroundTexture_ = nullptr;
    std::string currentBackgroundPath_;
    
    // 文本显示区域
    SDL_Rect textArea_;
    SDL_Texture* currentTextTexture_ = nullptr;
    int textW_ = 0;
    int textH_ = 0;
    
    // 颜色
    SDL_Color textColor_ = {255, 255, 255, 255};
    SDL_Color backgroundColor_ = {0, 0, 0, 200};
    
    // 内部方法
    void loadBackground(const std::string& path);
    void updateTextDisplay();
    void nextLine();
    void toggleAuto();
    void toggleSkip();
    void cleanup();
};
