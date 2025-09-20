#include "BattleState.h"
#include "TestState.h"
#include "../core/App.h"

#include "../core/Cards.h"
#include <SDL.h>

#include <SDL_ttf.h>
#include <cmath>

#include <algorithm>
#include <sstream>

BattleState::BattleState() = default;


BattleState::~BattleState() {
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (cardNameFont_) TTF_CloseFont(cardNameFont_);
	if (cardStatFont_) TTF_CloseFont(cardStatFont_);
	if (infoFont_) TTF_CloseFont(infoFont_);
	if (backButton_) delete backButton_;
}

void BattleState::onEnter(App& app) {

	// 设置窗口尺寸（适中尺寸）
	screenW_ = 1600;
	screenH_ = 1000;
	SDL_SetWindowSize(app.getWindow(), screenW_, screenH_);

	// 加载字体
	titleFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 32);
	cardNameFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 16);
	cardStatFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 14);
	infoFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 18);

	// 初始化战斗
	initializeBattle();
	
	// 布局UI
	layoutUI();
	
	// 创建按钮
	backButton_ = new Button();
	if (backButton_) {
		backButton_->setRect({20, 20, 100, 35});
		backButton_->setText("返回");
		if (infoFont_) backButton_->setFont(infoFont_, app.getRenderer());
		backButton_->setOnClick([this]() {
			pendingBackToTest_ = true;
		});
	}
	
	// 移除结束回合按钮，功能移到砚台上
	
	// 移除抽牌按钮，改为检测牌堆点击
}

void BattleState::onExit(App& app) {
	// 清理资源
}

void BattleState::handleEvent(App& app, const SDL_Event& e) {
	// 处理按钮事件
	if (backButton_) backButton_->handleEvent(e);
	
	// 处理鼠标移动事件（悬停检测）
	if (e.type == SDL_MOUSEMOTION) {
		int mouseX = e.motion.x;
		int mouseY = e.motion.y;
		
		// 重置悬停状态
		hoveredBattlefieldIndex_ = -1;
		hoveredHandCardIndex_ = -1;
		
		// 检查战场悬停
		for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
			const auto& rect = battlefield_[i].rect;
			if (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
				mouseY >= rect.y && mouseY <= rect.y + rect.h) {
				hoveredBattlefieldIndex_ = i;
				break;
			}
		}
		
		// 检查手牌悬停
		for (size_t i = 0; i < handCardRects_.size(); ++i) {
			const auto& rect = handCardRects_[i];
			if (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
				mouseY >= rect.y && mouseY <= rect.y + rect.h) {
				hoveredHandCardIndex_ = static_cast<int>(i);
				break;
			}
		}
	}
	
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mouseX = e.button.x;
		int mouseY = e.button.y;
		

		// 检查牌堆点击
		if (mouseX >= inkPileRect_.x && mouseX <= inkPileRect_.x + inkPileRect_.w &&
			mouseY >= inkPileRect_.y && mouseY <= inkPileRect_.y + inkPileRect_.h) {
			// 点击墨锭牌堆
			if (inkPileCount_ > 0) {
				Card inkCard = CardDB::instance().make("moding");
				if (!inkCard.id.empty()) {
					handCards_.push_back(inkCard);
					inkPileCount_--;
					layoutHandCards();
					statusMessage_ = "从墨锭牌堆抽到：" + inkCard.name;
				}
			} else {
				statusMessage_ = "墨锭牌堆已空！";
			}
		} else if (mouseX >= playerPileRect_.x && mouseX <= playerPileRect_.x + playerPileRect_.w &&
				   mouseY >= playerPileRect_.y && mouseY <= playerPileRect_.y + playerPileRect_.h) {
			// 点击玩家牌堆
			if (playerPileCount_ > 0) {
				auto allIds = CardDB::instance().allIds();
				std::vector<std::string> playerCardIds;
				for (const auto& id : allIds) {
					if (id != "moding") { // 排除墨锭
						playerCardIds.push_back(id);
					}
				}
				
				if (!playerCardIds.empty()) {
					int randomIndex = rand() % playerCardIds.size();
					Card newCard = CardDB::instance().make(playerCardIds[randomIndex]);
					if (!newCard.id.empty()) {
						handCards_.push_back(newCard);
						playerPileCount_--;
						layoutHandCards();
						statusMessage_ = "从玩家牌堆抽到：" + newCard.name;
					}
				}
			} else {
				statusMessage_ = "玩家牌堆已空！";
			}
		} else if (mouseX >= inkStoneRect_.x && mouseX <= inkStoneRect_.x + inkStoneRect_.w &&
				   mouseY >= inkStoneRect_.y && mouseY <= inkStoneRect_.y + inkStoneRect_.h) {
			// 点击砚台（结束回合）
			if (currentPhase_ == GamePhase::PlayerTurn) {
				endTurn();
			}
		}
		
		// 检查手牌点击
		for (size_t i = 0; i < handCardRects_.size(); ++i) {
			const auto& rect = handCardRects_[i];
			if (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
				mouseY >= rect.y && mouseY <= rect.y + rect.h) {
				selectedHandCard_ = static_cast<int>(i);
				
				// 检查献墨点数
				if (handCards_[i].sacrificeCost > 0) {
					// 需要献墨，进入献祭模式
					isSacrificing_ = true;
					sacrificeTargetCost_ = handCards_[i].sacrificeCost;
					currentSacrificeInk_ = 0;
					sacrificeCandidates_.clear();
					
					// 重置所有卡牌的献祭状态
					for (int j = 0; j < TOTAL_BATTLEFIELD_SLOTS; ++j) {
						battlefield_[j].isSacrificed = false;
						if (battlefield_[j].isAlive && battlefield_[j].isPlayer) {
							sacrificeCandidates_.push_back(j);
						}
					}
					
					statusMessage_ = "需要献祭 " + std::to_string(sacrificeTargetCost_) + " 点墨量，点击场上卡牌献祭";
				} else {
					// 0献墨点数，直接可以打出
					isSacrificing_ = false;
					statusMessage_ = "已选择手牌：" + handCards_[i].name + "（0献墨点数，可直接打出）";
				}
				break;
			}
		}
		
		// 检查战场点击
		if (currentPhase_ == GamePhase::PlayerTurn) {
			for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
				const auto& rect = battlefield_[i].rect;
				if (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
					mouseY >= rect.y && mouseY <= rect.y + rect.h) {
					
					
					// 计算行号（0=第一行，1=第二行，2=第三行）
					int row = i / BATTLEFIELD_COLS;
					
					if (isSacrificing_) {
						// Sacrifice mode: click battlefield cards to sacrifice
						
						if (battlefield_[i].isAlive && battlefield_[i].isPlayer && !battlefield_[i].isSacrificed) {
							// 献祭这张卡牌（点击就加点数，出现墨滴，标记为已献祭）
							currentSacrificeInk_++;
							currentInk_ += battlefield_[i].card.sacrificeCost; // 立即增加总墨量
							battlefield_[i].isSacrificed = true; // 标记为已献祭
							statusMessage_ = "献祭获得 " + std::to_string(battlefield_[i].card.sacrificeCost) + " 点墨量，当前：" + std::to_string(currentSacrificeInk_) + "/" + std::to_string(sacrificeTargetCost_) + " (总墨量:" + std::to_string(currentInk_) + ")";
							
							// 检查是否献祭足够
							if (currentSacrificeInk_ >= sacrificeTargetCost_) {
								// 献祭完成，开始动画
								isSacrificing_ = false;
								isSacrificeAnimating_ = true;
								showSacrificeInk_ = true;  // 开始显示献墨量
								sacrificeAnimTime_ = 0.0f;
								
								// 收集待摧毁的卡牌
								cardsToDestroy_.clear();
								for (int j = 0; j < TOTAL_BATTLEFIELD_SLOTS; ++j) {
									if (battlefield_[j].isSacrificed) {
										cardsToDestroy_.push_back(j);
									}
								}
								
								statusMessage_ = "献祭完成！卡牌正在消失...";
							}
						} else if (battlefield_[i].isSacrificed) {
							statusMessage_ = "这张卡牌已经被献祭了";
						} else {
							statusMessage_ = "只能献祭己方卡牌";
						}
					} else if (selectedHandCard_ >= 0) {
						// 正常模式：放置卡牌
						if (row == 2) {
							// 检查位置是否为空
							if (!battlefield_[i].isAlive) {
								// 检查是否需要献祭
								if (handCards_[selectedHandCard_].sacrificeCost > 0 && isSacrificing_) {
									statusMessage_ = "需要先献祭才能打出此卡牌";
								} else {
									// 0献墨点数或献祭已完成，直接打出
									playCard(selectedHandCard_, i);
									selectedHandCard_ = -1;
									statusMessage_ = "卡牌已放置在第三行";
								}
							} else {
								statusMessage_ = "该位置已有卡牌";
							}
						} else {
							statusMessage_ = "只能将卡牌放在最下面一行";
						}
					}
					break;
				}
			}
		}
		
	}
	
	// 右键点击检测（墨锭献祭和取消献祭）
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
		int mouseX = e.button.x;
		int mouseY = e.button.y;


		// 检查是否在献祭模式，右键取消献祭
		if (isSacrificing_) {
			// 取消献祭模式
			isSacrificing_ = false;
			currentSacrificeInk_ = 0;
			sacrificeCandidates_.clear();
			selectedHandCard_ = -1;
			statusMessage_ = "已取消献祭";
			return;
		}

		// 检查手牌右键点击（墨锭献祭）
		for (size_t i = 0; i < handCardRects_.size(); ++i) {

			const auto& rect = handCardRects_[i];
			if (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
				mouseY >= rect.y && mouseY <= rect.y + rect.h) {
				Card& card = handCards_[i];
				if (card.id == "moding" && currentPhase_ == GamePhase::PlayerTurn) {
					// 献祭墨锭，获得墨量
					currentInk_ += 3; // 献祭墨锭获得3点墨量
					if (currentInk_ > maxInk_) currentInk_ = maxInk_;
					
					// 从手牌移除
					handCards_.erase(handCards_.begin() + i);
					layoutHandCards();
					selectedHandCard_ = -1;
					
					statusMessage_ = "献祭墨锭，获得3点墨量！";
				}
				break;
			}
		}
	}
}

void BattleState::update(App& app, float dt) {

	// 处理状态转换
	if (pendingBackToTest_) {
		pendingBackToTest_ = false;
		app.setState(std::unique_ptr<State>(static_cast<State*>(new TestState())));
		return;
	}
	
	// 更新献祭动画
	if (isSacrificeAnimating_) {
		sacrificeAnimTime_ += dt;
		
		// 动画完成，摧毁卡牌
		if (sacrificeAnimTime_ >= sacrificeAnimDuration_) {
			for (int cardIndex : cardsToDestroy_) {
				battlefield_[cardIndex].isAlive = false;
			}
			isSacrificeAnimating_ = false;
			cardsToDestroy_.clear();
			statusMessage_ = "献祭完成！可以打出卡牌";
		}
	}

	// 更新手牌动画
	for (size_t i = 0; i < handCardScales_.size(); ++i) {
		float targetScale = 1.0f;
		
		// 悬停时放大
		if (i == hoveredHandCardIndex_) {
			targetScale = hoverScale_;
		}
		
		// 点击时进一步放大
		if (i == selectedHandCard_) {
			targetScale = clickScale_;
		}
		
		// 插值动画
		float diff = targetScale - handCardScales_[i];
		handCardScales_[i] += diff * animationSpeed_ * dt;
		
		// 确保缩放值在合理范围内
		handCardScales_[i] = std::max(0.5f, std::min(2.0f, handCardScales_[i]));
		
		// 更新漂浮动画（只有选中的卡牌才漂浮）
		if (i < handCardFloatTime_.size()) {
			if (i == selectedHandCard_) {
				// 只有选中的卡牌才进行漂浮动画
				handCardFloatTime_[i] += dt * floatSpeed_;
				handCardFloatY_[i] = sinf(handCardFloatTime_[i]) * floatAmplitude_;
			} else {
				// 非选中的卡牌保持静止
				handCardFloatY_[i] = 0.0f;
			}
		}
	}

	// 检查游戏结束
	checkGameOver();
}

void BattleState::render(App& app) {
	SDL_Renderer* renderer = app.getRenderer();


	// 绘制战斗背景
	SDL_SetRenderDrawColor(renderer, 20, 24, 34, 255);
	SDL_Rect bgRect = {0, 0, screenW_, screenH_};
	SDL_RenderFillRect(renderer, &bgRect);


	// 渲染各个区域
	renderBattlefield(app);
	renderHandCards(app);
	renderUI(app);
}

// 私有方法实现
void BattleState::initializeBattle() {
	// 初始化游戏状态
	currentPhase_ = GamePhase::PlayerTurn;
	currentTurn_ = 1;
	playerHealth_ = 20;
	enemyHealth_ = 20;
	currentInk_ = 0;
	maxInk_ = 10;
	selectedHandCard_ = -1;
	statusMessage_ = "战斗开始！";
	
	// 清空战场
	for (auto& card : battlefield_) {
		card.isAlive = false;
		card.health = 0;
	}
	
	// 开局抽牌：1张墨锭 + 3张玩家牌
	handCards_.clear();
	
	// 抽1张墨锭（必须是墨锭）
	Card inkCard = CardDB::instance().make("moding");
	if (!inkCard.id.empty()) {
		handCards_.push_back(inkCard);
	}
	
	// 抽3张玩家牌（从其他卡牌中随机选择）
	auto allIds = CardDB::instance().allIds();
	std::vector<std::string> playerCardIds;
	for (const auto& id : allIds) {
		if (id != "墨锭") { // 排除墨锭
			playerCardIds.push_back(id);
		}
	}
	
	// 随机选择3张玩家牌
	for (int i = 0; i < 3 && i < static_cast<int>(playerCardIds.size()); ++i) {
		int randomIndex = rand() % playerCardIds.size();
		Card card = CardDB::instance().make(playerCardIds[randomIndex]);
		if (!card.id.empty()) {
			handCards_.push_back(card);
		}
		// 移除已选择的卡牌，避免重复
		playerCardIds.erase(playerCardIds.begin() + randomIndex);
	}
	
	// 初始化牌堆
	inkPileCount_ = 10;  // 墨锭牌堆只有10张
	playerPileCount_ = 20;
}

void BattleState::layoutUI() {
	// 敌人区域（上方，增大上下长度）
	enemyAreaRect_ = {
		0,
		0,
		screenW_,
		200
	};
	
	// 战场区域（中央）- 3x4网格，向上移动
	int battlefieldWidth = 1000;
	int battlefieldHeight = 500;
	battlefieldRect_ = {
		(screenW_ - battlefieldWidth) / 2,
		screenH_ - 250 - battlefieldHeight - 20,  // 向上移动更多
		battlefieldWidth,
		battlefieldHeight
	};
	
	// 手牌区域（底部，和窗口一样长，恢复原来高度）
	handAreaRect_ = {
		0,
		screenH_ - 250,
		screenW_,
		250
	};
	
	// 墨尺（左侧，横着放，向上移动，长度变大）
	inkRulerRect_ = {
		50,
		screenH_ / 2 - 250,
		400,
		80
	};
	
	// 砚台（左侧，横着放，在墨尺下面，上下长度变大）
	inkStoneRect_ = {
		50,
		screenH_ / 2 - 150,
		400,
		120
	};
	
	// 卡牌介绍区域（右侧，介于牌堆和敌人区域之间，左右长度变大）
	cardInfoRect_ = {
		screenW_ - 400,
		250,
		350,
		200
	};
	
	// 道具区域（左侧，介于砚台和手牌区之间，向上移动，和砚台长度一样）
	itemAreaRect_ = {
		50,
		screenH_ - 500,
		400,
		200
	};
	
	// 墨锭牌堆（右侧，在卡牌介绍和手牌区之间，再向上移动，变大）
	inkPileRect_ = {
		screenW_ - 420,
		screenH_ - 450,
		180,
		140
	};
	
	// 玩家牌堆（右侧，在卡牌介绍和手牌区之间，再向上移动，变大）
	playerPileRect_ = {
		screenW_ - 230,
		screenH_ - 450,
		180,
		140
	};
	
	layoutBattlefield();
	layoutHandCards();
}

void BattleState::layoutBattlefield() {
	// 战场布局：3行4列，所有卡牌尺寸相同
	int cardWidth = 120;
	int cardHeight = 160;  // 增加卡牌高度
	int spacingX = 12;
	int spacingY = 12;
	
	int startX = battlefieldRect_.x + (battlefieldRect_.w - (BATTLEFIELD_COLS * cardWidth + (BATTLEFIELD_COLS - 1) * spacingX)) / 2;
	int startY = battlefieldRect_.y + (battlefieldRect_.h - (BATTLEFIELD_ROWS * cardHeight + (BATTLEFIELD_ROWS - 1) * spacingY)) / 2;
	
	for (int row = 0; row < BATTLEFIELD_ROWS; ++row) {
		for (int col = 0; col < BATTLEFIELD_COLS; ++col) {
			int index = row * BATTLEFIELD_COLS + col;
			battlefield_[index].rect = {
				startX + col * (cardWidth + spacingX),
				startY + row * (cardHeight + spacingY),
				cardWidth,
				cardHeight
			};
			battlefield_[index].isPlayer = true; // 所有位置都可以放置玩家卡牌
		}
	}
}

void BattleState::layoutHandCards() {
	handCardRects_.clear();
	if (handCards_.empty()) return;
	
	int cardWidth = 130;  // 从120增加到130
	int cardHeight = 170; // 从160增加到170
	int spacing = 8;      // 减少间距以适应更大的手牌
	int totalWidth = static_cast<int>(handCards_.size()) * cardWidth + (static_cast<int>(handCards_.size()) - 1) * spacing;
	int startX = handAreaRect_.x + (handAreaRect_.w - totalWidth) / 2;
	int startY = handAreaRect_.y + (handAreaRect_.h - cardHeight) / 2;
	
	// 初始化动画数据
	handCardScales_.resize(handCards_.size(), 1.0f);
	handCardAnimTime_.resize(handCards_.size(), 0.0f);
	handCardFloatY_.resize(handCards_.size(), 0.0f);
	handCardFloatTime_.resize(handCards_.size(), 0.0f);
	
	for (size_t i = 0; i < handCards_.size(); ++i) {
		handCardRects_.push_back({
			startX + static_cast<int>(i) * (cardWidth + spacing),
			startY,
			cardWidth,
			cardHeight
		});
	}
}

// drawCard函数已移除，现在通过点击牌堆来抽牌

void BattleState::playCard(int handIndex, int battlefieldIndex) {
	if (handIndex < 0 || handIndex >= static_cast<int>(handCards_.size())) return;
	if (battlefieldIndex < 0 || battlefieldIndex >= TOTAL_BATTLEFIELD_SLOTS) return;
	if (currentPhase_ != GamePhase::PlayerTurn) return;
	
	// 检查行号（只能放在最下面一行）
	int row = battlefieldIndex / BATTLEFIELD_COLS;
	if (row != 2) {
		statusMessage_ = "只能将卡牌放在最下面一行！";
		return;
	}
	
	Card& card = handCards_[handIndex];
	
	// 检查献墨点数
	if (card.sacrificeCost > 0) {
		// 需要献墨，检查是否已经完成献祭
		if (isSacrificing_) {
			statusMessage_ = "请先完成献祭！";
			return;
		}
		
		// 检查献祭是否足够（献祭完成后，currentSacrificeInk_应该等于sacrificeTargetCost_）
		if (currentSacrificeInk_ < sacrificeTargetCost_) {
			statusMessage_ = "献祭不足！需要 " + std::to_string(sacrificeTargetCost_) + " 点墨量";
			return;
		}
	}
	
	// 检查战场位置（所有位置都可以放置）
	if (battlefield_[battlefieldIndex].isAlive) {
		statusMessage_ = "该位置已有卡牌！";
		return;
	}
	
	// 放置卡牌
	battlefield_[battlefieldIndex].card = card;
	battlefield_[battlefieldIndex].isAlive = true;
	battlefield_[battlefieldIndex].health = card.health;
	battlefield_[battlefieldIndex].isPlayer = true;
	
	// 消耗献祭获得的墨量
	if (card.sacrificeCost > 0) {
		// 消耗献祭获得的墨量（墨量已经在献祭过程中增加了）
		currentInk_ -= card.sacrificeCost;
		// 重置献祭状态
		isSacrificing_ = false;
		currentSacrificeInk_ = 0;
		sacrificeCandidates_.clear();
	}
	
	// 从手牌移除
	handCards_.erase(handCards_.begin() + handIndex);
	layoutHandCards();
	
	// 隐藏献墨量显示
	showSacrificeInk_ = false;
	
	statusMessage_ = "打出：" + card.name;
}

void BattleState::endTurn() {
	if (currentPhase_ != GamePhase::PlayerTurn) return;
	
	currentPhase_ = GamePhase::EnemyTurn;
	statusMessage_ = "敌人回合";
	
	// 恢复墨量（每回合开始时获得一些墨量）
	currentInk_ += 2; // 每回合获得2点墨量
	if (currentInk_ > maxInk_) currentInk_ = maxInk_;
	
	// 延迟执行敌人回合
	enemyTurn();
}

void BattleState::enemyTurn() {
	// 简单的AI：随机放置卡牌
	// 这里可以添加更复杂的AI逻辑
	
	// 敌人回合结束，回到玩家回合
	currentPhase_ = GamePhase::PlayerTurn;
	currentTurn_++;
	statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始";
}

void BattleState::checkGameOver() {
	if (playerHealth_ <= 0) {
		currentPhase_ = GamePhase::GameOver;
		statusMessage_ = "游戏失败！";
	} else if (enemyHealth_ <= 0) {
		currentPhase_ = GamePhase::GameOver;
		statusMessage_ = "胜利！";
	}
}

void BattleState::renderBattlefield(App& app) {
	SDL_Renderer* r = app.getRenderer();
	
	// 移除战场背景框
	
	// 绘制战场卡牌
	for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
		const auto& bfCard = battlefield_[i];
		
		// 检查是否悬停，改变颜色
		if (i == hoveredBattlefieldIndex_) {
			// 悬停时的高亮效果
			SDL_SetRenderDrawColor(r, 100, 150, 200, 100);
			SDL_RenderFillRect(r, &bfCard.rect);
			SDL_SetRenderDrawColor(r, 150, 200, 255, 255);
			SDL_RenderDrawRect(r, &bfCard.rect);
		}
		
		if (bfCard.isAlive) {
			// 检查是否正在播放摧毁动画
			bool isAnimating = false;
			float animProgress = 0.0f;
			if (isSacrificeAnimating_) {
				for (int cardIndex : cardsToDestroy_) {
					if (cardIndex == i) {
						isAnimating = true;
						animProgress = sacrificeAnimTime_ / sacrificeAnimDuration_;
						break;
					}
				}
			}
			
			// 计算动画效果
			SDL_Rect renderRect = bfCard.rect;
			
			if (isAnimating) {
				// 缩放动画（逐渐缩小）
				float scale = 1.0f - animProgress;
				scale = std::max(0.1f, scale);
				
				int newWidth = static_cast<int>(bfCard.rect.w * scale);
				int newHeight = static_cast<int>(bfCard.rect.h * scale);
				
				renderRect.x = bfCard.rect.x + (bfCard.rect.w - newWidth) / 2;
				renderRect.y = bfCard.rect.y + (bfCard.rect.h - newHeight) / 2;
				renderRect.w = newWidth;
				renderRect.h = newHeight;
			}
			
			// 使用CardRenderer渲染卡牌
			CardRenderer::renderCard(app, bfCard.card, renderRect, cardNameFont_, cardStatFont_, false);
			
					// 显示献祭符号（只有被点击献祭的卡牌才显示符号）
					if (isSacrificing_ && bfCard.isPlayer && bfCard.isSacrificed) {
						// 绘制献祭符号（红色X）
						SDL_SetRenderDrawColor(r, 255, 0, 0, 255);
						int centerX = bfCard.rect.x + bfCard.rect.w / 2;
						int centerY = bfCard.rect.y + bfCard.rect.h / 2;
						int symbolSize = 20;
						
						// 绘制X符号
						SDL_RenderDrawLine(r, centerX - symbolSize/2, centerY - symbolSize/2, centerX + symbolSize/2, centerY + symbolSize/2);
						SDL_RenderDrawLine(r, centerX + symbolSize/2, centerY - symbolSize/2, centerX - symbolSize/2, centerY + symbolSize/2);
						
						// 绘制外圈
						SDL_SetRenderDrawColor(r, 255, 100, 100, 200);
						SDL_Rect symbolRect{centerX - symbolSize/2 - 2, centerY - symbolSize/2 - 2, symbolSize + 4, symbolSize + 4};
						SDL_RenderDrawRect(r, &symbolRect);
					}
			
			// 显示当前生命值
			if (cardStatFont_) {
				SDL_Color healthCol{160, 30, 40, 255};
				std::string healthText = std::to_string(bfCard.health);
				SDL_Surface* s = TTF_RenderUTF8_Blended(cardStatFont_, healthText.c_str(), healthCol);
				if (s) {
					SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
					SDL_Rect dst{bfCard.rect.x + 5, bfCard.rect.y + 5, s->w, s->h};
					SDL_RenderCopy(r, t, nullptr, &dst);
					SDL_DestroyTexture(t);
					SDL_FreeSurface(s);
				}
			}
				} else {

			// 绘制空槽位
			SDL_SetRenderDrawColor(r, 60, 70, 80, 80);
			SDL_RenderFillRect(r, &bfCard.rect);
			SDL_SetRenderDrawColor(r, 100, 110, 120, 150);
			SDL_RenderDrawRect(r, &bfCard.rect);
		}
	}
	
	// 战斗牌位标注已删除
}

void BattleState::renderHandCards(App& app) {
	SDL_Renderer* r = app.getRenderer();
	
	// 移除手牌区域背景
	
	// 创建渲染顺序数组（选中的卡牌最后渲染，确保在最上层）
	std::vector<size_t> renderOrder;
	for (size_t i = 0; i < handCards_.size(); ++i) {
		if (i != selectedHandCard_) {
			renderOrder.push_back(i);
		}
	}
	// 选中的卡牌最后渲染
	if (selectedHandCard_ >= 0 && selectedHandCard_ < static_cast<int>(handCards_.size())) {
		renderOrder.push_back(selectedHandCard_);
	}
	
	// 按顺序绘制手牌
	for (size_t orderIndex = 0; orderIndex < renderOrder.size(); ++orderIndex) {
		size_t i = renderOrder[orderIndex];
		if (i >= handCards_.size() || i >= handCardRects_.size()) continue;
		
		const auto& card = handCards_[i];
		const auto& rect = handCardRects_[i];
		bool selected = (selectedHandCard_ == static_cast<int>(i));
		bool hovered = (hoveredHandCardIndex_ == static_cast<int>(i));
		
		// 计算缩放后的矩形
		float scale = (i < handCardScales_.size()) ? handCardScales_[i] : 1.0f;
		int scaledW = static_cast<int>(rect.w * scale);
		int scaledH = static_cast<int>(rect.h * scale);
		int offsetX = (rect.w - scaledW) / 2;
		int offsetY = (rect.h - scaledH) / 2;
		
		// 添加漂浮偏移
		float floatY = (i < handCardFloatY_.size()) ? handCardFloatY_[i] : 0.0f;
		
		SDL_Rect scaledRect = {
			rect.x + offsetX,
			rect.y + offsetY + static_cast<int>(floatY),
			scaledW,
			scaledH
		};
		
		// 悬停时的高亮效果（更明显的变色）
		if (hovered) {
			// 更亮的灰白色背景
			SDL_SetRenderDrawColor(r, 255, 255, 255, 200);
			SDL_RenderFillRect(r, &scaledRect);
			// 蓝色边框表示悬停
			SDL_SetRenderDrawColor(r, 100, 150, 255, 255);
			SDL_RenderDrawRect(r, &scaledRect);
			// 添加内边框增强效果
			SDL_SetRenderDrawColor(r, 150, 200, 255, 200);
			SDL_Rect innerRect = {scaledRect.x + 2, scaledRect.y + 2, scaledRect.w - 4, scaledRect.h - 4};
			SDL_RenderDrawRect(r, &innerRect);
		}
		
		// 使用CardRenderer渲染手牌
		CardRenderer::renderCard(app, card, scaledRect, cardNameFont_, cardStatFont_, selected);
	}
}


void BattleState::renderUI(App& app) {
	SDL_Renderer* r = app.getRenderer();
	
	// 绘制手牌区域背景（水墨风格，黑白灰配色）
	SDL_SetRenderDrawColor(r, 30, 30, 30, 200);
	SDL_RenderFillRect(r, &handAreaRect_);
	SDL_SetRenderDrawColor(r, 60, 60, 60, 255);
	SDL_RenderDrawRect(r, &handAreaRect_);
	
	// 绘制分割线（手牌区域上方，水墨风格）
	SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
	SDL_RenderDrawLine(r, 0, handAreaRect_.y, screenW_, handAreaRect_.y);
	
	// 绘制敌人区域（上方，水墨风格）
	SDL_SetRenderDrawColor(r, 20, 20, 20, 200);
	SDL_RenderFillRect(r, &enemyAreaRect_);
	SDL_SetRenderDrawColor(r, 50, 50, 50, 255);
	SDL_RenderDrawRect(r, &enemyAreaRect_);
	
	// 绘制墨尺（左侧，横着放，水墨风格）
	SDL_SetRenderDrawColor(r, 40, 40, 40, 200);
	SDL_RenderFillRect(r, &inkRulerRect_);
	SDL_SetRenderDrawColor(r, 80, 80, 80, 255);
	SDL_RenderDrawRect(r, &inkRulerRect_);
	
	// 绘制墨尺刻度（横着，水墨风格）
	SDL_SetRenderDrawColor(r, 150, 150, 150, 255);
	for (int i = 0; i <= maxInk_; ++i) {
		int x = inkRulerRect_.x + (inkRulerRect_.w * i / maxInk_);
		SDL_RenderDrawLine(r, x, inkRulerRect_.y + 20, x, inkRulerRect_.y + 40);
	}
	
	// 绘制刻度数字（5 4 3 2 1 0 1 2 3 4 5，水墨风格）
	if (infoFont_) {
		SDL_Color textColor{200, 200, 200, 255}; // 水墨风格文字颜色
		for (int i = 0; i <= maxInk_; ++i) {
			int x = inkRulerRect_.x + (inkRulerRect_.w * i / maxInk_);
			std::string numText;
			if (i <= maxInk_ / 2) {
				numText = std::to_string(maxInk_ / 2 - i);
			} else {
				numText = std::to_string(i - maxInk_ / 2);
			}
			SDL_Surface* s = TTF_RenderUTF8_Blended(infoFont_, numText.c_str(), textColor);
			if (s) {
				SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
				if (t) {
					SDL_Rect dst{x - s->w/2, inkRulerRect_.y + 45, s->w, s->h};
					SDL_RenderCopy(r, t, nullptr, &dst);
					SDL_DestroyTexture(t);
				}
				SDL_FreeSurface(s);
			}
		}
	}
	
	// 绘制当前墨量指示（横着，水墨风格）
	if (currentInk_ >= 0) {  // 改为 >= 0，即使墨量为0也显示
		int inkWidth = (inkRulerRect_.w * currentInk_ / maxInk_);
		SDL_SetRenderDrawColor(r, 10, 10, 10, 255); // 水墨风格墨量指示
		SDL_Rect inkBar{inkRulerRect_.x, inkRulerRect_.y + 60, inkWidth, 20};
		SDL_RenderFillRect(r, &inkBar);
	}
	
	// 绘制墨尺指针（在中间位置，水墨风格）
	int pointerX = inkRulerRect_.x + inkRulerRect_.w / 2;
	SDL_SetRenderDrawColor(r, 255, 255, 255, 255); // 水墨风格白色指针
	// 主指针线（更粗更长）
	SDL_RenderDrawLine(r, pointerX, inkRulerRect_.y + 50, pointerX, inkRulerRect_.y + 80);
	SDL_RenderDrawLine(r, pointerX - 1, inkRulerRect_.y + 50, pointerX - 1, inkRulerRect_.y + 80);
	SDL_RenderDrawLine(r, pointerX + 1, inkRulerRect_.y + 50, pointerX + 1, inkRulerRect_.y + 80);
	// 指针箭头（更大更明显）
	SDL_RenderDrawLine(r, pointerX - 5, inkRulerRect_.y + 60, pointerX + 5, inkRulerRect_.y + 60);
	SDL_RenderDrawLine(r, pointerX - 4, inkRulerRect_.y + 65, pointerX + 4, inkRulerRect_.y + 65);
	SDL_RenderDrawLine(r, pointerX - 3, inkRulerRect_.y + 70, pointerX + 3, inkRulerRect_.y + 70);
	
	// 绘制砚台（左侧，横着放，在墨尺下面）
	SDL_SetRenderDrawColor(r, 35, 35, 35, 200);
	SDL_RenderFillRect(r, &inkStoneRect_);
	SDL_SetRenderDrawColor(r, 70, 70, 70, 255);
	SDL_RenderDrawRect(r, &inkStoneRect_);
	
	// 绘制砚台内部墨池（横着，更大）
	SDL_SetRenderDrawColor(r, 5, 5, 5, 200);
	SDL_Rect inkPool{inkStoneRect_.x + 20, inkStoneRect_.y + 20, inkStoneRect_.w - 40, inkStoneRect_.h - 40};
	SDL_RenderFillRect(r, &inkPool);
	
	// 绘制卡牌介绍区域（左下角）
	SDL_SetRenderDrawColor(r, 25, 25, 25, 200);
	SDL_RenderFillRect(r, &cardInfoRect_);
	SDL_SetRenderDrawColor(r, 60, 60, 60, 255);
	SDL_RenderDrawRect(r, &cardInfoRect_);
	
	// 绘制道具区域（右侧）
	SDL_SetRenderDrawColor(r, 30, 30, 30, 200);
	SDL_RenderFillRect(r, &itemAreaRect_);
	SDL_SetRenderDrawColor(r, 65, 65, 65, 255);
	SDL_RenderDrawRect(r, &itemAreaRect_);
	
	// 绘制区域标签
	if (infoFont_) {
		SDL_Color textColor{220, 220, 220, 255}; // 水墨风格文字颜色
		
		// 敌人区域标签
		SDL_Surface* s1 = TTF_RenderUTF8_Blended(infoFont_, "敌人", textColor);
		if (s1) {
			SDL_Texture* t1 = SDL_CreateTextureFromSurface(r, s1);
			if (t1) {
				SDL_Rect dst1{enemyAreaRect_.x + 10, enemyAreaRect_.y + 10, s1->w, s1->h};
				SDL_RenderCopy(r, t1, nullptr, &dst1);
				SDL_DestroyTexture(t1);
			}
			SDL_FreeSurface(s1);
		}
		
		// 墨尺标签（居中）
		SDL_Surface* s2 = TTF_RenderUTF8_Blended(infoFont_, "墨尺", textColor);
		if (s2) {
			SDL_Texture* t2 = SDL_CreateTextureFromSurface(r, s2);
			if (t2) {
				SDL_Rect dst2{inkRulerRect_.x + (inkRulerRect_.w - s2->w) / 2, inkRulerRect_.y + 10, s2->w, s2->h};
				SDL_RenderCopy(r, t2, nullptr, &dst2);
				SDL_DestroyTexture(t2);
			}
			SDL_FreeSurface(s2);
		}
		
		// 砚台标签（居中）
		SDL_Surface* s3 = TTF_RenderUTF8_Blended(infoFont_, "砚台", textColor);
		if (s3) {
			SDL_Texture* t3 = SDL_CreateTextureFromSurface(r, s3);
			if (t3) {
				SDL_Rect dst3{inkStoneRect_.x + (inkStoneRect_.w - s3->w) / 2, inkStoneRect_.y + 10, s3->w, s3->h};
				SDL_RenderCopy(r, t3, nullptr, &dst3);
				SDL_DestroyTexture(t3);
			}
			SDL_FreeSurface(s3);
		}
		
		// 卡牌介绍标签
		SDL_Surface* s4 = TTF_RenderUTF8_Blended(infoFont_, "卡牌介绍", textColor);
		if (s4) {
			SDL_Texture* t4 = SDL_CreateTextureFromSurface(r, s4);
			if (t4) {
				SDL_Rect dst4{cardInfoRect_.x + 10, cardInfoRect_.y + 10, s4->w, s4->h};
				SDL_RenderCopy(r, t4, nullptr, &dst4);
				SDL_DestroyTexture(t4);
			}
			SDL_FreeSurface(s4);
		}
		
		// 道具区域标签
		SDL_Surface* s5 = TTF_RenderUTF8_Blended(infoFont_, "道具", textColor);
		if (s5) {
			SDL_Texture* t5 = SDL_CreateTextureFromSurface(r, s5);
			if (t5) {
				SDL_Rect dst5{itemAreaRect_.x + 10, itemAreaRect_.y + 10, s5->w, s5->h};
				SDL_RenderCopy(r, t5, nullptr, &dst5);
				SDL_DestroyTexture(t5);
			}
			SDL_FreeSurface(s5);
		}
	}
	
	// 绘制墨锭牌堆（右下角）
	SDL_SetRenderDrawColor(r, 35, 35, 35, 200);
	SDL_RenderFillRect(r, &inkPileRect_);
	SDL_SetRenderDrawColor(r, 70, 70, 70, 255);
	SDL_RenderDrawRect(r, &inkPileRect_);
	
	// 绘制玩家牌堆（右下角）
	SDL_SetRenderDrawColor(r, 40, 40, 40, 200);
	SDL_RenderFillRect(r, &playerPileRect_);
	SDL_SetRenderDrawColor(r, 75, 75, 75, 255);
	SDL_RenderDrawRect(r, &playerPileRect_);
	
	// 绘制牌堆文字
	if (infoFont_) {
		SDL_Color textColor{220, 220, 220, 255}; // 水墨风格文字颜色
		
		// 墨锭牌堆文字
		SDL_Surface* s1 = TTF_RenderUTF8_Blended(infoFont_, "墨锭牌堆", textColor);
		if (s1) {
			SDL_Texture* t1 = SDL_CreateTextureFromSurface(r, s1);
			if (t1) {
				SDL_Rect dst1{inkPileRect_.x + 5, inkPileRect_.y + 5, s1->w, s1->h};
				SDL_RenderCopy(r, t1, nullptr, &dst1);
				SDL_DestroyTexture(t1);
			}
			SDL_FreeSurface(s1);
		}
		
		// 玩家牌堆文字
		SDL_Surface* s2 = TTF_RenderUTF8_Blended(infoFont_, "玩家牌堆", textColor);
		if (s2) {
			SDL_Texture* t2 = SDL_CreateTextureFromSurface(r, s2);
			if (t2) {
				SDL_Rect dst2{playerPileRect_.x + 5, playerPileRect_.y + 5, s2->w, s2->h};
				SDL_RenderCopy(r, t2, nullptr, &dst2);
				SDL_DestroyTexture(t2);
			}
			SDL_FreeSurface(s2);
		}
		
		// 牌堆数量显示
		std::string inkCountText = "x" + std::to_string(inkPileCount_);
		SDL_Surface* s3 = TTF_RenderUTF8_Blended(infoFont_, inkCountText.c_str(), textColor);
		if (s3) {
			SDL_Texture* t3 = SDL_CreateTextureFromSurface(r, s3);
			if (t3) {
				SDL_Rect dst3{inkPileRect_.x + 5, inkPileRect_.y + 25, s3->w, s3->h};
				SDL_RenderCopy(r, t3, nullptr, &dst3);
				SDL_DestroyTexture(t3);
			}
			SDL_FreeSurface(s3);
		}
		
		std::string playerCountText = "x" + std::to_string(playerPileCount_);
		SDL_Surface* s4 = TTF_RenderUTF8_Blended(infoFont_, playerCountText.c_str(), textColor);
		if (s4) {
			SDL_Texture* t4 = SDL_CreateTextureFromSurface(r, s4);
			if (t4) {
				SDL_Rect dst4{playerPileRect_.x + 5, playerPileRect_.y + 25, s4->w, s4->h};
				SDL_RenderCopy(r, t4, nullptr, &dst4);
				SDL_DestroyTexture(t4);
			}
			SDL_FreeSurface(s4);
		}
	}
	
		// 显示献祭进度（墨滴）- 放在战斗牌位和牌堆之间，竖着显示
		if (isSacrificing_ || showSacrificeInk_) {
			int progressX = battlefieldRect_.w + 80; // 战场右侧
			int progressY = battlefieldRect_.y + battlefieldRect_.h / 2 + 100; // 战场中央偏上
			int dropSize = 20; // 墨滴尺寸
			int dropSpacing = 25; // 垂直间距
			
			// 绘制墨滴背景框（竖着）
			SDL_SetRenderDrawColor(r, 50, 50, 50, 150);
			SDL_Rect dropBg{progressX - 10, progressY - 10, dropSize + 20, currentSacrificeInk_ * dropSpacing + 20};
			SDL_RenderFillRect(r, &dropBg);
			SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
			SDL_RenderDrawRect(r, &dropBg);
			
			// 绘制墨滴（献祭多少就显示多少个，竖着排列）
			for (int i = 0; i < currentSacrificeInk_; ++i) {
				int x = progressX;
				int y = progressY + i * dropSpacing;
				
				// 绘制墨滴（黑色圆形）
				SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
				SDL_Rect drop{x, y, dropSize, dropSize};
				SDL_RenderFillRect(r, &drop);
				SDL_SetRenderDrawColor(r, 50, 50, 50, 255);
				SDL_RenderDrawRect(r, &drop);
				
				// 绘制墨滴尾巴（向下）
				SDL_RenderDrawLine(r, x + dropSize/2, y + dropSize, x + dropSize/2, y + dropSize + dropSize/2);
			}
			
			// 显示进度文字（在墨滴下方）
			if (infoFont_) {
				SDL_Color progressCol{220, 220, 220, 255};
				std::string progressText = std::to_string(currentSacrificeInk_) + "/" + std::to_string(sacrificeTargetCost_);
				SDL_Surface* s = TTF_RenderUTF8_Blended(infoFont_, progressText.c_str(), progressCol);
				if (s) {
					SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
					SDL_Rect dst{progressX - s->w/2, progressY + currentSacrificeInk_ * dropSpacing + 10, s->w, s->h};
					SDL_RenderCopy(r, t, nullptr, &dst);
					SDL_DestroyTexture(t);
					SDL_FreeSurface(s);
				}
			}
		}
	
	// 渲染按钮
	if (backButton_) backButton_->render(r);
	
	// 显示状态消息（在敌人区域）
	if (!statusMessage_.empty() && infoFont_) {
		SDL_Color statusColor{255, 255, 255, 255}; // 白色文字
		SDL_Surface* statusSurface = TTF_RenderUTF8_Blended(infoFont_, statusMessage_.c_str(), statusColor);
		if (statusSurface) {
			SDL_Texture* statusTexture = SDL_CreateTextureFromSurface(r, statusSurface);
			if (statusTexture) {
				// 在敌人区域中央显示状态消息
				SDL_Rect statusRect{enemyAreaRect_.x + enemyAreaRect_.w / 2 - statusSurface->w / 2, 
								   enemyAreaRect_.y + enemyAreaRect_.h / 2 - statusSurface->h / 2, 
								   statusSurface->w, statusSurface->h};
				
				// 绘制半透明背景
				SDL_SetRenderDrawColor(r, 0, 0, 0, 150);
				SDL_Rect statusBg{statusRect.x - 10, statusRect.y - 5, statusRect.w + 20, statusRect.h + 10};
				SDL_RenderFillRect(r, &statusBg);
				
				// 绘制状态消息
				SDL_RenderCopy(r, statusTexture, nullptr, &statusRect);
				SDL_DestroyTexture(statusTexture);
			}
			SDL_FreeSurface(statusSurface);
		}
	}
}

// 3D效果已移除，恢复到原始状态