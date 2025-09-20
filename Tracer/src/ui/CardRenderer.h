#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include "../core/Card.h"

class App;

class CardRenderer {
public:
    static void renderCard(App& app,
                           const Card& card,
                           const SDL_Rect& rect,
                           _TTF_Font* nameFont,
                           _TTF_Font* statFont,
                           bool selected = false);
};


