#include "VictoryState.h"
#include "../core/App.h"
#include "../core/State.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>

VictoryState::VictoryState() {
    // 初始化字体
    titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 48);
    infoFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 24);
    
    if (!titleFont_) {
        std::cout << "[VICTORY] 无法加载标题字体" << std::endl;
    }
    if (!infoFont_) {
        std::cout << "[VICTORY] 无法加载信息字体" << std::endl;
    }
    
    // 创建按钮
    backToMenuButton_ = new Button();
    restartGameButton_ = new Button();
    
    // 设置按钮文本和位置
    backToMenuButton_->setText("返回主菜单");
    restartGameButton_->setText("重新开始");
    
    // 设置按钮位置和大小
    SDL_Rect backButtonRect{ (screenW_ - 400) / 2, 450, 400, 100 };
    SDL_Rect restartButtonRect{ (screenW_ - 400) / 2, 580, 400, 100 };
    backToMenuButton_->setRect(backButtonRect);
    restartGameButton_->setRect(restartButtonRect);
    
    // 设置按钮字体（需要在renderer可用时设置）
    
    // 设置最终统计信息
    finalStats_ = "游戏完成！\n";
    finalStats_ += "剩余蜡烛: " + std::to_string(App::getRemainingCandles()) + "\n";
    finalStats_ += "获得文脉: " + std::to_string(0) + "\n"; // 这里可以从WenMaiStore获取
}

VictoryState::~VictoryState() {
    if (titleFont_) {
        TTF_CloseFont(titleFont_);
        titleFont_ = nullptr;
    }
    if (infoFont_) {
        TTF_CloseFont(infoFont_);
        infoFont_ = nullptr;
    }
    
    if (backToMenuButton_) {
        delete backToMenuButton_;
        backToMenuButton_ = nullptr;
    }
    if (restartGameButton_) {
        delete restartGameButton_;
        restartGameButton_ = nullptr;
    }
}

void VictoryState::onEnter(App& app) {
    std::cout << "[VICTORY] 进入胜利界面" << std::endl;
    
    // 重置动画状态
    titleScale_ = 0.0f;
    titleTargetScale_ = 1.0f;
    titleAnimTime_ = 0.0f;
    isTitleAnimating_ = true;
    
    // 设置按钮字体
    if (backToMenuButton_ && infoFont_) {
        backToMenuButton_->setFont(infoFont_, app.getRenderer());
    }
    if (restartGameButton_ && infoFont_) {
        restartGameButton_->setFont(infoFont_, app.getRenderer());
    }
    
    // 更新最终统计信息
    finalStats_ = "游戏完成！\n";
    finalStats_ += "剩余蜡烛: " + std::to_string(App::getRemainingCandles()) + "\n";
    finalStats_ += "获得文脉: " + std::to_string(0) + "\n"; // 这里可以从WenMaiStore获取
}

void VictoryState::onExit(App& app) {
    std::cout << "[VICTORY] 退出胜利界面" << std::endl;
}

void VictoryState::handleEvent(App& app, const SDL_Event& e) {
    // 处理按钮事件
    if (backToMenuButton_) {
        backToMenuButton_->handleEvent(e);
    }
    if (restartGameButton_) {
        restartGameButton_->handleEvent(e);
    }
    
    // 检查按钮点击
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mouseX = e.button.x;
        int mouseY = e.button.y;
        
        // 检查返回主菜单按钮
        if (backToMenuButton_) {
            const SDL_Rect& rect = backToMenuButton_->getRect();
            if (mouseX >= rect.x && mouseX < rect.x + rect.w && 
                mouseY >= rect.y && mouseY < rect.y + rect.h) {
                std::cout << "[VICTORY] 返回主菜单按钮被点击" << std::endl;
                pendingGoMainMenu_ = true;
            }
        }
        
        // 检查重新开始按钮
        if (restartGameButton_) {
            const SDL_Rect& rect = restartGameButton_->getRect();
            if (mouseX >= rect.x && mouseX < rect.x + rect.w && 
                mouseY >= rect.y && mouseY < rect.y + rect.h) {
                std::cout << "[VICTORY] 重新开始按钮被点击" << std::endl;
                pendingRestartGame_ = true;
            }
        }
    }
}

void VictoryState::update(App& app, float dt) {
    // 更新标题动画
    if (isTitleAnimating_) {
        titleAnimTime_ += dt;
        if (titleAnimTime_ >= titleAnimDuration_) {
            titleAnimTime_ = titleAnimDuration_;
            isTitleAnimating_ = false;
        }
        
        // 使用缓动函数
        float t = titleAnimTime_ / titleAnimDuration_;
        t = 1.0f - (1.0f - t) * (1.0f - t); // easeOutQuad
        titleScale_ = titleTargetScale_ * t;
    }
    
    // 处理状态切换
    if (pendingGoMainMenu_) {
        pendingGoMainMenu_ = false;
        // 这里应该跳转到主菜单，但需要确认主菜单的状态类名
        std::cout << "[VICTORY] 跳转到主菜单" << std::endl;
        // app.setState(std::unique_ptr<State>(static_cast<State*>(new MainMenuState())));
        return;
    }
    
    if (pendingRestartGame_) {
        pendingRestartGame_ = false;
        // 重新开始游戏，跳转到地图探索
        std::cout << "[VICTORY] 重新开始游戏" << std::endl;
        // 这里需要重置游戏状态，然后跳转到地图探索
        // app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState())));
        return;
    }
}

void VictoryState::render(App& app) {
    SDL_Renderer* r = app.getRenderer();
    
    // 清屏
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);
    
    // 绘制背景渐变
    for (int y = 0; y < screenH_; ++y) {
        float t = (float)y / screenH_;
        int red = (int)(20 + (100 - 20) * t);
        int green = (int)(50 + (150 - 50) * t);
        int blue = (int)(100 + (200 - 100) * t);
        SDL_SetRenderDrawColor(r, red, green, blue, 255);
        SDL_RenderDrawLine(r, 0, y, screenW_, y);
    }
    
    // 绘制标题
    if (titleFont_) {
        SDL_Color titleColor{ 255, 215, 0, 255 }; // 金色
        SDL_Surface* titleSurface = TTF_RenderUTF8_Blended(titleFont_, victoryMessage_.c_str(), titleColor);
        if (titleSurface) {
            SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(r, titleSurface);
            if (titleTexture) {
                // 应用缩放动画
                int titleW = (int)(titleSurface->w * titleScale_);
                int titleH = (int)(titleSurface->h * titleScale_);
                int titleX = (screenW_ - titleW) / 2;
                int titleY = 150;
                
                SDL_Rect titleRect{ titleX, titleY, titleW, titleH };
                SDL_RenderCopy(r, titleTexture, nullptr, &titleRect);
                SDL_DestroyTexture(titleTexture);
            }
            SDL_FreeSurface(titleSurface);
        }
    }
    
    // 绘制统计信息
    if (infoFont_) {
        SDL_Color infoColor{ 255, 255, 255, 255 }; // 白色
        SDL_Surface* infoSurface = TTF_RenderUTF8_Blended(infoFont_, finalStats_.c_str(), infoColor);
        if (infoSurface) {
            SDL_Texture* infoTexture = SDL_CreateTextureFromSurface(r, infoSurface);
            if (infoTexture) {
                int infoX = (screenW_ - infoSurface->w) / 2;
                int infoY = 300;
                SDL_Rect infoRect{ infoX, infoY, infoSurface->w, infoSurface->h };
                SDL_RenderCopy(r, infoTexture, nullptr, &infoRect);
                SDL_DestroyTexture(infoTexture);
            }
            SDL_FreeSurface(infoSurface);
        }
    }
    
    // 绘制按钮
    if (backToMenuButton_) {
        backToMenuButton_->render(r);
    }
    
    if (restartGameButton_) {
        restartGameButton_->render(r);
    }
    
    // 绘制装饰性元素
    SDL_SetRenderDrawColor(r, 255, 215, 0, 100); // 半透明金色
    for (int i = 0; i < 20; ++i) {
        int x = rand() % screenW_;
        int y = rand() % screenH_;
        int size = 2 + (rand() % 4);
        SDL_Rect star{ x, y, size, size };
        SDL_RenderFillRect(r, &star);
    }
    
    // 绘制操作说明（左下角）
    if (infoFont_) {
        SDL_Color helpColor{ 200, 200, 200, 255 }; // 浅灰色文字
        const char* helpLines[] = {
            u8"胜利界面操作说明：",
            u8"• 点击\"返回主菜单\"回到开始界面",
            u8"• 点击\"重新开始\"开始新的游戏",
            u8"• 恭喜您完成了游戏！",
            u8"• 感谢您的游玩！"
        };
        
        int lineCount = sizeof(helpLines) / sizeof(helpLines[0]);
        int x = 20;
        int y = screenH_ - 150;
        
        for (int i = 0; i < lineCount; ++i) {
            SDL_Surface* s = TTF_RenderUTF8_Blended(infoFont_, helpLines[i], helpColor);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                if (t) {
                    SDL_Rect dst{ x, y, s->w, s->h };
                    SDL_RenderCopy(r, t, nullptr, &dst);
                    SDL_DestroyTexture(t);
                }
                SDL_FreeSurface(s);
            }
            y += 25;
        }
    }
}
