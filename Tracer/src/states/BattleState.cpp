#include "BattleState.h"
#include "TestState.h"
#include "../core/App.h"
#include <SDL.h>
#include <cmath>

BattleState::BattleState() = default;
BattleState::~BattleState() = default;

void BattleState::onEnter(App& app) {
	// 获取屏幕尺寸
	int w, h;
	SDL_GetWindowSize(app.getWindow(), &w, &h);
	screenW_ = w;
	screenH_ = h;

	// 初始化水墨装饰参数 - 预计算避免闪烁
	srand(12345); // 使用固定种子确保每次运行装饰都一样

	// 1. 初始化墨点装饰
	inkDots_.clear();
	for (int i = 0; i < 30; ++i) {
		InkDot dot;
		dot.x = rand() % screenW_;
		dot.y = rand() % screenH_;
		dot.size = 1 + rand() % 3;
		inkDots_.push_back(dot);
	}

	// 2. 初始化晕染效果
	stainEffects_.clear();
	for (int i = 0; i < 8; ++i) {
		StainEffect stain;
		stain.centerX = rand() % screenW_;
		stain.centerY = rand() % screenH_;
		stain.maxRadius = 15 + rand() % 20;
		stainEffects_.push_back(stain);
	}

	// 3. 初始化书法风格装饰线条
	calligraphyLines_.clear();
	// 左边装饰线
	for (int y = 100; y < screenH_ - 100; y += 50) {
		CalligraphyLine line;
		line.startX = 20 + rand() % 10;
		line.endX = 40 + rand() % 15;
		line.y = y;
		line.offset = (rand() % 3) - 1;
		calligraphyLines_.push_back(line);
	}
	// 右边装饰线
	for (int y = 150; y < screenH_ - 150; y += 60) {
		CalligraphyLine line;
		line.startX = screenW_ - 40 - rand() % 15;
		line.endX = screenW_ - 20 - rand() % 10;
		line.y = y;
		line.offset = (rand() % 3) - 1;
		calligraphyLines_.push_back(line);
	}

	// 4. 初始化云纹装饰
	cloudDecorations_.clear();
	for (int i = 0; i < 5; ++i) {
		CloudDecoration cloud;
		cloud.centerX = rand() % (screenW_ - 100) + 50;
		cloud.centerY = rand() % (screenH_ - 100) + 50;
		cloud.size = 20 + rand() % 30;
		cloud.points.clear();

		// 预计算云朵点
		for (int y = -cloud.size; y <= cloud.size; ++y) {
			for (int x = -cloud.size; x <= cloud.size; ++x) {
				float dist = sqrt(x*x + y*y);
				if (dist <= cloud.size && rand() % 100 < 30) {
					int px = cloud.centerX + x;
					int py = cloud.centerY + y;
					if (px >= 0 && px < screenW_ && py >= 0 && py < screenH_) {
						cloud.points.push_back({px, py});
					}
				}
			}
		}
		cloudDecorations_.push_back(cloud);
	}

	// 5. 初始化标题装饰点
	titleDecorationPoints_.clear();
	int titleY = 40;
	int titleHeight = 80;
	for (int y = titleY; y < titleY + titleHeight; ++y) {
		for (int x = 0; x < screenW_; ++x) {
			if (rand() % 200 < 3) {
				titleDecorationPoints_.push_back({x, y});
			}
		}
	}

	// 6. 初始化角落装饰
	cornerDecorationPoints_.clear();
	// 左上角装饰
	{
		int cornerX = 20, cornerY = 20;
		int size = 40;
		for (int y = 0; y < size; ++y) {
			for (int x = 0; x < size - y; ++x) {
				if (rand() % 100 < 40) {
					int px = cornerX + x, py = cornerY + y;
					if (px >= 0 && px < screenW_ && py >= 0 && py < screenH_) {
						cornerDecorationPoints_.push_back({px, py});
					}
				}
			}
		}
	}
	// 右上角装饰
	{
		int cornerX = screenW_ - 60, cornerY = 20;
		int size = 40;
		for (int y = 0; y < size; ++y) {
			for (int x = 0; x < size - y; ++x) {
				if (rand() % 100 < 40) {
					int px = cornerX + x, py = cornerY + y;
					if (px >= 0 && px < screenW_ && py >= 0 && py < screenH_) {
						cornerDecorationPoints_.push_back({px, py});
					}
				}
			}
		}
	}

	// 计算UI元素位置和尺寸

	// 阴阳鱼 - 更大尺寸，左边正中位置
	int yinYangSize = 200;
	yinYangPoolRect_ = {
		30,  // 左边距
		(screenH_ - yinYangSize) / 2,  // 垂直居中
		yinYangSize,
		yinYangSize
	};

	// 卡牌槽 - 3x4网格，下面四个稍大，有分界线
	int normalSlotWidth = 80;
	int normalSlotHeight = 100;
	int largeSlotWidth = 95;   // 下面四个稍大
	int largeSlotHeight = 115;
	int slotSpacing = 15;

	// 计算整体布局宽度（使用正常尺寸计算，因为分界线位置基于此）
	int boardWidth = BOARD_COLS * normalSlotWidth + (BOARD_COLS - 1) * slotSpacing;
	int normalBoardHeight = 2 * normalSlotHeight + slotSpacing; // 前两行
	int largeBoardHeight = largeSlotHeight; // 第三行
	int totalBoardHeight = normalBoardHeight + largeBoardHeight + 30; // 加分界线间距

	int boardStartX = (screenW_ - boardWidth) / 2; // 水平居中
	int boardStartY = (screenH_ - totalBoardHeight) / 2; // 垂直居中

	for (int row = 0; row < BOARD_ROWS; ++row) {
		for (int col = 0; col < BOARD_COLS; ++col) {
			int slotIndex = row * BOARD_COLS + col;
			int currentSlotWidth = (row == 2) ? largeSlotWidth : normalSlotWidth;
			int currentSlotHeight = (row == 2) ? largeSlotHeight : normalSlotHeight;

			// 第三行往下移一些，留出分界线空间
			int yOffset = (row < 2) ? row * (normalSlotHeight + slotSpacing) :
						 2 * (normalSlotHeight + slotSpacing) + 30; // 分界线间距

			// 第三行居中对齐（因为尺寸不同）
			int xOffset = (row == 2) ?
				col * (largeSlotWidth + slotSpacing) + (boardWidth - (BOARD_COLS * largeSlotWidth + (BOARD_COLS - 1) * slotSpacing)) / 2 :
				col * (normalSlotWidth + slotSpacing);

			cardSlots_[slotIndex] = {
				boardStartX + xOffset,
				boardStartY + yOffset,
				currentSlotWidth,
				currentSlotHeight
			};
		}
	}

	// 铃铛 - 右上角，传统样式（更大，避免与牌堆重叠）
	int bellSize = 60;
	bellRect_ = {
		screenW_ - bellSize - 40,
		40,  // 移到右上角，避免与牌堆重叠
		bellSize,
		bellSize
	};

	// 手牌区 - 底部中央，横向排列，根据当前手牌数量动态调整
	int handCardWidth = 70;
	int handCardHeight = 90;
	int handSpacing = 10;
	int totalHandWidth = currentHandCards_ * handCardWidth + (currentHandCards_ - 1) * handSpacing;
	int handStartX = (screenW_ - totalHandWidth) / 2;
	int handY = screenH_ - handCardHeight - 30;

	handCardRects_.clear();
	for (int i = 0; i < currentHandCards_; ++i) {
		handCardRects_.push_back({
			handStartX + i * (handCardWidth + handSpacing),
			handY,
			handCardWidth,
			handCardHeight
		});
	}

	// 右侧牌堆 - 抽牌堆和弃牌堆（与卡牌槽同尺寸）
	int pileWidth = 80;   // 与普通卡牌槽宽度相同
	int pileHeight = 100; // 与普通卡牌槽高度相同
	int pileSpacing = 20;
	int pileY = screenH_ - pileHeight - 50;

	// 抽牌堆（左侧）
	drawPileRect_ = {
		screenW_ - pileWidth * 2 - pileSpacing - 50,
		pileY,
		pileWidth,
		pileHeight
	};

	// 弃牌堆（右侧）
	discardPileRect_ = {
		screenW_ - pileWidth - 50,
		pileY,
		pileWidth,
		pileHeight
	};

	// 初始化阴阳平衡
	yinYangBalance_ = 50;

	// 创建返回测试按钮（左上角）
	if (!backToTestButton_) backToTestButton_ = new Button();
	if (backToTestButton_) {
		SDL_Rect r{20, 20, 120, 36};
		backToTestButton_->setRect(r);
		backToTestButton_->setText("返回测试");
		// BattleState 暂无字体系统，按钮无字渲染也可使用纯底色；如需字体，可接入TTF
		backToTestButton_->setOnClick([this]() {
			pendingGoTest_ = true;
		});
	}
}

void BattleState::onExit(App& app) {
	// 清理资源（如果有的话）
	if (backToTestButton_) { delete backToTestButton_; backToTestButton_ = nullptr; }
}

void BattleState::handleEvent(App& app, const SDL_Event& e) {
	// 处理鼠标悬停和点击事件
	if (backToTestButton_) backToTestButton_->handleEvent(e);
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mouseX = e.button.x;
		int mouseY = e.button.y;

		// 检查是否点击了铃铛
		if (mouseX >= bellRect_.x && mouseX <= bellRect_.x + bellRect_.w &&
			mouseY >= bellRect_.y && mouseY <= bellRect_.y + bellRect_.h) {
			// 铃铛被点击 - 可以添加铃铛功能，比如暂停或特殊技能
			SDL_Log("Bell clicked! (铃铛被点击)");
		}

		// 检查是否点击了卡牌槽
		for (int i = 0; i < TOTAL_SLOTS; ++i) {
			const auto& slot = cardSlots_[i];
			if (mouseX >= slot.x && mouseX <= slot.x + slot.w &&
				mouseY >= slot.y && mouseY <= slot.y + slot.h) {
				// 卡牌槽被点击 - 可以添加放置卡牌的逻辑
				SDL_Log("Card slot %d clicked! (卡牌槽%d被点击)", i, i);
			}
		}

		// 检查是否点击了手牌
		for (size_t i = 0; i < handCardRects_.size(); ++i) {
			const auto& card = handCardRects_[i];
			if (mouseX >= card.x && mouseX <= card.x + card.w &&
				mouseY >= card.y && mouseY <= card.y + card.h) {
				// 手牌被点击 - 可以添加出牌逻辑
				SDL_Log("Hand card %zu clicked! (手牌%zu被点击)", i, i);
			}
		}

		// 检查是否点击了抽牌堆
		if (mouseX >= drawPileRect_.x && mouseX <= drawPileRect_.x + drawPileRect_.w &&
			mouseY >= drawPileRect_.y && mouseY <= drawPileRect_.y + drawPileRect_.h) {
			// 抽牌堆被点击 - 抽牌逻辑
			SDL_Log("Draw pile clicked! (抽牌堆被点击)");
			// 示例：增加手牌数量
			if (currentHandCards_ < 10) {
				currentHandCards_++;
				// 重新计算手牌位置
				int handCardWidth = 70;
				int handCardHeight = 90;
				int handSpacing = 10;
				int totalHandWidth = currentHandCards_ * handCardWidth + (currentHandCards_ - 1) * handSpacing;
				int handStartX = (screenW_ - totalHandWidth) / 2;
				int handY = screenH_ - handCardHeight - 30;

				handCardRects_.clear();
				for (int i = 0; i < currentHandCards_; ++i) {
					handCardRects_.push_back({
						handStartX + i * (handCardWidth + handSpacing),
						handY,
						handCardWidth,
						handCardHeight
					});
				}
			}
		}

		// 检查是否点击了弃牌堆
		if (mouseX >= discardPileRect_.x && mouseX <= discardPileRect_.x + discardPileRect_.w &&
			mouseY >= discardPileRect_.y && mouseY <= discardPileRect_.y + discardPileRect_.h) {
			// 弃牌堆被点击 - 弃牌逻辑
			SDL_Log("Discard pile clicked! (弃牌堆被点击)");
			// 示例：减少手牌数量
			if (currentHandCards_ > 0) {
				currentHandCards_--;
				// 重新计算手牌位置
				int handCardWidth = 70;
				int handCardHeight = 90;
				int handSpacing = 10;
				int totalHandWidth = currentHandCards_ * handCardWidth + (currentHandCards_ - 1) * handSpacing;
				int handStartX = (screenW_ - totalHandWidth) / 2;
				int handY = screenH_ - handCardHeight - 30;

				handCardRects_.clear();
				for (int i = 0; i < currentHandCards_; ++i) {
					handCardRects_.push_back({
						handStartX + i * (handCardWidth + handSpacing),
						handY,
						handCardWidth,
						handCardHeight
					});
				}
			}
		}
	}
}

void BattleState::update(App& app, float dt) {
	// 更新游戏逻辑
	if (pendingGoTest_) {
		pendingGoTest_ = false;
		app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState())));
		return;
	}

	// 阴阳平衡缓慢随机变化（模拟游戏平衡机制）
	yinYangBalance_ += (rand() % 3 - 1); // -1, 0, 或 1
	yinYangBalance_ = SDL_max(0, SDL_min(100, yinYangBalance_)); // 限制在0-100范围内

	// 这里可以添加更多的游戏逻辑，比如：
	// - 卡牌冷却时间
	// - 粒子效果动画
	// - AI对手行动
	// - 游戏状态检查（胜利/失败）
}

void BattleState::render(App& app) {
	SDL_Renderer* renderer = app.getRenderer();

	// 绘制深色战斗背景
	SDL_SetRenderDrawColor(renderer,
		backgroundColor_.r, backgroundColor_.g, backgroundColor_.b, backgroundColor_.a);
	SDL_Rect bgRect = {0, 0, screenW_, screenH_};
	SDL_RenderFillRect(renderer, &bgRect);

	// 水墨风格的丰富装饰 - 使用预计算参数避免闪烁
	// 1. 随机墨点装饰
	SDL_SetRenderDrawColor(renderer, 100, 90, 80, 60);
	for (const auto& dot : inkDots_) {
		for (int dy = -dot.size; dy <= dot.size; ++dy) {
			for (int dx = -dot.size; dx <= dot.size; ++dx) {
				if (dx*dx + dy*dy <= dot.size*dot.size) {
					int px = dot.x + dx, py = dot.y + dy;
					if (px >= 0 && px < screenW_ && py >= 0 && py < screenH_) {
						SDL_RenderDrawPoint(renderer, px, py);
					}
				}
			}
		}
	}

	// 2. 水墨晕染效果 - 随机墨迹
	for (const auto& stain : stainEffects_) {
		for (int r = stain.maxRadius; r > 0; --r) {
			int alpha = (stain.maxRadius - r) * 255 / stain.maxRadius;
			alpha = SDL_min(alpha, 100); // 控制透明度
			if (alpha > 0) {
				SDL_SetRenderDrawColor(renderer, 60, 50, 40, alpha);
				// 绘制圆形晕染
				for (int angle = 0; angle < 360; angle += 3) {
					float rad = angle * 3.14159f / 180.0f;
					int x = stain.centerX + (int)(cos(rad) * r);
					int y = stain.centerY + (int)(sin(rad) * r);
					if (x >= 0 && x < screenW_ && y >= 0 && y < screenH_) {
						SDL_RenderDrawPoint(renderer, x, y);
					}
				}
			}
		}
	}

	// 3. 书法风格的装饰线条
	SDL_SetRenderDrawColor(renderer, 80, 70, 60, 120);
	for (const auto& line : calligraphyLines_) {
		int lineLength = line.endX - line.startX;
		for (int i = 0; i < lineLength; ++i) {
			int x = line.startX + i;
			int y = line.y + line.offset;
			if (x >= 0 && x < screenW_ && y >= 0 && y < screenH_) {
				SDL_RenderDrawPoint(renderer, x, y);
			}
		}
	}

	// 4. 水墨风格的云纹装饰
	SDL_SetRenderDrawColor(renderer, 70, 60, 50, 40);
	for (const auto& cloud : cloudDecorations_) {
		for (const auto& point : cloud.points) {
			SDL_RenderDrawPoint(renderer, point.first, point.second);
		}
	}

	// 5. 水墨风格标题装饰 - "水墨对弈"
	{
		// 绘制标题区域的淡雅装饰
		SDL_SetRenderDrawColor(renderer, 90, 80, 70, 50);
		for (const auto& point : titleDecorationPoints_) {
			SDL_RenderDrawPoint(renderer, point.first, point.second);
		}

		// 绘制一些书法风格的装饰笔画
		SDL_SetRenderDrawColor(renderer, 50, 40, 30, 80);
		int titleY = 40;
		// 左边装饰
		for (int i = 0; i < 15; ++i) {
			int x = 50 + i * 3;
			int y = titleY + 20 + (i % 3); // 使用固定模式而不是随机
			if (x < screenW_ && y < screenH_) {
				SDL_RenderDrawPoint(renderer, x, y);
			}
		}
		// 右边装饰
		for (int i = 0; i < 15; ++i) {
			int x = screenW_ - 50 - i * 3;
			int y = titleY + 20 + (i % 3);
			if (x >= 0 && y < screenH_) {
				SDL_RenderDrawPoint(renderer, x, y);
			}
		}
	}

	// 6. 角落的传统装饰图案
	SDL_SetRenderDrawColor(renderer, 60, 50, 40, 70);
	for (const auto& point : cornerDecorationPoints_) {
		SDL_RenderDrawPoint(renderer, point.first, point.second);
	}

	// 绘制传统的八卦阴阳鱼
	int yinYangCenterX = yinYangPoolRect_.x + yinYangPoolRect_.w / 2;
	int yinYangCenterY = yinYangPoolRect_.y + yinYangPoolRect_.h / 2;
	int yinYangRadius = yinYangPoolRect_.h / 2 - 5;

	// 绘制外圆边框 - 水墨风格
	SDL_SetRenderDrawColor(renderer, 60, 50, 40, 200);
	for (int i = 0; i < yinYangRadius; ++i) {
		int outerRadius = yinYangRadius - i;
		for (int angle = 0; angle < 360; ++angle) {
			float radian = angle * 3.14159f / 180.0f;
			int x = yinYangCenterX + static_cast<int>(cos(radian) * outerRadius);
			int y = yinYangCenterY + static_cast<int>(sin(radian) * outerRadius);
			if (i == 0) { // 外边框
				SDL_RenderDrawPoint(renderer, x, y);
			}
		}
	}

	// 绘制阴阳鱼主体 - 标准八卦阴阳鱼
	for (int y = -yinYangRadius; y <= yinYangRadius; ++y) {
		for (int x = -yinYangRadius; x <= yinYangRadius; ++x) {
			float distance = sqrt(x * x + y * y);
			if (distance <= yinYangRadius) {
				bool isYin = false;

				// 标准的阴阳鱼：上半部分为阳(白)，下半部分为阴(黑)
				// 这是最经典的八卦阴阳鱼样式
				if (y <= 0) {
					isYin = true; // 下半部分是阴（黑色）
				} else {
					isYin = false; // 上半部分是阳（白色）
				}

				// 阴阳鱼的"眼睛" - 传统八卦中的阴阳点
				float eyeRadius = yinYangRadius * 0.15f;
				float eyeOffsetY = yinYangRadius * 0.4f;

				// 阳鱼中的阴点（黑色圆点在白色区域）
				float yangEyeDist = sqrt(x * x + (y - eyeOffsetY) * (y - eyeOffsetY));
				if (yangEyeDist <= eyeRadius) {
					isYin = true; // 阳中的阴
				}

				// 阴鱼中的阳点（白色圆点在黑色区域）
				float yinEyeDist = sqrt(x * x + (y + eyeOffsetY) * (y + eyeOffsetY));
				if (yinEyeDist <= eyeRadius) {
					isYin = false; // 阴中的阳
				}

				// 根据阴阳平衡动态调整颜色强度
				Uint8 alpha = 255;
				if (yinYangBalance_ < 50) {
					// 阴盛，增强黑色
					if (isYin) alpha = 255;
					else alpha = static_cast<Uint8>(255 * yinYangBalance_ / 50.0f);
				} else {
					// 阳盛，增强白色
					if (!isYin) alpha = 255;
					else alpha = static_cast<Uint8>(255 * (100 - yinYangBalance_) / 50.0f);
				}

				if (isYin) {
					// 深墨色（阴）
					SDL_SetRenderDrawColor(renderer, 30, 25, 20, alpha);
				} else {
					// 浅墨色（阳）
					SDL_SetRenderDrawColor(renderer, 200, 190, 180, alpha);
				}

				SDL_RenderDrawPoint(renderer, yinYangCenterX + x, yinYangCenterY + y);
			}
		}
	}

	// 绘制八卦装饰点（四象）- 水墨风格
	SDL_SetRenderDrawColor(renderer, 80, 70, 60, 150);
	int dotRadius = 3;
	SDL_Rect dots[4] = {
		{yinYangCenterX - yinYangRadius - 15, yinYangCenterY, dotRadius*2, dotRadius*2}, // 左
		{yinYangCenterX + yinYangRadius + 15, yinYangCenterY, dotRadius*2, dotRadius*2}, // 右
		{yinYangCenterX, yinYangCenterY - yinYangRadius - 15, dotRadius*2, dotRadius*2}, // 上
		{yinYangCenterX, yinYangCenterY + yinYangRadius + 15, dotRadius*2, dotRadius*2}  // 下
	};

	for (const auto& dot : dots) {
		// 绘制圆点
		int centerX = dot.x + dot.w / 2;
		int centerY = dot.y + dot.h / 2;
		for (int dy = -dotRadius; dy <= dotRadius; ++dy) {
			for (int dx = -dotRadius; dx <= dotRadius; ++dx) {
				if (dx*dx + dy*dy <= dotRadius*dotRadius) {
					SDL_RenderDrawPoint(renderer, centerX + dx, centerY + dy);
				}
			}
		}
	}

	// 绘制卡牌槽
	for (int i = 0; i < TOTAL_SLOTS; ++i) {
		const auto& slot = cardSlots_[i];

		// 卡牌槽背景
		SDL_SetRenderDrawColor(renderer,
			slotColor_.r, slotColor_.g, slotColor_.b, slotColor_.a);
		SDL_RenderFillRect(renderer, &slot);

		// 卡牌槽边框
		SDL_SetRenderDrawColor(renderer,
			slotBorderColor_.r, slotBorderColor_.g, slotBorderColor_.b, slotBorderColor_.a);
		SDL_RenderDrawRect(renderer, &slot);

		// 卡牌槽内部装饰（十字线）
		int centerX = slot.x + slot.w / 2;
		int centerY = slot.y + slot.h / 2;
		SDL_SetRenderDrawColor(renderer, slotBorderColor_.r, slotBorderColor_.g, slotBorderColor_.b, 100);
		SDL_RenderDrawLine(renderer, slot.x + 10, centerY, slot.x + slot.w - 10, centerY);
		SDL_RenderDrawLine(renderer, centerX, slot.y + 10, centerX, slot.y + slot.h - 10);

		// 在卡牌槽角落添加小装饰点
		SDL_SetRenderDrawColor(renderer, 150, 180, 220, 120);
		SDL_Rect cornerDots[4] = {
			{slot.x + 3, slot.y + 3, 2, 2},           // 左上
			{slot.x + slot.w - 5, slot.y + 3, 2, 2},   // 右上
			{slot.x + 3, slot.y + slot.h - 5, 2, 2},   // 左下
			{slot.x + slot.w - 5, slot.y + slot.h - 5, 2, 2} // 右下
		};
		for (const auto& dot : cornerDots) {
			SDL_RenderFillRect(renderer, &dot);
		}
	}

	// 绘制卡牌槽分界线 - 在第二行和第三行之间
	{
		// 获取第二行最后一个卡牌槽的位置，作为分界线参考
		const auto& lastSecondRowSlot = cardSlots_[7]; // 第二行第四个 (0-7是前两行)
		int separatorY = lastSecondRowSlot.y + lastSecondRowSlot.h + 15; // 在第二行下面15像素

		// 计算分界线左右边界
		const auto& firstSlot = cardSlots_[0]; // 第一行第一个
		const auto& lastSlot = cardSlots_[BOARD_COLS - 1]; // 第一行最后一个
		int separatorLeft = firstSlot.x - 10;
		int separatorRight = lastSlot.x + lastSlot.w + 10;
		int separatorWidth = separatorRight - separatorLeft;

		// 绘制分界线背景
		SDL_SetRenderDrawColor(renderer, 40, 35, 30, 150); // 深墨色背景
		SDL_Rect separatorBg = {separatorLeft, separatorY - 2, separatorWidth, 4};
		SDL_RenderFillRect(renderer, &separatorBg);

		// 绘制分界线装饰点
		SDL_SetRenderDrawColor(renderer, 80, 70, 60, 200);
		for (int i = 0; i < separatorWidth; i += 8) {
			SDL_RenderDrawPoint(renderer, separatorLeft + i, separatorY);
		}
	}

	// 绘制水墨风格的传统铃铛 - 真实形状
	int bellCenterX = bellRect_.x + bellRect_.w / 2;
	int bellTopY = bellRect_.y + bellRect_.h / 4;
	int bellBottomY = bellRect_.y + bellRect_.h - 10;

	// 铃铛上部帽沿 - 弧形
	int capRadius = bellRect_.w / 3;
	for (int y = bellTopY - capRadius; y <= bellTopY; ++y) {
		int relY = y - (bellTopY - capRadius);
		int width = static_cast<int>(sqrt(capRadius * capRadius - relY * relY));
		for (int x = -width; x <= width; x += 1) {
			int px = bellCenterX + x;
			int py = y;
			if (px >= bellRect_.x && px <= bellRect_.x + bellRect_.w &&
				py >= bellRect_.y && py <= bellRect_.y + bellRect_.h) {
				SDL_SetRenderDrawColor(renderer, bellColor_.r, bellColor_.g, bellColor_.b, bellColor_.a);
				SDL_RenderDrawPoint(renderer, px, py);
			}
		}
	}

	// 铃铛主体 - 椭圆形渐变
	int bodyWidth = bellRect_.w / 2;
	int bodyHeight = bellRect_.h / 3;
	for (int y = bellTopY; y <= bellBottomY; ++y) {
		float bodyY = static_cast<float>(y - bellTopY) / bodyHeight;
		int width = static_cast<int>(bodyWidth * (1.0f - bodyY * bodyY * 0.3f));
		for (int x = -width; x <= width; x += 1) {
			int px = bellCenterX + x;
			int py = y;
			if (px >= bellRect_.x && px <= bellRect_.x + bellRect_.w &&
				py >= bellRect_.y && py <= bellRect_.y + bellRect_.h) {
				SDL_SetRenderDrawColor(renderer, bellColor_.r, bellColor_.g, bellColor_.b, bellColor_.a);
				SDL_RenderDrawPoint(renderer, px, py);
			}
		}
	}

	// 铃铛垂饰 - 小球
	int ballCenterY = bellBottomY + 8;
	int ballRadius = bellRect_.w / 6;
	for (int y = -ballRadius; y <= ballRadius; ++y) {
		int width = static_cast<int>(sqrt(ballRadius * ballRadius - y * y));
		for (int x = -width; x <= width; x += 1) {
			int px = bellCenterX + x;
			int py = ballCenterY + y;
			if (px >= bellRect_.x && px <= bellRect_.x + bellRect_.w &&
				py >= bellRect_.y && py <= bellRect_.y + bellRect_.h) {
				SDL_SetRenderDrawColor(renderer, bellColor_.r, bellColor_.g, bellColor_.b, bellColor_.a);
				SDL_RenderDrawPoint(renderer, px, py);
			}
		}
	}

	// 铃铛内部锤子 - 短线
	int hammerY = bellTopY + bellRect_.h / 4;
	int hammerLength = bellRect_.w / 4;
	SDL_SetRenderDrawColor(renderer, bellColor_.r + 20, bellColor_.g + 20, bellColor_.b + 20, bellColor_.a);
	SDL_RenderDrawLine(renderer, bellCenterX, hammerY, bellCenterX, hammerY + hammerLength);
	// 锤头
	int hammerHeadY = hammerY + hammerLength;
	int hammerHeadRadius = 2;
	for (int y = -hammerHeadRadius; y <= hammerHeadRadius; ++y) {
		int width = static_cast<int>(sqrt(hammerHeadRadius * hammerHeadRadius - y * y));
		for (int x = -width; x <= width; x += 1) {
			int px = bellCenterX + x;
			int py = hammerHeadY + y;
			if (px >= bellRect_.x && px <= bellRect_.x + bellRect_.w &&
				py >= bellRect_.y && py <= bellRect_.y + bellRect_.h) {
				SDL_SetRenderDrawColor(renderer, bellColor_.r + 20, bellColor_.g + 20, bellColor_.b + 20, bellColor_.a);
				SDL_RenderDrawPoint(renderer, px, py);
			}
		}
	}

	// 绘制手牌区
	for (size_t i = 0; i < handCardRects_.size(); ++i) {
		const auto& cardRect = handCardRects_[i];

		// 手牌背景
		SDL_SetRenderDrawColor(renderer,
			handCardColor_.r, handCardColor_.g, handCardColor_.b, handCardColor_.a);
		SDL_RenderFillRect(renderer, &cardRect);

		// 手牌边框
		SDL_SetRenderDrawColor(renderer,
			handCardColor_.r + 40, handCardColor_.g + 40, handCardColor_.b + 40, 255);
		SDL_RenderDrawRect(renderer, &cardRect);

		// 手牌内部装饰
		int cardCenterX = cardRect.x + cardRect.w / 2;
		int cardCenterY = cardRect.y + cardRect.h / 2;

		// 中央十字装饰
		SDL_SetRenderDrawColor(renderer, handCardColor_.r + 20, handCardColor_.g + 20, handCardColor_.b + 20, 80);
		SDL_RenderDrawLine(renderer, cardRect.x + 8, cardCenterY, cardRect.x + cardRect.w - 8, cardCenterY);
		SDL_RenderDrawLine(renderer, cardCenterX, cardRect.y + 8, cardCenterX, cardRect.y + cardRect.h - 8);

		// 角落装饰
		SDL_SetRenderDrawColor(renderer, 120, 150, 180, 100);
		SDL_Rect cornerDecor[4] = {
			{cardRect.x + 5, cardRect.y + 5, 3, 3},           // 左上
			{cardRect.x + cardRect.w - 8, cardRect.y + 5, 3, 3},   // 右上
			{cardRect.x + 5, cardRect.y + cardRect.h - 8, 3, 3},   // 左下
			{cardRect.x + cardRect.w - 8, cardRect.y + cardRect.h - 8, 3, 3} // 右下
		};
		for (const auto& decor : cornerDecor) {
			SDL_RenderFillRect(renderer, &decor);
		}
	}

	// 绘制右侧牌堆
	// 抽牌堆
	{
		const auto& pile = drawPileRect_;

		// 牌堆背景
		SDL_SetRenderDrawColor(renderer, 180, 170, 160, 160);
		SDL_RenderFillRect(renderer, &pile);

		// 牌堆边框
		SDL_SetRenderDrawColor(renderer, 80, 70, 60, 200);
		SDL_RenderDrawRect(renderer, &pile);

		// 牌堆装饰 - 卡牌背面图案
		int centerX = pile.x + pile.w / 2;
		int centerY = pile.y + pile.h / 2;

		// 中央装饰圆
		SDL_SetRenderDrawColor(renderer, 100, 90, 80, 120);
		int decoRadius = pile.w / 4;
		for (int y = -decoRadius; y <= decoRadius; ++y) {
			int width = static_cast<int>(sqrt(decoRadius * decoRadius - y * y));
			for (int x = -width; x <= width; ++x) {
				int px = centerX + x;
				int py = centerY + y;
				if (px >= pile.x && px <= pile.x + pile.w &&
					py >= pile.y && py <= pile.y + pile.h) {
					SDL_RenderDrawPoint(renderer, px, py);
				}
			}
		}

		// 牌堆文字装饰 - 水平线条
		SDL_SetRenderDrawColor(renderer, 60, 50, 40, 180);
		for (int i = 0; i < 4; ++i) {
			int lineY = pile.y + pile.h / 5 + i * pile.h / 5;
			SDL_RenderDrawLine(renderer, pile.x + 8, lineY, pile.x + pile.w - 8, lineY);
		}

		// 添加中央十字装饰
		SDL_SetRenderDrawColor(renderer, 80, 70, 60, 100);
		SDL_RenderDrawLine(renderer, centerX, pile.y + 8, centerX, pile.y + pile.h - 8);
		SDL_RenderDrawLine(renderer, pile.x + 8, centerY, pile.x + pile.w - 8, centerY);
	}

	// 弃牌堆
	{
		const auto& pile = discardPileRect_;

		// 牌堆背景
		SDL_SetRenderDrawColor(renderer, 170, 160, 150, 160);
		SDL_RenderFillRect(renderer, &pile);

		// 牌堆边框
		SDL_SetRenderDrawColor(renderer, 70, 60, 50, 200);
		SDL_RenderDrawRect(renderer, &pile);

		// 弃牌堆装饰 - 斜线交叉图案
		SDL_SetRenderDrawColor(renderer, 90, 80, 70, 120);
		for (int i = 0; i < 5; ++i) {
			int offset = i * 5;
			// 左上到右下
			SDL_RenderDrawLine(renderer,
				pile.x + offset, pile.y + offset,
				pile.x + pile.w - offset, pile.y + pile.h - offset);
			// 右上到左下
			if (offset < pile.w / 2) {
				SDL_RenderDrawLine(renderer,
					pile.x + pile.w - offset, pile.y + offset,
					pile.x + offset, pile.y + pile.h - offset);
			}
		}

		// 弃牌堆文字装饰 - 水平线条
		SDL_SetRenderDrawColor(renderer, 60, 50, 40, 180);
		for (int i = 0; i < 3; ++i) {
			int lineY = pile.y + pile.h / 4 + i * pile.h / 4;
			SDL_RenderDrawLine(renderer, pile.x + 8, lineY, pile.x + pile.w - 8, lineY);
		}

		// 添加角落装饰点
		SDL_SetRenderDrawColor(renderer, 110, 100, 90, 140);
		SDL_Rect cornerDots[4] = {
			{pile.x + 3, pile.y + 3, 2, 2},           // 左上
			{pile.x + pile.w - 5, pile.y + 3, 2, 2},   // 右上
			{pile.x + 3, pile.y + pile.h - 5, 2, 2},   // 左下
			{pile.x + pile.w - 5, pile.y + pile.h - 5, 2, 2} // 右下
		};
		for (const auto& dot : cornerDots) {
			SDL_RenderFillRect(renderer, &dot);
		}
	}

	// 水墨风格的淡雅分隔线
	SDL_SetRenderDrawColor(renderer, 120, 110, 100, 100);

	// 顶部区域分隔线（阴阳鱼和卡牌区之间）
	int topSeparatorY = yinYangPoolRect_.y + yinYangPoolRect_.h + 30;
	// 绘制虚线效果
	for (int x = yinYangPoolRect_.x + yinYangPoolRect_.w + 40; x < screenW_ - 40; x += 6) {
		SDL_RenderDrawLine(renderer, x, topSeparatorY, x + 3, topSeparatorY);
	}

	// 底部区域分隔线（手牌区上方）
	int bottomSeparatorY = screenH_ - 120;
	for (int x = 50; x < screenW_ - 50; x += 6) {
		SDL_RenderDrawLine(renderer, x, bottomSeparatorY, x + 3, bottomSeparatorY);
	}

	// 水墨风格的标题文字 - 淡雅墨色
	SDL_SetRenderDrawColor(renderer, 80, 70, 60, 180);
	const char* battleText = "水墨对弈";
	int textWidth = strlen(battleText) * 12; // 中文字体宽度
	int textX = yinYangPoolRect_.x + yinYangPoolRect_.w + 50;
	int textY = yinYangPoolRect_.y + 20;

	// 简单的文字轮廓（用点表示文字）
	for (size_t i = 0; i < strlen(battleText); ++i) {
		int charX = textX + i * 14;
		// 为每个字符绘制简单的像素文字
		for (int dy = 0; dy < 12; ++dy) {
			for (int dx = 0; dx < 10; ++dx) {
				if (rand() % 4 == 0) { // 随机墨点营造水墨效果
					SDL_RenderDrawPoint(renderer, charX + dx, textY + dy);
				}
			}
		}
	}

	// 渲染返回测试按钮（简单矩形按钮，无字体）
	if (backToTestButton_) {
		backToTestButton_->render(renderer);
	}
}
