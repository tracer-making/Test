#include "CardRenderer.h"
#include "../core/App.h"

static void drawDropOrBone(SDL_Renderer* r, bool bone, int x, int y, int size) {
    if (bone) {
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_Rect boneRect{ x, y, size, size };
        SDL_RenderFillRect(r, &boneRect);
        SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
        SDL_RenderDrawRect(r, &boneRect);
        SDL_RenderDrawLine(r, x + 1, y + 1, x + size - 2, y + size - 2);
        SDL_RenderDrawLine(r, x + size - 2, y + 1, x + 1, y + size - 2);
    } else {
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_Rect dropRect{ x, y, size, size };
        SDL_RenderFillRect(r, &dropRect);
        SDL_RenderDrawLine(r, x + size/2, y + size, x + size/2, y + size + size/2);
    }
}

void CardRenderer::renderCard(App& app,
                              const Card& card,
                              const SDL_Rect& rect,
                              _TTF_Font* nameFont,
                              _TTF_Font* statFont,
                              bool selected) {
    SDL_Renderer* r = app.getRenderer();

    SDL_SetRenderDrawColor(r, 240, 240, 240, 255);
    SDL_RenderFillRect(r, &rect);
    SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
    SDL_RenderDrawRect(r, &rect);
    if (selected) { 
        // 选中时更明显的蓝色边框
        SDL_SetRenderDrawColor(r, 50, 100, 255, 255); 
        SDL_RenderDrawRect(r, &rect);
        // 添加内边框增强选中效果
        SDL_SetRenderDrawColor(r, 100, 150, 255, 200);
        SDL_Rect innerRect = {rect.x + 2, rect.y + 2, rect.w - 4, rect.h - 4};
        SDL_RenderDrawRect(r, &innerRect);
    }

    SDL_SetRenderDrawColor(r, 180, 180, 180, 200);
    SDL_Rect dots[4] = { {rect.x+4,rect.y+4,2,2},{rect.x+rect.w-6,rect.y+4,2,2},{rect.x+4,rect.y+rect.h-6,2,2},{rect.x+rect.w-6,rect.y+rect.h-6,2,2} };
    for (const auto& d : dots) SDL_RenderFillRect(r, &d);

    // name
    int lineY = 0;
    if (nameFont) {
        SDL_Color nameCol{50, 40, 30, 255};
        SDL_Surface* s = TTF_RenderUTF8_Blended(nameFont, card.name.c_str(), nameCol);
        if (s) {
            SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
            int desiredNameH = SDL_max(12, (int)(rect.h * 0.16f));
            float scaleN = (float)desiredNameH / (float)s->h;
            int scaledW = (int)(s->w * scaleN);
            SDL_Rect ndst{ rect.x + (rect.w - scaledW)/2, rect.y + (int)(rect.h*0.04f), scaledW, desiredNameH };
            SDL_RenderCopy(r, t, nullptr, &ndst);
            SDL_DestroyTexture(t);
            int lineBase = ndst.y + ndst.h + SDL_max(2, (int)(rect.h * 0.015f));
            int thickness = SDL_max(1, (int)(rect.h * 0.007f));
            SDL_SetRenderDrawColor(r, 80, 70, 60, 220);
            for (int i = 0; i < thickness; ++i) SDL_RenderDrawLine(r, rect.x + 6, lineBase + i, rect.x + rect.w - 6, lineBase + i);
            lineY = lineBase;
            SDL_FreeSurface(s);
        }
    }

    // stats bottom
    if (statFont) {
        SDL_Color statCol{80, 50, 40, 255};
        int desiredStatH = SDL_max(12, (int)(rect.h * 0.18f));
        int margin = SDL_max(6, (int)(rect.h * 0.035f));
        std::string attackText = std::to_string(card.attack);
        SDL_Surface* sa = TTF_RenderUTF8_Blended(statFont, attackText.c_str(), statCol);
        if (sa) {
            SDL_Texture* ta = SDL_CreateTextureFromSurface(r, sa);
            float scaleA = (float)desiredStatH / (float)sa->h;
            int aW = (int)(sa->w * scaleA);
            SDL_Rect adst{ rect.x + margin, rect.y + rect.h - desiredStatH - margin, aW, desiredStatH };
            SDL_RenderCopy(r, ta, nullptr, &adst);
            SDL_DestroyTexture(ta);
            SDL_SetRenderDrawColor(r, 60, 70, 100, 60);
            int swordY = adst.y + adst.h + SDL_max(1, (int)(rect.h * 0.006f));
            int swordW = SDL_max(adst.w, (int)(rect.w * 0.22f));
            int swordX = adst.x;
            SDL_RenderDrawLine(r, swordX, swordY, swordX + swordW, swordY);
            SDL_RenderDrawLine(r, swordX + swordW/3, swordY-2, swordX + swordW/3, swordY+2);
            SDL_RenderDrawLine(r, swordX + swordW, swordY, swordX + swordW - 4, swordY - 3);
            SDL_RenderDrawLine(r, swordX + swordW, swordY, swordX + swordW - 4, swordY + 3);
            SDL_FreeSurface(sa);
        }
        std::string healthText = std::to_string(card.health);
        SDL_Color hpCol{160, 30, 40, 255};
        SDL_Surface* sh = TTF_RenderUTF8_Blended(statFont, healthText.c_str(), hpCol);
        if (sh) {
            SDL_Texture* th = SDL_CreateTextureFromSurface(r, sh);
            float scaleH = (float)desiredStatH / (float)sh->h;
            int hW = (int)(sh->w * scaleH);
            SDL_Rect hdst{ rect.x + rect.w - hW - margin, rect.y + rect.h - desiredStatH - margin, hW, desiredStatH };
            SDL_RenderCopy(r, th, nullptr, &hdst);
            SDL_DestroyTexture(th);
            SDL_FreeSurface(sh);
        }
    }

    // category + cost below line
    if (statFont && lineY > 0) {
        int desiredStatH = SDL_max(10, (int)(rect.h * 0.12f));
        SDL_Color categoryCol{100, 80, 60, 255};
        SDL_Surface* s = TTF_RenderUTF8_Blended(statFont, card.category.c_str(), categoryCol);
        if (s) {
            SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
            float scale = (float)desiredStatH / (float)s->h;
            int scaledW = (int)(s->w * scale);
            SDL_Rect dst{ rect.x + 6, lineY + 4, scaledW, desiredStatH };
            SDL_RenderCopy(r, t, nullptr, &dst);
            SDL_DestroyTexture(t);
            SDL_FreeSurface(s);
        }

        bool hasBoneCost = false;
        for (const auto& m : card.marks) if (m == "消耗骨头") { hasBoneCost = true; break; }
        if (card.sacrificeCost > 0 || hasBoneCost) {
            int dropSize = SDL_max(4, (int)(rect.h * 0.08f));
            int startY = lineY + 4;
            if (hasBoneCost) {
                // 消耗骨头的卡牌，显示白色骨头图标
                if (card.sacrificeCost > 3) {
                    // 数量超过3，显示"骨头×数量"
                    int startX = rect.x + rect.w - 6 - (dropSize + 20);
                    drawDropOrBone(r, true, startX, startY, dropSize);
                    SDL_SetRenderDrawColor(r, 60, 50, 40, 255);
                    int multX = startX + dropSize + 2;
                    int multY = startY + dropSize/2;
                    SDL_RenderDrawLine(r, multX, multY - 2, multX + 4, multY + 2);
                    SDL_RenderDrawLine(r, multX + 4, multY - 2, multX, multY + 2);
                    SDL_Color countCol{60, 50, 40, 255};
                    SDL_Surface* cs = TTF_RenderUTF8_Blended(statFont, std::to_string(card.sacrificeCost).c_str(), countCol);
                    if (cs) {
                        SDL_Texture* ct = SDL_CreateTextureFromSurface(r, cs);
                        float scale = (float)desiredStatH / (float)cs->h;
                        int w = (int)(cs->w * scale);
                        int h = (int)(cs->h * scale);
                        SDL_Rect cdst{ multX + 8, multY - h/2, w, h };
                        SDL_RenderCopy(r, ct, nullptr, &cdst);
                        SDL_DestroyTexture(ct);
                        SDL_FreeSurface(cs);
                    }
                } else {
                    // 数量不超过3，显示多个骨头图标
                    int dropSpacing = dropSize + 2;
                    int startX = rect.x + rect.w - 6 - (card.sacrificeCost * dropSpacing);
                    for (int i = 0; i < card.sacrificeCost; ++i) {
                        drawDropOrBone(r, true, startX + i * dropSpacing, startY, dropSize);
                    }
                }
            } else if (card.sacrificeCost > 3) {
                int startX = rect.x + rect.w - 6 - (dropSize + 20);
                drawDropOrBone(r, false, startX, startY, dropSize);
                SDL_SetRenderDrawColor(r, 60, 50, 40, 255);
                int multX = startX + dropSize + 2;
                int multY = startY + dropSize/2;
                SDL_RenderDrawLine(r, multX, multY - 2, multX + 4, multY + 2);
                SDL_RenderDrawLine(r, multX + 4, multY - 2, multX, multY + 2);
                SDL_Color countCol{60, 50, 40, 255};
                SDL_Surface* cs = TTF_RenderUTF8_Blended(statFont, std::to_string(card.sacrificeCost).c_str(), countCol);
                if (cs) {
                    SDL_Texture* ct = SDL_CreateTextureFromSurface(r, cs);
                    float scale = (float)desiredStatH / (float)cs->h;
                    int w = (int)(cs->w * scale);
                    int h = (int)(cs->h * scale);
                    SDL_Rect cdst{ multX + 8, multY - h/2, w, h };
                    SDL_RenderCopy(r, ct, nullptr, &cdst);
                    SDL_DestroyTexture(ct);
                    SDL_FreeSurface(cs);
                }
            } else {
                int dropSpacing = dropSize + 2;
                int startX = rect.x + rect.w - 6 - (card.sacrificeCost * dropSpacing);
                for (int i = 0; i < card.sacrificeCost; ++i) {
                    drawDropOrBone(r, hasBoneCost, startX + i * dropSpacing, startY, dropSize);
                }
            }
        }
    }

    // marks multi-line, skip bone
    if (statFont && lineY > 0 && !card.marks.empty()) {
        SDL_Color markCol{120, 80, 50, 255};
        int desiredStatH = SDL_max(8, (int)(rect.h * 0.10f));
        int lineHeight = desiredStatH + 2;
        int startY = lineY + 4 + desiredStatH + 8;
        int idx = 0;
        for (const auto& m : card.marks) {
            if (m == "消耗骨头") continue;
            SDL_Surface* s = TTF_RenderUTF8_Blended(statFont, m.c_str(), markCol);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
                float scale = (float)desiredStatH / (float)s->h;
                int w = (int)(s->w * scale);
                SDL_Rect dst{ rect.x + (rect.w - w)/2, startY + idx * lineHeight, w, desiredStatH };
                SDL_RenderCopy(r, t, nullptr, &dst);
                SDL_DestroyTexture(t);
                SDL_FreeSurface(s);
                idx++;
            }
        }
    }
}


