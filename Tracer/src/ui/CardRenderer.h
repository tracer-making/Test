#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include "../core/Card.h"

class App;

class CardRenderer {
private:
    static SDL_Texture* bloodTexture_;
    static bool bloodTextureLoaded_;
    static SDL_Texture* boneTexture_;
    static bool boneTextureLoaded_;
    
    static void loadBloodTexture(SDL_Renderer* renderer);
    static void loadBoneTexture(SDL_Renderer* renderer);

public:
    static void drawBloodDrop(SDL_Renderer* renderer, int x, int y, int size);
    static void drawBone(SDL_Renderer* renderer, int x, int y, int size);
    static void renderCard(App& app,
                           const Card& card,
                           const SDL_Rect& rect,
                           _TTF_Font* nameFont,
                           _TTF_Font* statFont,
                           bool selected = false);
    
    // 检测鼠标是否点击在印记上
    static std::string getClickedMark(const Card& card, 
                                      const SDL_Rect& rect, 
                                      int mouseX, int mouseY,
                                      _TTF_Font* statFont);
    
    // 处理印记点击事件（使用全局印记提示系统）
    static void handleMarkClick(const Card& card, 
                                const SDL_Rect& rect, 
                                int mouseX, int mouseY,
                                _TTF_Font* statFont);
    
    // 处理印记悬停事件（鼠标悬停显示提示）
    static void handleMarkHover(const Card& card, 
                                const SDL_Rect& rect, 
                                int mouseX, int mouseY,
                                _TTF_Font* statFont);
    
    // 渲染印记效果提示
    static void renderMarkTooltip(App& app, 
                                  const std::string& markName,
                                  const std::string& description,
                                  int x, int y,
                                  _TTF_Font* font);
    
    // 全局印记提示渲染（供所有状态使用）
    static void renderGlobalMarkTooltip(App& app, _TTF_Font* font);
    
    // 统一的状态消息渲染（供所有状态使用）
    static void renderStatusMessage(SDL_Renderer* renderer, const std::string& message, 
                                   _TTF_Font* font, int screenW, int screenH);
    
    // 教程系统
    static void startTutorial(const std::vector<std::string>& texts, 
                             const std::vector<SDL_Rect>& highlightRects = {});
    static void startInteractiveTutorial(const std::vector<std::string>& texts,
                                        const std::vector<std::string>& actions,
                                        const std::vector<SDL_Rect>& highlightRects = {});
    static void updateTutorial(float dt);
    static void renderTutorial(SDL_Renderer* renderer, _TTF_Font* font, int screenW, int screenH);
    static void handleTutorialClick();
    static bool isTutorialActive();
    static bool isInteractiveTutorialActive();
    static void endTutorial();
    static void nextTutorialStep();
    static void waitForUserAction();
};


