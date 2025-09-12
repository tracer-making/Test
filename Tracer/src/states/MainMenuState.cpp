#include "MainMenuState.h"
#include "BattleState.h"
#include "../core/App.h"
#include "../ui/Button.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <memory>

MainMenuState::MainMenuState() = default;
MainMenuState::~MainMenuState() = default;

void MainMenuState::onEnter(App& app) {
	srand(static_cast<unsigned int>(time(nullptr)));
	int w, h; SDL_GetWindowSize(app.getWindow(), &w, &h); screenW_ = w; screenH_ = h;

	// 加载中文字体（请确保文件存在）
	font_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 120); // 标题更大
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 20); // 按钮字体稍大
	if (!font_ || !smallFont_) { SDL_Log("TTF_OpenFont failed: %s", TTF_GetError()); }
	else {
		SDL_Color titleCol { 170, 210, 255, 255 };
		SDL_Surface* ts = TTF_RenderUTF8_Blended(font_, u8"溯洄遗梦", titleCol);
		if (ts) {
			titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), ts);
			titleW_ = ts->w; titleH_ = ts->h;
			SDL_FreeSurface(ts);
		}
	}


	std::vector<Button*> owned;
	owned.reserve(4);
	for (int i = 0; i < 4; ++i) owned.push_back(new Button());
	buttons_[0] = owned[0];
	buttons_[1] = owned[1];
	buttons_[2] = owned[2];
	buttons_[3] = owned[3];

	int bw = 200, bh = 45; // 按钮更小
	int cx = screenW_ / 2 - bw / 2;
	int cy = screenH_ / 2 - (bh * 4 + 15 * 3) / 2 + 60; // 按钮往下移
	const char* labels[4] = {u8"开始游戏", u8"设置", u8"藏馆", u8"退出"};
	for (int i = 0; i < 4; ++i) {
		SDL_Rect r { cx, cy + i * (bh + 15), bw, bh };
		buttons_[i]->setRect(r);
		buttons_[i]->setText(labels[i]);
		if (smallFont_) buttons_[i]->setFont(smallFont_, app.getRenderer());
	}
	// 不设置按钮回调，在handleEvent中直接处理
	buttons_[1]->setOnClick([]() { SDL_Log("Settings clicked"); });
	buttons_[2]->setOnClick([]() { SDL_Log("Collection clicked"); });
	buttons_[3]->setOnClick([&app]() { SDL_Event quit; quit.type = SDL_QUIT; SDL_PushEvent(&quit); });

	// 初始化乱码数据流（更快、多一点、小一点）
	streams_.clear();
	const char* charSet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()[]{}|;:,.<>?";
	int charSetSize = strlen(charSet);
	int streamCount = screenW_ / 12; // 增加密度
	for (int i = 0; i < streamCount; ++i) {
		DataStream s;
		s.x = i * 12 + (rand() % 6);
		s.y = static_cast<float>(rand() % screenH_);
		s.speed = 120.f + static_cast<float>(rand() % 180); // 120-300 px/s，更快

		// 生成随机字符序列
		int charCount = 12 + (rand() % 16); // 12-28个字符
		s.characters.clear();
		s.alphas.clear();
		for (int c = 0; c < charCount; ++c) {
			s.characters += charSet[rand() % charSetSize];
			s.alphas.push_back(static_cast<Uint8>(120 + rand() % 130)); // 120-250透明度
		}
		streams_.push_back(s);
	}

	// 初始化装饰性花纹
	decorations_.clear();
	int decoCount = 15; // 增加数量
	for (int i = 0; i < decoCount; ++i) {
		Decoration d;
		d.x = rand() % screenW_;
		d.y = rand() % screenH_;
		d.radius = 15 + (rand() % 35); // 稍微小一点
		d.alpha = static_cast<Uint8>(20 + rand() % 60);
		d.isArc = (rand() % 2 == 0);
		decorations_.push_back(d);
	}

	// 初始化闪烁星光
	stars_.clear();
	int starCount = 25; // 星星数量
	for (int i = 0; i < starCount; ++i) {
		Star star;
		star.x = rand() % screenW_;
		star.y = rand() % screenH_;
		star.brightness = static_cast<float>(rand()) / RAND_MAX;
		star.twinkleSpeed = 2.0f + static_cast<float>(rand() % 300) / 100.0f; // 2.0-5.0
		stars_.push_back(star);
	}
}

void MainMenuState::onExit(App& app) {
	for (auto* b : buttons_) { delete b; }
	buttons_ = {nullptr, nullptr, nullptr, nullptr};
	if (font_) { TTF_CloseFont(font_); font_ = nullptr; }
	if (smallFont_) { TTF_CloseFont(smallFont_); smallFont_ = nullptr; }
	if (titleTex_) { SDL_DestroyTexture(titleTex_); titleTex_ = nullptr; }
}

void MainMenuState::handleEvent(App& app, const SDL_Event& e) {
	// 处理按钮事件
	for (auto* b : buttons_) if (b) b->handleEvent(e);

	// 特殊处理开始游戏按钮（避免lambda生命周期问题）
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mx = e.button.x, my = e.button.y;
		if (buttons_[0]) {
			const SDL_Rect& rect = buttons_[0]->getRect();
			if (mx >= rect.x && mx <= rect.x + rect.w &&
				my >= rect.y && my <= rect.y + rect.h) {
				app.setState(std::unique_ptr<State>(static_cast<State*>(new BattleState())));
			}
		}
	}
}

void MainMenuState::update(App& app, float dt) {
	const char* charSet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()[]{}|;:,.<>?";
	int charSetSize = strlen(charSet);

	for (auto& s : streams_) {
		s.y += s.speed * dt;
		if (s.y - static_cast<float>(s.characters.length() * 10) > screenH_) { // 小字体间距
			s.y = -static_cast<float>(rand() % screenH_);
			s.speed = 120.f + static_cast<float>(rand() % 180);

			// 重新生成字符序列
			int charCount = 12 + (rand() % 16);
			s.characters.clear();
			s.alphas.clear();
			for (int c = 0; c < charCount; ++c) {
				s.characters += charSet[rand() % charSetSize];
				s.alphas.push_back(static_cast<Uint8>(120 + rand() % 130));
			}
		}
	}

	// 更新星光闪烁
	for (auto& star : stars_) {
		star.brightness += star.twinkleSpeed * dt;
		if (star.brightness > 1.0f) star.brightness = 0.0f;
	}
}

void MainMenuState::render(App& app) {
	SDL_Renderer* r = app.getRenderer();
	// 深色背景
	SDL_SetRenderDrawColor(r, 8, 10, 16, 255);
	SDL_Rect bg {0,0,screenW_,screenH_};
	SDL_RenderFillRect(r, &bg);

	// 装饰性花纹
	SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
	for (const auto& d : decorations_) {
		SDL_SetRenderDrawColor(r, 100, 150, 200, d.alpha);
		if (d.isArc) {
			// 绘制弧线（简化为一组点）
			for (int i = 0; i < 8; ++i) {
				float angle = static_cast<float>(i) * 3.14159f / 4.0f;
				int px = d.x + static_cast<int>(cosf(angle) * d.radius * 0.7f);
				int py = d.y + static_cast<int>(sinf(angle) * d.radius * 0.7f);
				SDL_Rect dot { px - 2, py - 2, 4, 4 };
				SDL_RenderFillRect(r, &dot);
			}
		} else {
			// 绘制圆形（简化为一组点）
			for (int i = 0; i < 12; ++i) {
				float angle = static_cast<float>(i) * 3.14159f / 6.0f;
				int px = d.x + static_cast<int>(cosf(angle) * d.radius);
				int py = d.y + static_cast<int>(sinf(angle) * d.radius);
				SDL_Rect dot { px - 1, py - 1, 2, 2 };
				SDL_RenderFillRect(r, &dot);
			}
		}
	}

	// 闪烁星光
	for (const auto& star : stars_) {
		Uint8 alpha = static_cast<Uint8>(star.brightness * 200 + 55); // 55-255
		SDL_SetRenderDrawColor(r, 150, 200, 255, alpha);
		SDL_Rect starRect { star.x - 1, star.y - 1, 2, 2 };
		SDL_RenderFillRect(r, &starRect);
	}


	// 乱码字符数据流（小字体，快一点）
	if (smallFont_) {
		for (const auto& s : streams_) {
			int charY = static_cast<int>(s.y);
			for (size_t i = 0; i < s.characters.length(); ++i) {
				if (charY + 10 < 0 || charY > screenH_) {
					charY += 10;
					continue;
				}

				char charStr[2] = { s.characters[i], '\0' };
				SDL_Color charCol = { 60, 150, 255, s.alphas[i] };
				SDL_Surface* charSurf = TTF_RenderUTF8_Blended(smallFont_, charStr, charCol);
				if (charSurf) {
					SDL_Texture* charTex = SDL_CreateTextureFromSurface(r, charSurf);
					if (charTex) {
						SDL_Rect dst { s.x, charY, charSurf->w, charSurf->h };
						SDL_RenderCopy(r, charTex, nullptr, &dst);

						SDL_DestroyTexture(charTex);
					}
					SDL_FreeSurface(charSurf);
				}
				charY += 8; // 小字体间距
			}
		}
	}

	// 标题及其装饰花纹
	if (titleTex_) {
		SDL_Rect dst { screenW_/2 - titleW_/2, screenH_/2 - 240, titleW_, titleH_ };

		// 标题光晕效果 - 类似CSS text-shadow的真实光晕
		if (font_) {
			SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_ADD);

			const char* titleText = u8"溯洄遗梦";

			// 类似CSS text-shadow的光晕效果 - 多层阴影
			struct TextShadow {
				int offsetX, offsetY;
				Uint8 alpha;
				float scale;
				SDL_Color color;
			};

			TextShadow titleShadows[] = {
				// 外围扩散光晕
				{-6, -6, 15, 1.0f, {20, 60, 120, 255}},   // 左上远
				{6, -6, 15, 1.0f, {20, 60, 120, 255}},    // 右上远
				{-6, 6, 15, 1.0f, {20, 60, 120, 255}},    // 左下远
				{6, 6, 15, 1.0f, {20, 60, 120, 255}},     // 右下远

				// 中层光晕
				{-4, -4, 25, 1.0f, {40, 90, 160, 255}},   // 左上中
				{4, -4, 25, 1.0f, {40, 90, 160, 255}},    // 右上中
				{-4, 4, 25, 1.0f, {40, 90, 160, 255}},    // 左下中
				{4, 4, 25, 1.0f, {40, 90, 160, 255}},     // 右下中

				// 主要方向光晕
				{0, -8, 20, 1.0f, {30, 80, 150, 255}},    // 上
				{0, 8, 20, 1.0f, {30, 80, 150, 255}},     // 下
				{-8, 0, 20, 1.0f, {30, 80, 150, 255}},    // 左
				{8, 0, 20, 1.0f, {30, 80, 150, 255}},     // 右

				// 内层光晕
				{-2, -2, 45, 1.0f, {70, 130, 200, 255}},  // 左上近
				{2, -2, 45, 1.0f, {70, 130, 200, 255}},   // 右上近
				{-2, 2, 45, 1.0f, {70, 130, 200, 255}},   // 左下近
				{2, 2, 45, 1.0f, {70, 130, 200, 255}},    // 右下近

				// 核心光晕
				{0, 0, 80, 1.02f, {100, 160, 255, 255}},  // 中心放大
				{-1, -1, 60, 1.01f, {90, 150, 240, 255}}, // 轻微偏移
				{1, 1, 60, 1.01f, {90, 150, 240, 255}},   // 轻微偏移
			};

			for (const auto& shadow : titleShadows) {
				// 为每个阴影层创建文字纹理
				SDL_Surface* shadowSurf = TTF_RenderUTF8_Blended(font_, titleText, shadow.color);
				if (shadowSurf) {
					// 设置透明度
					SDL_SetSurfaceAlphaMod(shadowSurf, shadow.alpha);

					SDL_Texture* shadowTex = SDL_CreateTextureFromSurface(r, shadowSurf);
					if (shadowTex) {
						int sw, sh;
						SDL_QueryTexture(shadowTex, nullptr, nullptr, &sw, &sh);

						// 计算缩放后的尺寸
						int scaledW = static_cast<int>(sw * shadow.scale);
						int scaledH = static_cast<int>(sh * shadow.scale);

						SDL_Rect shadowDst = {
							dst.x + shadow.offsetX + (dst.w - scaledW) / 2,
							dst.y + shadow.offsetY + (dst.h - scaledH) / 2,
							scaledW,
							scaledH
						};

						SDL_RenderCopy(r, shadowTex, nullptr, &shadowDst);
						SDL_DestroyTexture(shadowTex);
					}
					SDL_FreeSurface(shadowSurf);
				}
			}

			SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
		}

		// 渲染正常标题文字
		SDL_RenderCopy(r, titleTex_, nullptr, &dst);

		// 标题周围的艺术花纹（更亮的科技感）
		SDL_SetRenderDrawColor(r, 150, 200, 255, 120);
		// 左边装饰
		for (int i = 0; i < 6; ++i) {
			int x = dst.x - 25 - i * 6;
			int y = dst.y + dst.h / 2 + (i - 3) * 10;
			SDL_Rect deco { x - 3, y - 3, 6, 6 };
			SDL_RenderFillRect(r, &deco);
		}
		// 右边装饰
		for (int i = 0; i < 6; ++i) {
			int x = dst.x + dst.w + 25 + i * 6;
			int y = dst.y + dst.h / 2 + (i - 3) * 10;
			SDL_Rect deco { x - 3, y - 3, 6, 6 };
			SDL_RenderFillRect(r, &deco);
		}

		// 添加标题下方的科技线条
		SDL_SetRenderDrawColor(r, 100, 160, 220, 150);
		int lineY = dst.y + dst.h + 20;
		int lineLength = dst.w + 100;
		int lineX = dst.x - 50;
		for (int i = 0; i < lineLength; i += 8) {
			if (i % 16 == 0) { // 每隔一段画一个点
				SDL_Rect dot { lineX + i, lineY, 4, 2 };
				SDL_RenderFillRect(r, &dot);
			}
		}
	}

	// 按钮及其装饰花纹
	for (size_t i = 0; i < buttons_.size(); ++i) {
		if (buttons_[i]) {
			buttons_[i]->render(r);

			// 每个按钮的侧边装饰（科技感增强）
			SDL_Rect buttonRect = buttons_[i]->getRect();

			// 左边科技装饰线
			SDL_SetRenderDrawColor(r, 120, 180, 255, 150);
			for (int j = 0; j < 4; ++j) {
				int x = buttonRect.x - 18 - j * 4;
				int y = buttonRect.y + j * 10;
				SDL_Rect deco { x - 2, y - 1, 4, 2 };
				SDL_RenderFillRect(r, &deco);
			}

			// 右边科技装饰线
			for (int j = 0; j < 4; ++j) {
				int x = buttonRect.x + buttonRect.w + 18 + j * 4;
				int y = buttonRect.y + j * 10;
				SDL_Rect deco { x - 2, y - 1, 4, 2 };
				SDL_RenderFillRect(r, &deco);
			}

			// 按钮角落的光点
			SDL_SetRenderDrawColor(r, 150, 200, 255, 120);
			// 左上角
			SDL_Rect corner1 { buttonRect.x - 8, buttonRect.y - 8, 3, 3 };
			SDL_RenderFillRect(r, &corner1);
			// 右上角
			SDL_Rect corner2 { buttonRect.x + buttonRect.w + 5, buttonRect.y - 8, 3, 3 };
			SDL_RenderFillRect(r, &corner2);
			// 左下角
			SDL_Rect corner3 { buttonRect.x - 8, buttonRect.y + buttonRect.h + 5, 3, 3 };
			SDL_RenderFillRect(r, &corner3);
			// 右下角
			SDL_Rect corner4 { buttonRect.x + buttonRect.w + 5, buttonRect.y + buttonRect.h + 5, 3, 3 };
			SDL_RenderFillRect(r, &corner4);
		}
	}
}

void MainMenuState::startBattle(App& app) {
	app.setState(std::unique_ptr<State>(new BattleState()));
}