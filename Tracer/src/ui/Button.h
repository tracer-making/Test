#pragma once

#include <SDL.h>
#include <functional>
#include <string>
struct SDL_Texture;
struct _TTF_Font;

class Button {
public:
	using Callback = std::function<void()>;

	Button();
	void setRect(const SDL_Rect& r);
	void setText(const std::string& text);
	void setOnClick(Callback cb);

	void handleEvent(const SDL_Event& e);
	void render(SDL_Renderer* renderer) const;
	void setFont(_TTF_Font* font, SDL_Renderer* renderer);

	bool isHovered() const { return hovered_; }

private:
	SDL_Rect rect_ {0,0,0,0};
	std::string text_;
	Callback onClick_;
	bool hovered_ = false;
	SDL_Texture* textTexture_ = nullptr;
	_TTF_Font* font_ = nullptr;
};

