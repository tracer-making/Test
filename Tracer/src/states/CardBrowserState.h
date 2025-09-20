#pragma once
#include "../core/State.h"
#include "../ui/Button.h"
#include "../core/Card.h"
#include "../core/Cards.h"
#include <vector>
#include <string>

class CardBrowserState : public State {
public:
    CardBrowserState();
    ~CardBrowserState();

    void onEnter(App& app) override;
    void onExit(App& app) override;
    void handleEvent(App& app, const SDL_Event& event) override;
    void update(App& app, float deltaTime) override;
    void render(App& app) override;

private:
    void layoutCards();
    void layoutButtons();
    void updatePageInfo();
    void filterByCategory(const std::string& category);
    void renderCard(App& app, const Card& card, int x, int y, int width, int height, bool selected = false);
    
    // UI Elements
    Button backButton_;
    Button prevPageButton_;
    Button nextPageButton_;
    Button allCategoryButton_;
    Button yuCategoryButton_;
    Button quanCategoryButton_;
    Button luCategoryButton_;
    Button jieCategoryButton_;
    Button linCategoryButton_;
    Button otherCategoryButton_;
    
    // Card Display
    std::vector<Card> allCards_;
    std::vector<Card> filteredCards_;
    std::vector<SDL_Rect> cardRects_;
    int currentPage_ = 0;
    int cardsPerPage_ = 10; // 5x2 grid
    std::string currentCategory_ = "全部";
    
    // Fonts
    _TTF_Font* titleFont_ = nullptr;
    _TTF_Font* cardNameFont_ = nullptr;
    _TTF_Font* cardStatFont_ = nullptr;
    _TTF_Font* pageInfoFont_ = nullptr;
    
    // Screen dimensions（适中尺寸）
    int screenW_ = 1600;
    int screenH_ = 1000;
    
    // State
    bool pendingBackToTest_ = false;
    int selectedCardIndex_ = -1;
};
