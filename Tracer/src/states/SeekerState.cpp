#include "SeekerState.h"
#include "TestState.h"
#include "../core/App.h"
#include "../core/Deck.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <sstream>

SeekerState::SeekerState() = default;

SeekerState::~SeekerState() {
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (smallFont_) TTF_CloseFont(smallFont_);
	if (nameFont_) TTF_CloseFont(nameFont_);
	if (statFont_) TTF_CloseFont(statFont_);
	if (titleTex_) SDL_DestroyTexture(titleTex_);
	if (backButton_) delete backButton_;
	if (confirmButton_) delete confirmButton_;
}

void SeekerState::onEnter(App& app) {
	SDL_Log("SeekerState::onEnter - Start");
	
	int w, h;
	SDL_GetWindowSize(app.getWindow(), &w, &h);
	screenW_ = w;
	screenH_ = h;
	SDL_Log("Screen size: %d x %d", screenW_, screenH_);

	// 加载字体（与牌库界面保持一致）
	titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 64);
	smallFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 16);
	nameFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 22);
	statFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 24);
	if (!titleFont_ || !smallFont_ || !nameFont_ || !statFont_) {
		SDL_Log("TTF_OpenFont failed: %s", TTF_GetError());
		return;
	}

	// 创建标题纹理
	SDL_Color titleCol{ 200, 230, 255, 255 };
	SDL_Surface* ts = TTF_RenderUTF8_Blended(titleFont_, u8"寻物人", titleCol);
	if (ts) {
		titleTex_ = SDL_CreateTextureFromSurface(app.getRenderer(), ts);
		SDL_FreeSurface(ts);
	}

	// 创建返回按钮
	backButton_ = new Button();
	if (backButton_) {
		SDL_Rect backRect{ 20, 20, 60, 40 };
		backButton_->setRect(backRect);
		backButton_->setText(u8"返回");
		if (smallFont_) backButton_->setFont(smallFont_, app.getRenderer());
		backButton_->setOnClick([this]() {
			pendingBackToTest_ = true;
		});
	}

	// 创建确认按钮
	confirmButton_ = new Button();
	if (confirmButton_) {
		SDL_Rect confirmRect{ screenW_ / 2 - 50, screenH_ - 80, 100, 40 };
		confirmButton_->setRect(confirmRect);
		confirmButton_->setText(u8"破壁");
		if (smallFont_) confirmButton_->setFont(smallFont_, app.getRenderer());
		confirmButton_->setOnClick([this]() {
			if (selectedIndex_ >= 0 && selectedIndex_ < (int)artifacts_.size()) {
				auto& a = artifacts_[selectedIndex_];
				if (!a.revealed) {
					a.revealed = true;
					// 根据稀有度显示不同奖励文本
					if (a.rarity >= 2) {
						message_ = u8"破壁成功！获得传奇之墨/传奇卡！";
					} else if (a.rarity == 1) {
						message_ = u8"破壁成功！获得稀有奖励！";
					} else {
						message_ = u8"破壁成功！获得普通奖励！";
					}
					if (!a.rewardText.empty()) message_ += (u8" " + a.rewardText);
				}
			} else {
				message_ = u8"请先选择一件文物！";
			}
		});
	}

	// 生成未知文物并布局
	generateArtifacts();
	layoutArtifactsGrid();
	
	SDL_Log("SeekerState::onEnter - Complete");
}

void SeekerState::onExit(App& app) {
	SDL_Log("SeekerState::onExit");
}

void SeekerState::handleEvent(App& app, const SDL_Event& e) {
	if (backButton_) backButton_->handleEvent(e);
	if (confirmButton_) confirmButton_->handleEvent(e);

	// 处理文物选择
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mx = e.button.x, my = e.button.y;
		for (size_t i = 0; i < artifacts_.size(); ++i) {
			const SDL_Rect& rect = artifacts_[i].rect;
			if (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h) {
				selectedIndex_ = (int)i;
				message_ = u8"已选择：未知文物";
				break;
			}
		}
	}
}

void SeekerState::update(App& app, float dt) {
	if (pendingBackToTest_) {
		pendingBackToTest_ = false;
		// 返回到测试界面
		app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState())));
	}
}

void SeekerState::render(App& app) {
	SDL_Renderer* r = app.getRenderer();
	
	// 水墨风格背景（与牌库界面一致）
	SDL_SetRenderDrawColor(r, 20, 24, 34, 255);
	SDL_RenderClear(r);
	
	// 水墨背景点
	SDL_SetRenderDrawColor(r, 80, 70, 60, 60);
	srand(4242); // 固定种子确保一致性
	for (int i = 0; i < 400; ++i) {
		int x = rand() % screenW_;
		int y = rand() % screenH_;
		SDL_RenderDrawPoint(r, x, y);
	}

	// 渲染标题
	if (titleTex_) {
		int titleW, titleH;
		SDL_QueryTexture(titleTex_, nullptr, nullptr, &titleW, &titleH);
		SDL_Rect dst{ screenW_ / 2 - titleW / 2, 80, titleW, titleH };
		SDL_RenderCopy(r, titleTex_, nullptr, &dst);
	}

	// 渲染说明文字
	if (smallFont_) {
		SDL_Color textCol{ 200, 200, 200, 255 };
		SDL_Surface* textSurf = TTF_RenderUTF8_Blended(smallFont_, 
			u8"从三件未知文物中选择其一进行破壁，揭示并获得对应奖励", textCol);
		if (textSurf) {
			SDL_Texture* textTex = SDL_CreateTextureFromSurface(r, textSurf);
			SDL_Rect textRect{ screenW_ / 2 - textSurf->w / 2, 150, textSurf->w, textSurf->h };
			SDL_RenderCopy(r, textTex, nullptr, &textRect);
			SDL_DestroyTexture(textTex);
			SDL_FreeSurface(textSurf);
		}
	}

	// 渲染三件未知文物（使用水墨卡面样式）
	for (size_t i = 0; i < artifacts_.size(); ++i) {
		SDL_Rect rect = artifacts_[i].rect;
		const bool isSelected = (selectedIndex_ == (int)i);
		const bool revealed = artifacts_[i].revealed;

		// 选中高亮
		if (isSelected) {
			SDL_SetRenderDrawColor(r, 255, 215, 0, 200);
			SDL_Rect highlightRect{ rect.x - 3, rect.y - 3, rect.w + 6, rect.h + 6 };
			SDL_RenderFillRect(r, &highlightRect);
		}

		// 纸面底色
		SDL_SetRenderDrawColor(r, 235, 230, 220, 230);
		SDL_RenderFillRect(r, &rect);
		// 边框
		SDL_SetRenderDrawColor(r, 60, 50, 40, 220);
		SDL_RenderDrawRect(r, &rect);
		// 角落装饰点
		SDL_SetRenderDrawColor(r, 120, 110, 100, 150);
		SDL_Rect dots[4] = {
			{rect.x + 4, rect.y + 4, 2, 2},
			{rect.x + rect.w - 6, rect.y + 4, 2, 2},
			{rect.x + 4, rect.y + rect.h - 6, 2, 2},
			{rect.x + rect.w - 6, rect.y + rect.h - 6, 2, 2}
		};
		for (const auto& d : dots) SDL_RenderFillRect(r, &d);

		// 标题（未知/已揭示）
		if (nameFont_) {
			SDL_Color nameCol{50, 40, 30, 255};
			std::string title = revealed ? artifacts_[i].title : std::string(u8"未知文物");
			SDL_Surface* s = TTF_RenderUTF8_Blended(nameFont_, title.c_str(), nameCol);
			if (s) {
				SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
				int desiredNameH = SDL_max(12, (int)(rect.h * 0.16f));
				float scaleN = (float)desiredNameH / (float)s->h;
				int scaledW = (int)(s->w * scaleN);
				int nx = rect.x + (rect.w - scaledW) / 2;
				SDL_Rect ndst{ nx, rect.y + (int)(rect.h * 0.06f), scaledW, desiredNameH };
				SDL_RenderCopy(r, t, nullptr, &ndst);
				SDL_DestroyTexture(t);
				// 分割线
				int lineY = ndst.y + ndst.h + SDL_max(2, (int)(rect.h * 0.015f));
				int thickness = SDL_max(1, (int)(rect.h * 0.007f));
				SDL_SetRenderDrawColor(r, 80, 70, 60, 220);
				for (int j = 0; j < thickness; ++j) SDL_RenderDrawLine(r, rect.x + 6, lineY + j, rect.x + rect.w - 6, lineY + j);
				SDL_FreeSurface(s);
			}
		}

		// 中间区域：未知遮罩/图案 或 揭示后的奖励描述
		if (smallFont_) {
			if (!revealed) {
				SDL_Color col{80, 70, 60, 200};
				SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, u8"破壁以见真", col);
				if (s) {
					SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
					SDL_Rect dst{ rect.x + (rect.w - s->w)/2, rect.y + rect.h/2 - s->h/2, s->w, s->h };
					SDL_RenderCopy(r, t, nullptr, &dst);
					SDL_DestroyTexture(t);
					SDL_FreeSurface(s);
				}
			} else {
				SDL_Color col{120, 30, 30, 230};
				std::string text = artifacts_[i].rewardText.empty() ? std::string(u8"获得奖励！") : artifacts_[i].rewardText;
				SDL_Surface* s = TTF_RenderUTF8_Blended(smallFont_, text.c_str(), col);
				if (s) {
					SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
					SDL_Rect dst{ rect.x + (rect.w - s->w)/2, rect.y + rect.h/2 - s->h/2, s->w, s->h };
					SDL_RenderCopy(r, t, nullptr, &dst);
					SDL_DestroyTexture(t);
					SDL_FreeSurface(s);
				}
			}
		}

		// 底部：稀有度指示（揭示后显示具体稀有度）
		if (statFont_) {
			int desiredStatH = SDL_max(12, (int)(rect.h * 0.16f));
			int margin = SDL_max(6, (int)(rect.h * 0.035f));
			SDL_Color rareCol{80, 50, 40, 255};
			const char* rareText = revealed ? (artifacts_[i].rarity>=2?"传奇":(artifacts_[i].rarity==1?"稀有":"普通")) : "?";
			SDL_Surface* s = TTF_RenderUTF8_Blended(statFont_, rareText, rareCol);
			if (s) {
				SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
				float scale = (float)desiredStatH / (float)s->h;
				int wscaled = (int)(s->w * scale);
				SDL_Rect dst{ rect.x + rect.w - wscaled - margin, rect.y + rect.h - desiredStatH - margin, wscaled, desiredStatH };
				SDL_RenderCopy(r, t, nullptr, &dst);
				SDL_DestroyTexture(t);
				SDL_FreeSurface(s);
			}
		}
	}

	// 渲染消息
	if (!message_.empty() && smallFont_) {
		SDL_Color msgCol{ 255, 255, 0, 255 };
		SDL_Surface* msgSurf = TTF_RenderUTF8_Blended(smallFont_, message_.c_str(), msgCol);
		if (msgSurf) {
			SDL_Texture* msgTex = SDL_CreateTextureFromSurface(r, msgSurf);
			SDL_Rect msgRect{ screenW_ / 2 - msgSurf->w / 2, screenH_ - 150, msgSurf->w, msgSurf->h };
			SDL_RenderCopy(r, msgTex, nullptr, &msgRect);
			SDL_DestroyTexture(msgTex);
			SDL_FreeSurface(msgSurf);
		}
	}

	// 渲染按钮
	if (backButton_) backButton_->render(r);
	if (confirmButton_) confirmButton_->render(r);
}

void SeekerState::generateArtifacts() {
	artifacts_.clear();
	// 简单权重：普通 65%，稀有 28%，传奇 7%
	auto rollRarity = [](){ int r = rand()%100; if (r<7) return 2; if (r<35) return 1; return 0; };
	const char* titles[3] = { "远古碎片", "封泥残章", "秘宝匣" };
	const char* rareLabel[3] = { "普通", "稀有", "传奇" };
	for (int i=0;i<3;++i) {
		Artifact a;
		a.rarity = rollRarity();
		a.title = std::string(titles[i]) + std::string(" (") + rareLabel[a.rarity] + ")";
		switch (a.rarity) {
			case 2: a.rewardText = u8"获得传奇之墨或传奇卡"; break;
			case 1: a.rewardText = u8"获得稀有之墨或稀有卡"; break;
			default: a.rewardText = u8"获得普通之墨或普通卡"; break;
		}
		artifacts_.push_back(a);
	}
}

void SeekerState::layoutArtifactsGrid() {
	int n = (int)artifacts_.size();
	if (n<=0) return;
	// 布局与牌库风格一致，3列1行优先
	int marginX = 40;
	int topY = 200;
	int bottomMargin = 100;
	int availableW = SDL_max(200, screenW_ - marginX * 2);
	int availableH = SDL_max(160, screenH_ - topY - bottomMargin);
	const float aspect = 2.0f/3.0f;
	int cols = SDL_min(3, n);
	int rows = (n + cols - 1) / cols;
	int gap = 18;
	int cardW = (availableW - (cols - 1) * gap) / cols;
	int cardH = (int)(cardW / aspect);
	int totalH = rows * cardH + (rows - 1) * gap;
	if (totalH > availableH) {
		float scale = (float)availableH / (float)totalH;
		cardW = SDL_max(24, (int)(cardW * scale));
		cardH = SDL_max(36, (int)(cardH * scale));
	}
	int totalW = cols * cardW + (cols - 1) * gap;
	int startX = (screenW_ - totalW) / 2;
	int startY = topY + (availableH - (rows * cardH + (rows - 1) * gap)) / 2;
	startY = SDL_max(topY, startY);
	for (int i=0;i<n;++i) {
		int r = i / cols;
		int c = i % cols;
		artifacts_[i].rect = { startX + c * (cardW + gap), startY + r * (cardH + gap), cardW, cardH };
	}
}
