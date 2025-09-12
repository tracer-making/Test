#include "Button.h"
#include <SDL.h>
#if __has_include(<SDL2/SDL_ttf.h>)
#include <SDL2/SDL_ttf.h>
#else
#include <SDL_ttf.h>
#endif

Button::Button() = default;

void Button::setRect(const SDL_Rect& r) { rect_ = r; }
void Button::setText(const std::string& text) { text_ = text; }
void Button::setOnClick(Callback cb) { onClick_ = std::move(cb); }

void Button::setFont(_TTF_Font* font, SDL_Renderer* renderer) {
	font_ = font;
	if (!font_ || text_.empty()) return;
	if (textTexture_) { SDL_DestroyTexture(textTexture_); textTexture_ = nullptr; }
	SDL_Color col { 200, 220, 255, 255 }; // 浅蓝色文字
	SDL_Surface* s = TTF_RenderUTF8_Blended(font_, text_.c_str(), col);
	if (!s) return;
	textTexture_ = SDL_CreateTextureFromSurface(renderer, s);
	SDL_FreeSurface(s);
}

void Button::handleEvent(const SDL_Event& e) {
	if (e.type == SDL_MOUSEMOTION) {
		int mx = e.motion.x, my = e.motion.y;
		hovered_ = (mx >= rect_.x && mx <= rect_.x + rect_.w && my >= rect_.y && my <= rect_.y + rect_.h);
	}
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mx = e.button.x, my = e.button.y;
		bool inside = (mx >= rect_.x && mx <= rect_.x + rect_.w && my >= rect_.y && my <= rect_.y + rect_.h);
		if (inside && onClick_) onClick_();
	}
}

void Button::render(SDL_Renderer* renderer) const {
	// 科技感按钮颜色
	SDL_Color color = hovered_ ? SDL_Color{80, 120, 180, 200} : SDL_Color{60, 80, 120, 150};
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(renderer, &rect_);

	// 边框
	SDL_SetRenderDrawColor(renderer, 100, 140, 200, 255);
	SDL_RenderDrawRect(renderer, &rect_);

	// 悬停时的蓝色荧光效果 - 改进版
	if (hovered_) {
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
		
		// 更柔和的光晕效果 - 使用填充而不是边框
		// 内层光晕
		SDL_SetRenderDrawColor(renderer, 40, 100, 200, 20);
		SDL_Rect innerGlow = {rect_.x - 2, rect_.y - 2, rect_.w + 4, rect_.h + 4};
		SDL_RenderFillRect(renderer, &innerGlow);
		
		// 中层光晕
		SDL_SetRenderDrawColor(renderer, 30, 80, 180, 12);
		SDL_Rect midGlow = {rect_.x - 4, rect_.y - 4, rect_.w + 8, rect_.h + 8};
		SDL_RenderFillRect(renderer, &midGlow);
		
		// 外层光晕
		SDL_SetRenderDrawColor(renderer, 20, 60, 160, 6);
		SDL_Rect outerGlow = {rect_.x - 6, rect_.y - 6, rect_.w + 12, rect_.h + 12};
		SDL_RenderFillRect(renderer, &outerGlow);
		
		// 更精致的角点效果
		SDL_SetRenderDrawColor(renderer, 80, 150, 255, 100);
		// 四个角的亮点，更小更精致
		SDL_Rect corners[4] = {
			{rect_.x - 1, rect_.y - 1, 3, 3},                    // 左上
			{rect_.x + rect_.w - 2, rect_.y - 1, 3, 3},          // 右上
			{rect_.x - 1, rect_.y + rect_.h - 2, 3, 3},          // 左下
			{rect_.x + rect_.w - 2, rect_.y + rect_.h - 2, 3, 3} // 右下
		};
		for (const auto& corner : corners) {
			SDL_RenderFillRect(renderer, &corner);
		}

		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	}

	if (textTexture_) {
		int tw, th; SDL_QueryTexture(textTexture_, nullptr, nullptr, &tw, &th);
		SDL_Rect dst { rect_.x + (rect_.w - tw)/2, rect_.y + (rect_.h - th)/2, tw, th };

	// 悬停时的文字光晕效果 - 类似CSS text-shadow的光晕
	if (hovered_) {
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);

		// 类似CSS text-shadow的光晕效果
		if (font_ && !text_.empty()) {
			// 创建多个偏移的文字阴影，形成光晕
			struct ShadowLayer {
				int offsetX, offsetY;
				Uint8 alpha;
				float scale;
			};

			ShadowLayer shadows[] = {
				{-2, -2, 30, 1.0f},  // 左上
				{2, -2, 30, 1.0f},   // 右上
				{-2, 2, 30, 1.0f},   // 左下
				{2, 2, 30, 1.0f},    // 右下
				{0, -3, 25, 1.0f},   // 上
				{0, 3, 25, 1.0f},    // 下
				{-3, 0, 25, 1.0f},   // 左
				{3, 0, 25, 1.0f},    // 右
				{-1, -1, 40, 1.0f},  // 轻微偏移
				{1, 1, 40, 1.0f},    // 轻微偏移
				{0, 0, 60, 1.05f},   // 中心放大
			};

			SDL_Color glowColor = {100, 160, 255, 255}; // 亮蓝色

			for (const auto& shadow : shadows) {
				// 为每个阴影层创建文字纹理
				SDL_Surface* shadowSurf = TTF_RenderUTF8_Blended(font_, text_.c_str(), glowColor);
				if (shadowSurf) {
					// 修改alpha值
					SDL_SetSurfaceAlphaMod(shadowSurf, shadow.alpha);

					SDL_Texture* shadowTex = SDL_CreateTextureFromSurface(renderer, shadowSurf);
					if (shadowTex) {
						int sw, sh;
						SDL_QueryTexture(shadowTex, nullptr, nullptr, &sw, &sh);

						// 计算缩放后的尺寸
						int scaledW = static_cast<int>(sw * shadow.scale);
						int scaledH = static_cast<int>(sh * shadow.scale);

						SDL_Rect shadowDst = {
							dst.x + shadow.offsetX + (tw - scaledW) / 2,
							dst.y + shadow.offsetY + (th - scaledH) / 2,
							scaledW,
							scaledH
						};

						SDL_RenderCopy(renderer, shadowTex, nullptr, &shadowDst);
						SDL_DestroyTexture(shadowTex);
					}
					SDL_FreeSurface(shadowSurf);
				}
			}
		}

		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	}

		// 渲染正常文字
		SDL_RenderCopy(renderer, textTexture_, nullptr, &dst);
	}
}


