#include "TutorialState.h"
#include "../core/App.h"
#include "../ui/CardRenderer.h"
#include <SDL.h>
#include <SDL_ttf.h>

TutorialState::TutorialState() 
    : tutorialStarted_(false)
    , screenW_(1280)
    , screenH_(720)
    , font_(nullptr) {
}

void TutorialState::onEnter(App& app) {
    // 设置窗口尺寸
    screenW_ = 1280;
    screenH_ = 720;
    SDL_SetWindowSize(app.getWindow(), screenW_, screenH_);
    
    // 加载字体
    font_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 24);
    if (!font_) {
        SDL_Log("Failed to load tutorial font: %s", TTF_GetError());
    }
    
    tutorialStarted_ = false;
}

void TutorialState::onExit(App& app) {
    if (font_) {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }
}

void TutorialState::update(App& app, float dt) {
    // 更新教程系统
    CardRenderer::updateTutorial(dt);
}

void TutorialState::render(App& app) {
    SDL_Renderer* renderer = app.getRenderer();
    
    // 清屏
    SDL_SetRenderDrawColor(renderer, 20, 24, 34, 255);
    SDL_RenderClear(renderer);
    
    // 渲染教程
    CardRenderer::renderTutorial(renderer, font_, screenW_, screenH_);
}

void TutorialState::handleEvent(App& app, const SDL_Event& e) {
    // 如果教程未开始，开始教程
    if (!tutorialStarted_) {
        startTutorial();
        tutorialStarted_ = true;
        return;
    }
    
    // 如果教程正在进行，只处理点击事件
    if (CardRenderer::isTutorialActive()) {
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            CardRenderer::handleTutorialClick();
        }
        // 阻止所有其他事件
        return;
    }
    
    // 教程结束，可以处理其他事件或退出
    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_ESCAPE) {
            // 退出教程状态
            app.setState(nullptr);
        }
    }
}

void TutorialState::setTutorialTexts(const std::vector<std::string>& texts) {
    tutorialTexts_ = texts;
}

void TutorialState::setTutorialHighlights(const std::vector<SDL_Rect>& highlights) {
    tutorialHighlights_ = highlights;
}

void TutorialState::startTutorial() {
    if (!tutorialTexts_.empty()) {
        CardRenderer::startTutorial(tutorialTexts_, tutorialHighlights_);
    }
}
