#include "TextPlayerState.h"
#include "TestState.h"
#include "../core/App.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <fstream>
#include <sstream>
#include <iostream>

TextPlayerState::TextPlayerState() = default;

TextPlayerState::~TextPlayerState() {
    cleanup();
}

void TextPlayerState::onEnter(App& app) {
    SDL_Log("TextPlayerState::onEnter - Start");
    SDL_Log("当前预设名称: '%s', 文本文件: '%s'", presetName_.c_str(), textFile_.c_str());
    
    // 设置窗口尺寸（与其他界面保持一致）
    screenW_ = 1600;
    screenH_ = 1000;
    SDL_SetWindowSize(app.getWindow(), screenW_, screenH_);
    
    // 加载字体
    font_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 24);
    smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 18);
    if (!font_ || !smallFont_) {
        SDL_Log("TTF_OpenFont failed: %s", TTF_GetError());
        return;
    }
    
    // 设置文本显示区域（屏幕下方）
    textArea_ = {100, screenH_ - 250, screenW_ - 200, 200};
    
    // 创建返回按钮
    backButton_ = new Button();
    SDL_Rect backRect{20, 20, 80, 40};
    backButton_->setRect(backRect);
    backButton_->setText(u8"返回");
    if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer());
    backButton_->setOnClick([&app]() {
        app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState())));
    });
    
    // 创建控制按钮（放在文本框右上角）
    int buttonWidth = 70;
    int buttonHeight = 30;
    int buttonSpacing = 80;
    int buttonY = textArea_.y + 10;  // 文本框顶部
    int buttonX = textArea_.x + textArea_.w - (buttonWidth * 3 + buttonSpacing * 2) - 20;  // 右上角
    
    // Auto按钮
    autoButton_ = new Button();
    SDL_Rect autoRect{buttonX, buttonY, buttonWidth, buttonHeight};
    autoButton_->setRect(autoRect);
    autoButton_->setText(u8"自动");
    if (smallFont_) autoButton_->setFont(smallFont_, app.getRenderer());
    autoButton_->setOnClick([this]() { toggleAuto(); });
    
    // Skip按钮
    skipButton_ = new Button();
    SDL_Rect skipRect{buttonX + buttonSpacing, buttonY, buttonWidth, buttonHeight};
    skipButton_->setRect(skipRect);
    skipButton_->setText(u8"跳过");
    if (smallFont_) skipButton_->setFont(smallFont_, app.getRenderer());
    skipButton_->setOnClick([this]() { toggleSkip(); });
    
    // Next按钮
    nextButton_ = new Button();
    SDL_Rect nextRect{buttonX + buttonSpacing * 2, buttonY, buttonWidth, buttonHeight};
    nextButton_->setRect(nextRect);
    nextButton_->setText(u8"下一句");
    if (smallFont_) nextButton_->setFont(smallFont_, app.getRenderer());
    nextButton_->setOnClick([this]() { nextLine(); });
    
    // 加载文本数据
    if (textFile_.empty()) {
        loadTextData("assets/text_data.txt");
    } else {
        loadTextData(textFile_);
    }
    
    // 开始播放
    startPlayback();
    
    SDL_Log("TextPlayerState::onEnter - Complete");
}

void TextPlayerState::handleEvent(App& app, const SDL_Event& e) {
    // 处理按钮事件
    if (backButton_) backButton_->handleEvent(e);
    if (autoButton_) autoButton_->handleEvent(e);
    if (skipButton_) skipButton_->handleEvent(e);
    if (nextButton_) nextButton_->handleEvent(e);
    
    // 处理点击事件（点击文本区域也可以下一句）
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;
        
        // 检查是否点击在文本区域
        if (mx >= textArea_.x && mx <= textArea_.x + textArea_.w &&
            my >= textArea_.y && my <= textArea_.y + textArea_.h) {
            if (!autoMode_) {
                nextLine();
            }
        }
    }
    
    // 处理键盘事件
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_SPACE:
            if (!autoMode_) nextLine();
            break;
        case SDLK_a:
            toggleAuto();
            break;
        case SDLK_s:
            toggleSkip();
            break;
        case SDLK_ESCAPE:
            app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState())));
            break;
        }
    }
}

void TextPlayerState::update(App& app, float dt) {
    // 处理自动退出
    if (autoExitTimer_ > 0.0f) {
        autoExitTimer_ -= dt;
        if (autoExitTimer_ <= 0.0f) {
            // 使用设置的下一个状态工厂函数，如果没有则默认退出到测试界面
            if (nextStateFactory_) {
                app.setState(nextStateFactory_());
            } else {
                app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState())));
            }
            return;
        }
    }
    
    if (!isPlaying_) return;
    
    // 自动播放逻辑
    if (autoMode_ && !skipMode_) {
        autoTimer_ += dt;
        if (autoTimer_ >= autoInterval_) {
            nextLine();
            autoTimer_ = 0.0f;
        }
    }
    
    // 跳过模式逻辑
    if (skipMode_) {
        autoTimer_ += dt;
        if (autoTimer_ >= 0.1f) {  // 快速跳过
            nextLine();
            autoTimer_ = 0.0f;
        }
    }
}

void TextPlayerState::render(App& app) {
    SDL_Renderer* r = app.getRenderer();
    
    // 处理背景加载
    static std::string lastBackgroundPath;
    if (!currentBackgroundPath_.empty() && currentBackgroundPath_ != lastBackgroundPath) {
        // 清理旧背景
        if (backgroundTexture_) {
            SDL_DestroyTexture(backgroundTexture_);
            backgroundTexture_ = nullptr;
        }
        
        SDL_Surface* surface = IMG_Load(currentBackgroundPath_.c_str());
        if (surface) {
            backgroundTexture_ = SDL_CreateTextureFromSurface(r, surface);
            SDL_FreeSurface(surface);
            lastBackgroundPath = currentBackgroundPath_;
            SDL_Log("加载背景图片: %s", currentBackgroundPath_.c_str());
        } else {
            SDL_Log("无法加载背景图片: %s, 错误: %s", currentBackgroundPath_.c_str(), IMG_GetError());
        }
    }
    
    // 渲染背景
    if (backgroundTexture_) {
        SDL_RenderCopy(r, backgroundTexture_, nullptr, nullptr);
    } else {
        // 默认黑色背景
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_RenderClear(r);
    }
    
    // 渲染文本背景框
    SDL_SetRenderDrawColor(r, backgroundColor_.r, backgroundColor_.g, backgroundColor_.b, backgroundColor_.a);
    SDL_RenderFillRect(r, &textArea_);
    
    // 渲染文本边框
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderDrawRect(r, &textArea_);
    
    // 处理文本渲染
    if (!currentTextTexture_ && currentLineIndex_ < textLines_.size()) {
        const TextLine& currentLine = textLines_[currentLineIndex_];
        
        // 检查是否是结束词，如果是则不渲染文本
        if (currentLine.text == "end" || currentLine.text == "END") {
            // 结束词不显示，直接跳过
        } else if (font_) {
            SDL_Surface* textSurface = TTF_RenderUTF8_Blended_Wrapped(font_, currentLine.text.c_str(), textColor_, textArea_.w - 40);
            if (textSurface) {
                currentTextTexture_ = SDL_CreateTextureFromSurface(r, textSurface);
                textW_ = textSurface->w;
                textH_ = textSurface->h;
                SDL_FreeSurface(textSurface);
            }
        }
    }
    
    // 渲染当前文本
    if (currentTextTexture_) {
        SDL_Rect textRect{
            textArea_.x + 20,
            textArea_.y + (textArea_.h - textH_) / 2,
            textW_,
            textH_
        };
        SDL_RenderCopy(r, currentTextTexture_, nullptr, &textRect);
    }
    
    // 渲染按钮
    if (backButton_) backButton_->render(r);
    if (autoButton_) {
        // 高亮当前模式
        if (autoMode_) {
            SDL_SetRenderDrawColor(r, 100, 200, 100, 255);
            SDL_Rect highlight = autoButton_->getRect();
            highlight.x -= 2;
            highlight.y -= 2;
            highlight.w += 4;
            highlight.h += 4;
            SDL_RenderDrawRect(r, &highlight);
        }
        autoButton_->render(r);
    }
    if (skipButton_) {
        if (skipMode_) {
            SDL_SetRenderDrawColor(r, 200, 100, 100, 255);
            SDL_Rect highlight = skipButton_->getRect();
            highlight.x -= 2;
            highlight.y -= 2;
            highlight.w += 4;
            highlight.h += 4;
            SDL_RenderDrawRect(r, &highlight);
        }
        skipButton_->render(r);
    }
    if (nextButton_) nextButton_->render(r);
    
    // 渲染状态提示
    if (autoMode_) {
        SDL_Color statusColor = {100, 255, 100, 255};
        SDL_Surface* statusSurface = TTF_RenderUTF8_Blended(smallFont_, u8"自动播放中...", statusColor);
        if (statusSurface) {
            SDL_Texture* statusTexture = SDL_CreateTextureFromSurface(r, statusSurface);
            SDL_Rect statusRect{textArea_.x, textArea_.y - 30, statusSurface->w, statusSurface->h};
            SDL_RenderCopy(r, statusTexture, nullptr, &statusRect);
            SDL_DestroyTexture(statusTexture);
            SDL_FreeSurface(statusSurface);
        }
    }
    
    // 渲染播放完成提示
    if (autoExitTimer_ > 0.0f) {
        SDL_Color exitColor = {255, 200, 100, 255};
        std::string exitText = u8"播放完成，即将退出... (" + std::to_string(static_cast<int>(autoExitTimer_)) + "s)";
        SDL_Surface* exitSurface = TTF_RenderUTF8_Blended(smallFont_, exitText.c_str(), exitColor);
        if (exitSurface) {
            SDL_Texture* exitTexture = SDL_CreateTextureFromSurface(r, exitSurface);
            SDL_Rect exitRect{textArea_.x, textArea_.y + textArea_.h + 10, exitSurface->w, exitSurface->h};
            SDL_RenderCopy(r, exitTexture, nullptr, &exitRect);
            SDL_DestroyTexture(exitTexture);
            SDL_FreeSurface(exitSurface);
        }
    }
}

void TextPlayerState::setTextFile(const std::string& filePath) {
    textFile_ = filePath;
}

void TextPlayerState::setPresetName(const std::string& presetName) {
    presetName_ = presetName;
}

void TextPlayerState::setNextStateFactory(std::function<std::unique_ptr<State>()> factory) {
    nextStateFactory_ = factory;
}

void TextPlayerState::loadTextData(const std::string& filePath) {
    textLines_.clear();
    
    SDL_Log("尝试加载文本文件: %s, 预设名称: '%s'", filePath.c_str(), presetName_.c_str());
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        SDL_Log("无法打开文本文件: %s", filePath.c_str());
        // 创建默认文本数据
        textLines_ = {
            {"欢迎来到文本播放器！", "", false},
            {"这是一个演示文本。", "", false},
            {"你可以使用自动播放功能。", "", false},
            {"也可以手动点击下一句。", "", false},
            {"支持背景图片切换功能。", "assets/background1.png", true},
            {"这是最后一句文本。", "", false}
        };
        return;
    }
    
    SDL_Log("文件打开成功，开始读取内容");
    
    std::string line;
    bool foundPreset = false;
    bool readingPreset = false;
    
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
        SDL_Log("读取第 %d 行: '%s'", lineCount, line.c_str());
        if (line.empty()) continue;
        
        // 检查是否是目标预设词
        if (line.length() >= 8 && line.substr(0, 8) == "[PRESET:") {
            std::string presetName = line.substr(8);
            // 移除末尾的]如果有的话
            if (presetName.back() == ']') {
                presetName.pop_back();
            }
            
            SDL_Log("找到预设: '%s' (长度: %zu), 目标预设: '%s' (长度: %zu)", 
                   presetName.c_str(), presetName.length(), 
                   presetName_.c_str(), presetName_.length());
            
            // 添加详细的字符串比较调试
            bool exactMatch = (presetName == presetName_);
            SDL_Log("精确匹配: %s", exactMatch ? "是" : "否");
            
            if (exactMatch) {
                foundPreset = true;
                readingPreset = true;
                SDL_Log("匹配成功，开始读取预设内容");
                continue;
            } else if (readingPreset) {
                // 已经读取完目标预设，停止读取
                SDL_Log("读取完目标预设，停止读取");
                break;
            } else {
                continue; // 跳过其他预设
            }
        }
        
        // 如果正在读取目标预设
        if (readingPreset) {
            TextLine textLine;
            
            // 检查是否包含背景图片路径（以[BG:]开头）
            if (line.substr(0, 4) == "[BG:") {
                size_t endPos = line.find("]");
                if (endPos != std::string::npos) {
                    textLine.backgroundPath = line.substr(4, endPos - 4);
                    textLine.hasBackground = true;
                    // 获取文本内容（在]之后）
                    if (endPos + 1 < line.length()) {
                        textLine.text = line.substr(endPos + 1);
                    }
                }
            } else {
                textLine.text = line;
                textLine.hasBackground = false;
            }
            
            textLines_.push_back(textLine);
        }
    }
    
    file.close();
    SDL_Log("加载了 %zu 行文本数据", textLines_.size());
    if (textLines_.empty()) {
        SDL_Log("警告：没有加载到任何文本数据！预设名称: '%s'", presetName_.c_str());
    }
}

void TextPlayerState::startPlayback() {
    if (textLines_.empty()) return;
    
    isPlaying_ = true;
    currentLineIndex_ = 0;
    autoMode_ = false;
    skipMode_ = false;
    autoTimer_ = 0.0f;
    
    updateTextDisplay();
    SDL_Log("开始文本播放");
}

void TextPlayerState::stopPlayback() {
    isPlaying_ = false;
    autoMode_ = false;
    skipMode_ = false;
    SDL_Log("停止文本播放");
}

void TextPlayerState::loadBackground(const std::string& path) {
    if (path.empty()) return;
    
    // 如果背景没有变化，不需要重新加载
    if (currentBackgroundPath_ == path) return;
    
    // 清理旧背景
    if (backgroundTexture_) {
        SDL_DestroyTexture(backgroundTexture_);
        backgroundTexture_ = nullptr;
    }
    
    // 加载新背景
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (surface) {
        // 这里需要renderer，但当前方法没有App参数
        // 在实际使用时，这个方法会在render方法中调用，那时会有renderer
        SDL_FreeSurface(surface);
        currentBackgroundPath_ = path;
        SDL_Log("加载背景图片: %s", path.c_str());
    } else {
        SDL_Log("无法加载背景图片: %s, 错误: %s", path.c_str(), IMG_GetError());
    }
}

void TextPlayerState::updateTextDisplay() {
    if (currentLineIndex_ >= textLines_.size()) {
        stopPlayback();
        // 文本播放完成后，延迟1秒自动退出
        autoExitTimer_ = 1.0f;
        return;
    }
    
    const TextLine& currentLine = textLines_[currentLineIndex_];
    
    // 检查是否是结束词
    if (currentLine.text == "end" || currentLine.text == "END") {
        stopPlayback();
        // 检测到结束词，立即自动退出
        autoExitTimer_ = 0.5f;
        return;
    }
    
    // 更新背景路径（实际加载在render方法中进行）
    if (currentLine.hasBackground && !currentLine.backgroundPath.empty()) {
        currentBackgroundPath_ = currentLine.backgroundPath;
    }
    
    // 清理旧文本纹理
    if (currentTextTexture_) {
        SDL_DestroyTexture(currentTextTexture_);
        currentTextTexture_ = nullptr;
    }
    
    SDL_Log("显示文本: %s", currentLine.text.c_str());
}

void TextPlayerState::nextLine() {
    if (!isPlaying_) return;
    
    currentLineIndex_++;
    if (currentLineIndex_ >= textLines_.size()) {
        stopPlayback();
        return;
    }
    
    updateTextDisplay();
}

void TextPlayerState::toggleAuto() {
    autoMode_ = !autoMode_;
    skipMode_ = false;  // 关闭跳过模式
    autoTimer_ = 0.0f;
    SDL_Log("自动模式: %s", autoMode_ ? "开启" : "关闭");
}

void TextPlayerState::toggleSkip() {
    skipMode_ = !skipMode_;
    autoMode_ = false;  // 关闭自动模式
    autoTimer_ = 0.0f;
    SDL_Log("跳过模式: %s", skipMode_ ? "开启" : "关闭");
}

void TextPlayerState::cleanup() {
    if (font_) {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }
    if (smallFont_) {
        TTF_CloseFont(smallFont_);
        smallFont_ = nullptr;
    }
    if (backgroundTexture_) {
        SDL_DestroyTexture(backgroundTexture_);
        backgroundTexture_ = nullptr;
    }
    if (currentTextTexture_) {
        SDL_DestroyTexture(currentTextTexture_);
        currentTextTexture_ = nullptr;
    }
    
    delete backButton_;
    delete autoButton_;
    delete skipButton_;
    delete nextButton_;
    
    backButton_ = nullptr;
    autoButton_ = nullptr;
    skipButton_ = nullptr;
    nextButton_ = nullptr;
}
