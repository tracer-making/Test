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
	SDL_Color col { 28, 28, 32, 255 };
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
	SDL_Color color = hovered_ ? SDL_Color{200, 200, 220, 255} : SDL_Color{160, 160, 180, 255};
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(renderer, &rect_);
	// 文本渲染将在后续用 SDL_ttf 接入；当前按钮以矩形占位
	SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255);
	SDL_RenderDrawRect(renderer, &rect_);
	if (textTexture_) {
		int tw, th; SDL_QueryTexture(textTexture_, nullptr, nullptr, &tw, &th);
		SDL_Rect dst { rect_.x + (rect_.w - tw)/2, rect_.y + (rect_.h - th)/2, tw, th };
		SDL_RenderCopy(renderer, textTexture_, nullptr, &dst);
	}
}


