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
	if (enemyFont_) TTF_CloseFont(enemyFont_);
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
	enemyFont_ = TTF_OpenFont("assets/fonts/Sanji.ttf", 48);  // 敌人区域使用更大字体

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
	
	// 处理键盘事件
	if (e.type == SDL_KEYDOWN) {
		if (e.key.keysym.sym == SDLK_t) {
			// T键切换上帝模式
			godMode_ = !godMode_;
			if (godMode_) {
				statusMessage_ = "上帝模式已开启！悬停在敌方区域按A键生成碧蟾，按S键生成玄乌";
			} else {
				statusMessage_ = "上帝模式已关闭";
			}
		} else if (e.key.keysym.sym == SDLK_a && godMode_) {
			// A键在悬停位置生成碧蟾
			if (hoveredBattlefieldIndex_ >= 0 && hoveredBattlefieldIndex_ < TOTAL_BATTLEFIELD_SLOTS) {
				int row = hoveredBattlefieldIndex_ / BATTLEFIELD_COLS;
				if (row < 2) { // 只能在敌方区域（前两行）生成
					if (!battlefield_[hoveredBattlefieldIndex_].isAlive) {
						// 生成碧蟾卡牌
						Card bichan = CardDB::instance().make("bichan");
						if (!bichan.id.empty()) {
							battlefield_[hoveredBattlefieldIndex_].card = bichan;
							battlefield_[hoveredBattlefieldIndex_].isAlive = true;
							battlefield_[hoveredBattlefieldIndex_].health = bichan.health;
							battlefield_[hoveredBattlefieldIndex_].isPlayer = false; // 敌方卡牌
							statusMessage_ = "在敌方区域生成碧蟾";
						}
					} else {
						statusMessage_ = "该位置已有卡牌";
					}
				} else {
					statusMessage_ = "只能在敌方区域（前两行）生成卡牌";
				}
			} else {
				statusMessage_ = "请悬停在敌方区域再按A键";
			}
		} else if (e.key.keysym.sym == SDLK_s && godMode_) {
			// S键在悬停位置生成玄乌
			if (hoveredBattlefieldIndex_ >= 0 && hoveredBattlefieldIndex_ < TOTAL_BATTLEFIELD_SLOTS) {
				int row = hoveredBattlefieldIndex_ / BATTLEFIELD_COLS;
				if (row < 2) { // 只能在敌方区域（前两行）生成
					if (!battlefield_[hoveredBattlefieldIndex_].isAlive) {
						// 生成玄乌卡牌
						Card xuanwu = CardDB::instance().make("xuanwu");
						if (!xuanwu.id.empty()) {
							battlefield_[hoveredBattlefieldIndex_].card = xuanwu;
							battlefield_[hoveredBattlefieldIndex_].isAlive = true;
							battlefield_[hoveredBattlefieldIndex_].health = xuanwu.health;
							battlefield_[hoveredBattlefieldIndex_].isPlayer = false; // 敌方卡牌
							statusMessage_ = "在敌方区域生成玄乌";
						}
					} else {
						statusMessage_ = "该位置已有卡牌";
					}
				} else {
					statusMessage_ = "只能在敌方区域（前两行）生成卡牌";
				}
			} else {
				statusMessage_ = "请悬停在敌方区域再按S键";
			}
		}
	}
	
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
		

		// 检查牌堆点击（回合制限制）
		if (mouseX >= inkPileRect_.x && mouseX <= inkPileRect_.x + inkPileRect_.w &&
			mouseY >= inkPileRect_.y && mouseY <= inkPileRect_.y + inkPileRect_.h) {
			// 检查是否在献祭模式
			if (isSacrificing_) {
				statusMessage_ = "献祭模式中，请点击场上卡牌献祭或右键取消！";
				return;
			}
			
			// 检查是否在打出阶段
			if (showSacrificeInk_) {
				statusMessage_ = "请先打出选中的卡牌！";
				return;
			}
			
			// 点击墨锭牌堆
			if (currentTurn_ == 1) {
				statusMessage_ = "第一回合不能抽牌！";
			} else if (hasDrawnThisTurn_) {
				statusMessage_ = "本回合已抽过牌！";
			} else if (inkPileCount_ > 0) {
				Card inkCard = CardDB::instance().make("moding");
				if (!inkCard.id.empty()) {
					handCards_.push_back(inkCard);
					inkPileCount_--;
					hasDrawnThisTurn_ = true;
					mustDrawThisTurn_ = false; // 抽牌后取消强制抽牌要求
					layoutHandCards();
					statusMessage_ = "从墨锭牌堆抽到：" + inkCard.name;
				}
			} else {
				statusMessage_ = "墨锭牌堆已空！";
			}
		} else if (mouseX >= playerPileRect_.x && mouseX <= playerPileRect_.x + playerPileRect_.w &&
				   mouseY >= playerPileRect_.y && mouseY <= playerPileRect_.y + playerPileRect_.h) {
			// 检查是否在献祭模式
			if (isSacrificing_) {
				statusMessage_ = "献祭模式中，请点击场上卡牌献祭或右键取消！";
				return;
			}
			
			// 检查是否在打出阶段
			if (showSacrificeInk_) {
				statusMessage_ = "请先打出选中的卡牌！";
				return;
			}
			
			// 点击玩家牌堆
			if (currentTurn_ == 1) {
				statusMessage_ = "第一回合不能抽牌！";
			} else if (hasDrawnThisTurn_) {
				statusMessage_ = "本回合已抽过牌！";
			} else if (playerPileCount_ > 0 && !playerDeck_.empty()) {
				// 从固定牌堆中抽取第一张牌
				std::string cardId = playerDeck_[0];
				Card newCard = CardDB::instance().make(cardId);
				if (!newCard.id.empty()) {
					handCards_.push_back(newCard);
					playerDeck_.erase(playerDeck_.begin()); // 移除已抽取的牌
					playerPileCount_--;
					hasDrawnThisTurn_ = true;
					mustDrawThisTurn_ = false; // 抽牌后取消强制抽牌要求
					layoutHandCards();
					statusMessage_ = "从玩家牌堆抽到：" + newCard.name;
				}
			} else {
				statusMessage_ = "玩家牌堆已空！";
			}
		} else if (mouseX >= inkStoneRect_.x && mouseX <= inkStoneRect_.x + inkStoneRect_.w &&
				   mouseY >= inkStoneRect_.y && mouseY <= inkStoneRect_.y + inkStoneRect_.h) {
			// 点击砚台（结束回合）
			if (currentPhase_ == GamePhase::PlayerTurn) {
				// 检查是否必须抽牌
				if (mustDrawThisTurn_) {
					statusMessage_ = "必须先抽牌才能结束回合！";
				} else if (isSacrificing_) {
					statusMessage_ = "献祭模式中，请点击场上卡牌献祭或右键取消！";
				} else if (showSacrificeInk_ || selectedHandCard_ >= 0) {
					statusMessage_ = "请先打出选中的卡牌！";
				} else {
					endTurn();
				}
			}
		}
		
		// 检查手牌点击
		for (size_t i = 0; i < handCardRects_.size(); ++i) {
			const auto& rect = handCardRects_[i];
			if (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
				mouseY >= rect.y && mouseY <= rect.y + rect.h) {
				// 检查是否必须抽牌
				if (mustDrawThisTurn_) {
					statusMessage_ = "必须先抽牌才能使用手牌！";
					return;
				}
				
				// 检查是否在献祭模式
				if (isSacrificing_) {
					statusMessage_ = "献祭模式中，请点击场上卡牌献祭或右键取消！";
					return;
				}
				
				// 检查是否在打出阶段
				if (showSacrificeInk_) {
					statusMessage_ = "请先打出选中的卡牌！";
					return;
				}
				
				// 检查是否已经有选中的卡牌在等待打出
				if (selectedHandCard_ >= 0) {
					statusMessage_ = "请先打出已选中的卡牌！";
					return;
				}
				selectedHandCard_ = static_cast<int>(i);
				
				// 检查是否有消耗骨头词条
				bool hasBoneCost = false;
				for (const auto& mark : handCards_[i].marks) {
					if (mark == "消耗骨头") {
						hasBoneCost = true;
						break;
					}
				}
				
				if (hasBoneCost) {
					// 消耗骨头的卡牌，检查魂骨是否足够
					int boneCost = handCards_[i].sacrificeCost;
					if (boneCount_ < boneCost) {
						statusMessage_ = "魂骨不足！需要 " + std::to_string(boneCost) + " 个魂骨，当前有 " + std::to_string(boneCount_) + " 个";
					} else {
						isSacrificing_ = false;
						statusMessage_ = "已选择手牌：" + handCards_[i].name + "（消耗 " + std::to_string(boneCost) + " 个魂骨）";
					}
				} else if (handCards_[i].sacrificeCost > 0) {
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
					// 检查是否必须抽牌
					if (mustDrawThisTurn_) {
						statusMessage_ = "必须先抽牌才能进行其他操作！";
						return;
					}
					
					// 计算行号（0=第一行，1=第二行，2=第三行）
					int row = i / BATTLEFIELD_COLS;
					
					if (isSacrificing_) {
						// Sacrifice mode: click battlefield cards to sacrifice
						
						if (battlefield_[i].isAlive && battlefield_[i].isPlayer && !battlefield_[i].isSacrificed) {
							// 献祭这张卡牌（点击就加点数，出现墨滴，标记为已献祭）
							
							
							// 计算献祭获得的墨量（检查优质祭品印记）
							int inkGained = battlefield_[i].card.sacrificeCost;
							if (hasMark(battlefield_[i].card, u8"优质祭品")) {
								inkGained = 3; // 优质祭品提供3点墨量
								currentSacrificeInk_+= inkGained;
								statusMessage_ = "优质祭品！获得3点墨量，当前：" + std::to_string(currentSacrificeInk_) + "/" + std::to_string(sacrificeTargetCost_);
							} else {
								currentSacrificeInk_++;
								statusMessage_ = "献祭获得 " + std::to_string(inkGained) + " 点墨量，当前：" + std::to_string(currentSacrificeInk_) + "/" + std::to_string(sacrificeTargetCost_);
							}
							
						
							battlefield_[i].isSacrificed = true; // 标记为已献祭
							
							// 检查是否献祭足够
							if (currentSacrificeInk_ >= sacrificeTargetCost_) {
								// 献祭完成，开始摧毁动画
								isSacrificing_ = false;
								showSacrificeInk_ = true;  // 开始显示献墨量
								
								// 收集待摧毁的卡牌，检查生生不息印记
								cardsToDestroy_.clear();
								for (int j = 0; j < TOTAL_BATTLEFIELD_SLOTS; ++j) {
									if (battlefield_[j].isSacrificed) {
										// 检查是否有生生不息印记
										bool hasImmortal = hasMark(battlefield_[j].card, u8"生生不息");
										
										if (hasImmortal) {
											// 生生不息：不死亡，但依然产生魂骨
											boneCount_++;
											statusMessage_ = "生生不息！获得魂骨！当前魂骨数量: " + std::to_string(boneCount_);
										} else {
											// 正常献祭：标记为死亡
											battlefield_[j].isAlive = false;
											cardsToDestroy_.push_back(j);
										}
									}
								}
								
								// 如果有卡牌需要摧毁，启动摧毁动画
								if (!cardsToDestroy_.empty()) {
									isDestroyAnimating_ = true;
									destroyAnimTime_ = 0.0f;
									statusMessage_ = "献祭完成！卡牌正在消失...";
								} else {
									statusMessage_ = "献祭完成！生生不息卡牌存活！";
								}
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
								// 检查卡牌类型
								bool hasBoneCost = false;
								for (const auto& mark : handCards_[selectedHandCard_].marks) {
									if (mark == "消耗骨头") {
										hasBoneCost = true;
										break;
									}
								}
								
								if (hasBoneCost) {
									// 消耗骨头的卡牌，检查魂骨是否足够
									int boneCost = handCards_[selectedHandCard_].sacrificeCost;
									if (boneCount_ < boneCost) {
										statusMessage_ = "魂骨不足！需要 " + std::to_string(boneCost) + " 个魂骨，当前有 " + std::to_string(boneCount_) + " 个";
									} else {
										playCard(selectedHandCard_, i);
										selectedHandCard_ = -1;
										statusMessage_ = "卡牌已放置在第三行";
									}
								} else if (handCards_[selectedHandCard_].sacrificeCost > 0 && isSacrificing_) {
									// 需要献祭的卡牌
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

		// 检查是否必须抽牌
		if (mustDrawThisTurn_) {
			statusMessage_ = "必须先抽牌才能进行其他操作！";
			return;
		}

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
		
		// 检查是否在打出阶段，右键取消打出
		if (selectedHandCard_ >= 0) {
			// 检查选中的卡牌是否需要献墨点数
			bool needsSacrifice = false;
			for (const auto& mark : handCards_[selectedHandCard_].marks) {
				if (mark == "消耗骨头") {
					// 消耗骨头的卡牌不需要献祭
					needsSacrifice = false;
					break;
				}
			}
			
			if (!needsSacrifice && handCards_[selectedHandCard_].sacrificeCost > 0) {
				needsSacrifice = true;
			}
			
			if (!needsSacrifice) {
				// 不需要献墨点数的卡牌，可以右键取消打出
				selectedHandCard_ = -1;
				showSacrificeInk_ = false;
				statusMessage_ = "已取消打出";
				return;
			} else {
				// 需要献墨点数的卡牌，不能右键取消
				statusMessage_ = "需要献墨的卡牌不能取消打出！";
				return;
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
	
	
	// 更新伤害显示
	if (showDamage_) {
		damageDisplayTime_ += dt;
		if (damageDisplayTime_ >= damageDisplayDuration_) {
			showDamage_ = false;
		}
	}
	
	// 处理攻击动画
	if (isAttackAnimating_) {
		attackAnimTime_ += dt;
		
		// 检查是否需要切换到下一张卡牌攻击
		if (currentAttackingIndex_ < static_cast<int>(attackingCards_.size())) {
			int currentCardIndex = attackingCards_[currentAttackingIndex_];
			
			// 检查是否是特殊攻击
			const auto& attacker = battlefield_[currentCardIndex];
			BattleState::AttackType attackType = getCardAttackType(attacker.card);
			
			if (attackType != BattleState::AttackType::Normal) {
				// 特殊攻击：设置特殊攻击动画，但不立即切换卡牌
				if (!isSpecialAttackAnimating_) {
					int col = currentCardIndex % BATTLEFIELD_COLS;
					executeSpecialAttack(currentCardIndex, col, isPlayerAttacking_);
				}
			} else {
				// 普通攻击：执行普通攻击逻辑
				if (attackAnimTime_ >= attackAnimDuration_ / 2.0f && !hasAttacked_) {
					hasAttacked_ = true; // 标记已攻击，防止重复执行
					
					// 执行普通攻击
					int col = currentCardIndex % BATTLEFIELD_COLS;
					executeNormalAttack(currentCardIndex, col, isPlayerAttacking_, attacker.card.attack);
				}
			}
			
			// 当前卡牌攻击动画完成，切换到下一张
			if (attackAnimTime_ >= attackAnimDuration_) {
				BattleState::AttackType attackType = getCardAttackType(attacker.card);
				if (attackType == BattleState::AttackType::Normal) {
					// 普通攻击：直接切换下一张卡牌
					currentAttackingIndex_++;
					attackAnimTime_ = 0.0f;
					hasAttacked_ = false; // 重置攻击标志，为下一张卡牌准备
				}
				// 特殊攻击：等待特殊攻击动画完成后再切换（在特殊攻击动画逻辑中处理）
			}
		} else {
			// 所有卡牌攻击完成
			isAttackAnimating_ = false;
			attackAnimTime_ = 0.0f;
			attackingCards_.clear();
			currentAttackingIndex_ = 0;
			
			if (isPlayerAttacking_) {
				// 玩家攻击完成，进入敌方回合
				if (totalDamageDealt_ > 0) {
					showDamage_ = true;
					damageDisplayTime_ = 0.0f;
					statusMessage_ = "造成 " + std::to_string(totalDamageDealt_) + " 点伤害！";
				} else {
					statusMessage_ = "我方攻击完成";
				}
				
				currentPhase_ = GamePhase::EnemyTurn;
				currentTurn_++; // 增加回合数
				hasDrawnThisTurn_ = false; // 重置抽牌状态
				mustDrawThisTurn_ = (currentTurn_ >= 2); // 第二回合开始必须抽牌
				
				// 每回合开始时恢复一些墨量（已移除墨量系统）
				
				// 延迟执行敌人回合
				enemyTurn();
			} else {
				// 敌方攻击完成，回到玩家回合
				currentPhase_ = GamePhase::PlayerTurn;
				if (mustDrawThisTurn_) {
					statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始 - 必须先抽牌！";
				} else {
					statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始";
				}
			}
		}
	}
	
	// 处理特殊攻击动画
	if (isSpecialAttackAnimating_) {
		attackAnimTime_ += dt;
		
		// 检查当前目标是否有效
		if (currentTargetIndex_ < 3 && specialAttackTargets_[currentTargetIndex_] != -1) {
			int currentTargetIndex = specialAttackTargets_[currentTargetIndex_];
			
			// 攻击动画进行到一半时执行攻击逻辑
			if (attackAnimTime_ >= attackAnimDuration_ / 2.0f && !hasAttacked_) {
				hasAttacked_ = true;
				
				// 执行对当前目标的攻击
				int attackerIndex = attackingCards_[currentAttackingIndex_];
				int damage = battlefield_[attackerIndex].card.attack;
				attackTarget(attackerIndex, currentTargetIndex, damage);
			}
			
			// 当前目标攻击完成，切换到下一个目标
			if (attackAnimTime_ >= attackAnimDuration_) {
				currentTargetIndex_++;
				attackAnimTime_ = 0.0f;
				hasAttacked_ = false;
				
				// 检查是否所有目标都攻击完成
				if (currentTargetIndex_ >= 3 || specialAttackTargets_[currentTargetIndex_] == -1) {
					isSpecialAttackAnimating_ = false;
					// 继续正常的攻击流程
					currentAttackingIndex_++;
					attackAnimTime_ = 0.0f;
					hasAttacked_ = false;
					
					// 特殊攻击完成，回到正常攻击流程
					// 不在这里检查是否所有卡牌都攻击完成，让正常攻击流程处理
				}
			}
		} else {
			// 特殊攻击完成
			isSpecialAttackAnimating_ = false;
		}
	}
	
	// 处理摧毁动画
	if (isDestroyAnimating_) {
		destroyAnimTime_ += dt;
		if (destroyAnimTime_ >= destroyAnimDuration_) {
		// 动画结束，卡牌已经死亡，只需要清理动画状态
		// 注意：卡牌在攻击时已经设置为isAlive = false，这里不需要重复设置
		// 魂骨获取由下面的状态检测逻辑处理，避免重复计算
			
			// 检查是否是献祭完成的动画
			if (showSacrificeInk_) {
				statusMessage_ = "献祭完成！可以打出卡牌";
				showSacrificeInk_ = false;
			}
			
			isDestroyAnimating_ = false;
			destroyAnimTime_ = 0.0f;
			cardsToDestroy_.clear();
		}
	}
	
	// 检测卡牌死亡，获得魂骨
	for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
		bool wasAlive = previousCardStates_[i];
		bool isAlive = battlefield_[i].isAlive && battlefield_[i].isPlayer;

		// 如果卡牌从存活变为死亡，获得魂骨
		if (wasAlive && !isAlive) {
			// 检查是否是生生不息卡牌在献祭时死亡（这种情况已经在献祭逻辑中处理了魂骨）
			bool wasSacrificed = battlefield_[i].isSacrificed;
			bool hasImmortal = hasMark(battlefield_[i].card, u8"生生不息");
			
			// 如果不是生生不息卡牌在献祭时死亡，则获得魂骨
			if (!(wasSacrificed && hasImmortal)) {
				boneCount_++;
				statusMessage_ = "获得魂骨！当前魂骨数量: " + std::to_string(boneCount_);
			}
		}

		// 更新状态记录
		previousCardStates_[i] = isAlive;
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
	
	// 初始化固定玩家牌堆（三张玄牡和两张千峰驼鹿）
	playerDeck_.clear();
	playerDeck_.push_back("xuanmu");             // 玄牡
	playerDeck_.push_back("xuanmu");             // 玄牡
	playerDeck_.push_back("qianfeng_tuolu"); 
	playerDeck_.push_back("xuanmu");             // 玄牡
	playerDeck_.push_back("qianfeng_tuolu");    // 千峰驼鹿
	   // 千峰驼鹿
	playerPileCount_ = static_cast<int>(playerDeck_.size());
	
	// 抽3张玩家牌（从固定玩家牌堆中抽取）
	for (int i = 0; i < 3 && i < static_cast<int>(playerDeck_.size()); ++i) {
		std::string cardId = playerDeck_[0];
		Card card = CardDB::instance().make(cardId);
		if (!card.id.empty()) {
			handCards_.push_back(card);
		}
		// 从牌堆中移除已抽取的卡牌
		playerDeck_.erase(playerDeck_.begin());
		playerPileCount_--;
	}
	
	// 调试信息：确认手牌数量
	statusMessage_ = "开局手牌数量: " + std::to_string(handCards_.size()) + " (1张墨锭 + 3张玩家牌)";
	
	// 初始化牌堆
	inkPileCount_ = 10;  // 墨锭牌堆只有10张
	
	// 初始化回合制系统
	hasDrawnThisTurn_ = false;
	mustDrawThisTurn_ = false;  // 第一回合不需要强制抽牌
	
	// 初始化战斗系统
	enemyHealth_ = 20;
	totalDamageDealt_ = 0;
	showDamage_ = false;
	damageDisplayTime_ = 0.0f;
	
	// 初始化魂骨系统
	boneCount_ = 0;
	previousCardStates_.resize(TOTAL_BATTLEFIELD_SLOTS, false);
	
	// 初始化上帝模式系统
	godMode_ = false;
	enemyTurnStarted_ = false;
	
	// 初始化摧毁动画系统
	isDestroyAnimating_ = false;
	destroyAnimTime_ = 0.0f;
	cardsToDestroy_.clear();
	
	// 初始化攻击动画系统
	isAttackAnimating_ = false;
	attackAnimTime_ = 0.0f;
	attackingCards_.clear();
	currentAttackingIndex_ = 0;
	isPlayerAttacking_ = true;
	hasAttacked_ = false;
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
		300,  // 从400减少到300，右边界向左移动
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
	
	// 检查消耗骨头
	bool hasBoneCost = false;
	int boneCost = 0;
	for (const auto& mark : card.marks) {
		if (mark == "消耗骨头") {
			hasBoneCost = true;
			boneCost = card.sacrificeCost; // 使用sacrificeCost字段存储骨头消耗数量
			break;
		}
	}
	
	if (hasBoneCost) {
		// 需要消耗魂骨
		if (boneCount_ < boneCost) {
			statusMessage_ = "魂骨不足！需要 " + std::to_string(boneCost) + " 个魂骨，当前有 " + std::to_string(boneCount_) + " 个";
			return;
		}
		// 消耗魂骨
		boneCount_ -= boneCost;
		statusMessage_ = "消耗 " + std::to_string(boneCost) + " 个魂骨，打出：" + card.name;
	} else {
		// 检查献墨点数（非骨头消耗的卡牌）
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
	
	// 重置献祭状态
	if (card.sacrificeCost > 0) {
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
	
	// 收集所有可以攻击的我方卡牌（从左到右）
	attackingCards_.clear();
	for (int col = 0; col < BATTLEFIELD_COLS; ++col) {
		int playerIndex = 2 * BATTLEFIELD_COLS + col; // 第三行
		if (battlefield_[playerIndex].isAlive && battlefield_[playerIndex].isPlayer && 
			battlefield_[playerIndex].card.attack > 0) {
			attackingCards_.push_back(playerIndex);
		}
	}
	
	if (!attackingCards_.empty()) {
		// 开始攻击动画
		isAttackAnimating_ = true;
		attackAnimTime_ = 0.0f;
		currentAttackingIndex_ = 0;
		isPlayerAttacking_ = true;
		totalDamageDealt_ = 0;
		statusMessage_ = "我方开始攻击！";
	} else {
		// 没有可攻击的卡牌，直接进入敌方回合
		currentPhase_ = GamePhase::EnemyTurn;
		currentTurn_++; // 增加回合数
		hasDrawnThisTurn_ = false; // 重置抽牌状态
		mustDrawThisTurn_ = (currentTurn_ >= 2); // 第二回合开始必须抽牌
		
		// 每回合开始时恢复一些墨量（已移除墨量系统）
		
		// 延迟执行敌人回合
		enemyTurn();
	}
}

void BattleState::enemyTurn() {
	// 收集所有可以攻击的敌方卡牌（从左到右）
	attackingCards_.clear();
	for (int col = 0; col < BATTLEFIELD_COLS; ++col) {
		int enemyIndex = 1 * BATTLEFIELD_COLS + col;  // 第二行（敌方攻击行）
		if (battlefield_[enemyIndex].isAlive && !battlefield_[enemyIndex].isPlayer && 
			battlefield_[enemyIndex].card.attack > 0) {
			attackingCards_.push_back(enemyIndex);
		}
	}
	
	if (!attackingCards_.empty()) {
		// 开始敌方攻击动画
		isAttackAnimating_ = true;
		attackAnimTime_ = 0.0f;
		currentAttackingIndex_ = 0;
		isPlayerAttacking_ = false;
		statusMessage_ = "敌方开始攻击！";
				} else {
		// 没有可攻击的敌方卡牌，直接回到玩家回合
		currentPhase_ = GamePhase::PlayerTurn;
		if (mustDrawThisTurn_) {
			statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始 - 必须先抽牌！";
		} else {
			statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始";
		}
	}
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
	
	// 第一遍：绘制所有非攻击中的卡牌
	for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
		const auto& bfCard = battlefield_[i];
		
		// 检查是否正在播放攻击动画
		bool isAttacking = false;
		if (isAttackAnimating_ && currentAttackingIndex_ < static_cast<int>(attackingCards_.size())) {
			int currentCardIndex = attackingCards_[currentAttackingIndex_];
			if (currentCardIndex == i) {
				isAttacking = true;
			}
		}
		
		// 跳过正在攻击的卡牌，它们会在第二遍渲染
		if (isAttacking) continue;
		
		// 检查是否悬停，改变颜色
		if (i == hoveredBattlefieldIndex_) {
			// 悬停时的高亮效果
			SDL_SetRenderDrawColor(r, 100, 150, 200, 100);
			SDL_RenderFillRect(r, &bfCard.rect);
			SDL_SetRenderDrawColor(r, 150, 200, 255, 255);
			SDL_RenderDrawRect(r, &bfCard.rect);
		}
		
		// 检查是否正在播放摧毁动画
		bool isAnimating = false;
		float animProgress = 0.0f;
		if (isDestroyAnimating_) {
			for (int cardIndex : cardsToDestroy_) {
				if (cardIndex == i) {
					isAnimating = true;
					animProgress = destroyAnimTime_ / destroyAnimDuration_;
					break;
				}
			}
		}
		
		// 如果卡牌存活或者正在播放摧毁动画，则渲染
		if (bfCard.isAlive || isAnimating) {
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
			
			// 使用CardRenderer渲染卡牌，使用当前血量
			Card tempCard = bfCard.card;
			tempCard.health = bfCard.health; // 使用当前血量
			CardRenderer::renderCard(app, tempCard, renderRect, cardNameFont_, cardStatFont_, false);
			
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
				} else {
			// 绘制空槽位
			SDL_SetRenderDrawColor(r, 60, 70, 80, 80);
			SDL_RenderFillRect(r, &bfCard.rect);
			SDL_SetRenderDrawColor(r, 100, 110, 120, 150);
			SDL_RenderDrawRect(r, &bfCard.rect);
		}
	}
	
	// 第二遍：绘制正在攻击的卡牌（最上层）- 只在普通攻击时显示
	if (isAttackAnimating_ && !isSpecialAttackAnimating_ && currentAttackingIndex_ < static_cast<int>(attackingCards_.size())) {
		// 检查当前卡牌是否是特殊攻击
		int currentCardIndex = attackingCards_[currentAttackingIndex_];
		const auto& attacker = battlefield_[currentCardIndex];
		BattleState::AttackType attackType = getCardAttackType(attacker.card);
		
		// 只有普通攻击才显示普通攻击动画
		if (attackType == BattleState::AttackType::Normal) {
			int currentCardIndex = attackingCards_[currentAttackingIndex_];
			const auto& bfCard = battlefield_[currentCardIndex];
			
			if (bfCard.isAlive) {
				float attackProgress = attackAnimTime_ / attackAnimDuration_;
				
				// 计算攻击动画效果
				SDL_Rect renderRect = bfCard.rect;
				
				// 攻击动画效果：卡牌向前移动并闪烁
				float moveDistance = 40.0f * std::sin(attackProgress * 3.14159f); // 增加移动距离
				float flashIntensity = 0.3f + 0.7f * std::sin(attackProgress * 12.56636f); // 更快的闪烁
				
				// 根据攻击方向移动卡牌
				if (isPlayerAttacking_) {
					// 玩家攻击：向上移动
					renderRect.y -= static_cast<int>(moveDistance);
				} else {
					// 敌方攻击：向下移动
					renderRect.y += static_cast<int>(moveDistance);
				}
				
				// 使用CardRenderer渲染卡牌，使用当前血量
				Card tempCard = bfCard.card;
				tempCard.health = bfCard.health; // 使用当前血量
				CardRenderer::renderCard(app, tempCard, renderRect, cardNameFont_, cardStatFont_, false);
				
				// 添加多层闪烁边框效果
				SDL_SetRenderDrawColor(r, 255, 255, 0, static_cast<Uint8>(255 * flashIntensity));
				SDL_RenderDrawRect(r, &renderRect);
				
				// 添加外圈高亮效果
				SDL_SetRenderDrawColor(r, 255, 200, 0, static_cast<Uint8>(128 * flashIntensity));
				SDL_Rect outerRect = renderRect;
				outerRect.x -= 3;
				outerRect.y -= 3;
				outerRect.w += 6;
				outerRect.h += 6;
				SDL_RenderDrawRect(r, &outerRect);
				
				// 添加攻击特效：从卡牌中心向外扩散的圆圈
				int centerX = renderRect.x + renderRect.w / 2;
				int centerY = renderRect.y + renderRect.h / 2;
				int effectRadius = static_cast<int>(20.0f * attackProgress);
				
				SDL_SetRenderDrawColor(r, 255, 100, 0, static_cast<Uint8>(200 * (1.0f - attackProgress)));
				for (int i = 0; i < effectRadius; i += 2) {
					SDL_Rect effectRect = {centerX - i, centerY - i, i * 2, i * 2};
					SDL_RenderDrawRect(r, &effectRect);
				}
			}
		}
	}
	
	// 第三遍：绘制特殊攻击的卡牌动画
	if (isSpecialAttackAnimating_ && currentTargetIndex_ < 3 && specialAttackTargets_[currentTargetIndex_] != -1 && currentAttackingIndex_ < static_cast<int>(attackingCards_.size())) {
		int currentTargetIndex = specialAttackTargets_[currentTargetIndex_];
		int attackerIndex = attackingCards_[currentAttackingIndex_];
		const auto& attacker = battlefield_[attackerIndex];
		
		if (attacker.isAlive) {
			float attackProgress = attackAnimTime_ / attackAnimDuration_;
			
			// 计算攻击动画效果
			SDL_Rect renderRect = attacker.rect;
			
			// 攻击动画效果：卡牌移动并闪烁
			float moveDistance = 40.0f * std::sin(attackProgress * 3.14159f);
			float flashIntensity = 0.3f + 0.7f * std::sin(attackProgress * 12.56636f);
			
			// 根据当前目标计算移动方向
			if (currentTargetIndex != -1) {
				const auto& target = battlefield_[currentTargetIndex];
				
				// 计算攻击者到目标的方向向量
				int attackerCenterX = attacker.rect.x + attacker.rect.w / 2;
				int attackerCenterY = attacker.rect.y + attacker.rect.h / 2;
				int targetCenterX = target.rect.x + target.rect.w / 2;
				int targetCenterY = target.rect.y + target.rect.h / 2;
				
				// 计算方向向量
				float deltaX = targetCenterX - attackerCenterX;
				float deltaY = targetCenterY - attackerCenterY;
				float distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);
				
				if (distance > 0) {
					// 归一化方向向量
					float dirX = deltaX / distance;
					float dirY = deltaY / distance;
					
					// 根据方向移动卡牌
					renderRect.x += static_cast<int>(moveDistance * dirX);
					renderRect.y += static_cast<int>(moveDistance * dirY);
				}
			} else {
				// 如果没有目标，使用默认的前向移动
				if (isPlayerAttacking_) {
					renderRect.y -= static_cast<int>(moveDistance);
				} else {
					renderRect.y += static_cast<int>(moveDistance);
				}
			}
			
			// 使用CardRenderer渲染卡牌
			Card tempCard = attacker.card;
			tempCard.health = attacker.health;
			CardRenderer::renderCard(app, tempCard, renderRect, cardNameFont_, cardStatFont_, false);
			
			// 根据攻击类型显示不同的边框颜色
			switch (currentAttackType_) {
				case BattleState::AttackType::Double:
					SDL_SetRenderDrawColor(r, 0, 255, 255, static_cast<Uint8>(255 * flashIntensity)); // 青色 - 双向攻击
					break;
				case BattleState::AttackType::Triple:
					SDL_SetRenderDrawColor(r, 255, 0, 255, static_cast<Uint8>(255 * flashIntensity)); // 紫色 - 三向攻击
					break;
				case BattleState::AttackType::Twice:
					SDL_SetRenderDrawColor(r, 255, 165, 0, static_cast<Uint8>(255 * flashIntensity)); // 橙色 - 双重攻击
					break;
				case BattleState::AttackType::DoubleTwice:
					SDL_SetRenderDrawColor(r, 0, 255, 0, static_cast<Uint8>(255 * flashIntensity)); // 绿色 - 双向双重攻击
					break;
				case BattleState::AttackType::TripleTwice:
					SDL_SetRenderDrawColor(r, 255, 255, 255, static_cast<Uint8>(255 * flashIntensity)); // 白色 - 三向双重攻击
					break;
				default:
					SDL_SetRenderDrawColor(r, 255, 255, 0, static_cast<Uint8>(255 * flashIntensity)); // 黄色 - 普通攻击
					break;
			}
			SDL_RenderDrawRect(r, &renderRect);
			
			// 添加外圈高亮效果
			SDL_SetRenderDrawColor(r, 255, 200, 0, static_cast<Uint8>(128 * flashIntensity));
			SDL_Rect outerRect = renderRect;
			outerRect.x -= 3;
			outerRect.y -= 3;
			outerRect.w += 6;
			outerRect.h += 6;
			SDL_RenderDrawRect(r, &outerRect);
			
			// 添加攻击特效：从卡牌中心向外扩散的圆圈
			int centerX = renderRect.x + renderRect.w / 2;
			int centerY = renderRect.y + renderRect.h / 2;
			int effectRadius = static_cast<int>(20.0f * attackProgress);
			
			SDL_SetRenderDrawColor(r, 255, 100, 0, static_cast<Uint8>(200 * (1.0f - attackProgress)));
			for (int i = 0; i < effectRadius; i += 2) {
				SDL_Rect effectRect = {centerX - i, centerY - i, i * 2, i * 2};
				SDL_RenderDrawRect(r, &effectRect);
			}
		}
	}
	
	// 战斗牌位标注已删除
}

// 印记系统实现 - 多印记优先级处理
BattleState::AttackType BattleState::getCardAttackType(const Card& card) {
	// 检查卡牌的印记来确定攻击类型
	// 优先级：组合攻击 > 单一攻击 > 普通攻击
	// 
	// 设计理念：
	// 1. 三向双重攻击：最全面，攻击对位+左右斜对位各两次
	// 2. 双向双重攻击：攻击左右斜对位各两次，避开对位
	// 3. 三向攻击：攻击对位+左右斜对位
	// 4. 双向攻击：攻击左右斜对位，避开对位
	// 5. 双重攻击：对位攻击两次，伤害最高
	// 6. 普通攻击：基础攻击
	
	bool hasTriple = false;
	bool hasDouble = false;
	bool hasTwice = false;
	
	// 检查所有印记
	for (const auto& mark : card.marks) {
		if (mark == u8"三向攻击") {
			hasTriple = true;
		} else if (mark == u8"双向攻击") {
			hasDouble = true;
		} else if (mark == u8"双重攻击") {
			hasTwice = true;
		}
	}
	
	// 按优先级返回攻击类型（组合攻击优先）
	if (hasTriple && hasTwice) {
		return BattleState::AttackType::TripleTwice;
	} else if (hasDouble && hasTwice) {
		return BattleState::AttackType::DoubleTwice;
	} else if (hasTriple) {
		return BattleState::AttackType::Triple;
	} else if (hasDouble) {
		return BattleState::AttackType::Double;
	} else if (hasTwice) {
		return BattleState::AttackType::Twice;
	}
	
	return BattleState::AttackType::Normal;
}

// 检查卡牌是否有特定印记
bool BattleState::hasMark(const Card& card, const std::string& mark) {
	for (const auto& cardMark : card.marks) {
		if (cardMark == mark) {
			return true;
		}
	}
	return false;
}

// 执行特殊攻击
void BattleState::executeSpecialAttack(int attackerIndex, int targetCol, bool isPlayerAttacking) {
	const auto& attacker = battlefield_[attackerIndex];
	BattleState::AttackType attackType = getCardAttackType(attacker.card);
	
	// 如果是普通攻击，直接执行
	if (attackType == BattleState::AttackType::Normal) {
		executeNormalAttack(attackerIndex, targetCol, isPlayerAttacking, attacker.card.attack);
		return;
	}
	
	// 设置特殊攻击动画
	currentAttackType_ = attackType;
	isSpecialAttackAnimating_ = true;
	currentTargetIndex_ = 0;
	attackAnimTime_ = 0.0f;
	
	// 清空目标数组
	for (int i = 0; i < 3; ++i) {
		specialAttackTargets_[i] = -1;
	}
	
	// 根据攻击类型设置目标
	switch (attackType) {
		case BattleState::AttackType::Double: {
			// 双向攻击：设置左右斜对位目标
			setupDiagonalTargets(attackerIndex, targetCol, isPlayerAttacking);
			break;
		}
		case BattleState::AttackType::Triple: {
			// 三向攻击：设置对位+左右斜对位目标
			setupTripleTargets(attackerIndex, targetCol, isPlayerAttacking);
			break;
		}
		case BattleState::AttackType::Twice: {
			// 双重攻击：设置对位目标两次
			setupTwiceTargets(attackerIndex, targetCol, isPlayerAttacking);
			break;
		}
		case BattleState::AttackType::DoubleTwice: {
			// 双向双重攻击：设置左右斜对位目标各两次
			setupDoubleTwiceTargets(attackerIndex, targetCol, isPlayerAttacking);
			break;
		}
		case BattleState::AttackType::TripleTwice: {
			// 三向双重攻击：设置对位+左右斜对位目标各两次
			setupTripleTwiceTargets(attackerIndex, targetCol, isPlayerAttacking);
			break;
		}
		default:
			break;
	}
}

// 设置双向攻击目标
void BattleState::setupDiagonalTargets(int attackerIndex, int targetCol, bool isPlayerAttacking) {
	int targetCount = 0;
	
	// 左斜对位
	if (targetCol > 0) {
		int leftTargetIndex = isPlayerAttacking ? 
			(1 * BATTLEFIELD_COLS + (targetCol - 1)) : 
			(2 * BATTLEFIELD_COLS + (targetCol - 1));
		specialAttackTargets_[targetCount++] = leftTargetIndex;
	}
	
	// 右斜对位
	if (targetCol < BATTLEFIELD_COLS - 1) {
		int rightTargetIndex = isPlayerAttacking ? 
			(1 * BATTLEFIELD_COLS + (targetCol + 1)) : 
			(2 * BATTLEFIELD_COLS + (targetCol + 1));
		specialAttackTargets_[targetCount++] = rightTargetIndex;
	}
}

// 设置三向攻击目标
void BattleState::setupTripleTargets(int attackerIndex, int targetCol, bool isPlayerAttacking) {
	int targetCount = 0;
	
	// 三向攻击顺序：左斜对位 → 对位 → 右斜对位
	
	// 1. 左斜对位
	if (targetCol > 0) {
		int leftTargetIndex = isPlayerAttacking ? 
			(1 * BATTLEFIELD_COLS + (targetCol - 1)) : 
			(2 * BATTLEFIELD_COLS + (targetCol - 1));
		specialAttackTargets_[targetCount++] = leftTargetIndex;
	}
	
	// 2. 对位
	int frontTargetIndex = isPlayerAttacking ? 
		(1 * BATTLEFIELD_COLS + targetCol) : 
		(2 * BATTLEFIELD_COLS + targetCol);
	specialAttackTargets_[targetCount++] = frontTargetIndex;
	
	// 3. 右斜对位
	if (targetCol < BATTLEFIELD_COLS - 1) {
		int rightTargetIndex = isPlayerAttacking ? 
			(1 * BATTLEFIELD_COLS + (targetCol + 1)) : 
			(2 * BATTLEFIELD_COLS + (targetCol + 1));
		specialAttackTargets_[targetCount++] = rightTargetIndex;
	}
}

// 设置双重攻击目标
void BattleState::setupTwiceTargets(int attackerIndex, int targetCol, bool isPlayerAttacking) {
	int targetIndex = isPlayerAttacking ? 
		(1 * BATTLEFIELD_COLS + targetCol) : 
		(2 * BATTLEFIELD_COLS + targetCol);
	
	// 同一个目标攻击两次
	specialAttackTargets_[0] = targetIndex;
	specialAttackTargets_[1] = targetIndex;
}

// 设置双向双重攻击目标
void BattleState::setupDoubleTwiceTargets(int attackerIndex, int targetCol, bool isPlayerAttacking) {
	int targetCount = 0;
	
	// 左斜对位攻击两次
	if (targetCol > 0) {
		int leftTargetIndex = isPlayerAttacking ? 
			(1 * BATTLEFIELD_COLS + (targetCol - 1)) : 
			(2 * BATTLEFIELD_COLS + (targetCol - 1));
		specialAttackTargets_[targetCount++] = leftTargetIndex;
		specialAttackTargets_[targetCount++] = leftTargetIndex;
	}
	
	// 右斜对位攻击两次
	if (targetCol < BATTLEFIELD_COLS - 1) {
		int rightTargetIndex = isPlayerAttacking ? 
			(1 * BATTLEFIELD_COLS + (targetCol + 1)) : 
			(2 * BATTLEFIELD_COLS + (targetCol + 1));
		specialAttackTargets_[targetCount++] = rightTargetIndex;
		specialAttackTargets_[targetCount++] = rightTargetIndex;
	}
}

// 设置三向双重攻击目标
void BattleState::setupTripleTwiceTargets(int attackerIndex, int targetCol, bool isPlayerAttacking) {
	int targetCount = 0;
	
	// 三向双重攻击顺序：左斜对位 → 对位 → 右斜对位（每个目标攻击两次）
	
	// 1. 左斜对位攻击两次
	if (targetCol > 0) {
		int leftTargetIndex = isPlayerAttacking ? 
			(1 * BATTLEFIELD_COLS + (targetCol - 1)) : 
			(2 * BATTLEFIELD_COLS + (targetCol - 1));
		specialAttackTargets_[targetCount++] = leftTargetIndex;
		specialAttackTargets_[targetCount++] = leftTargetIndex;
	}
	
	// 2. 对位攻击两次
	int frontTargetIndex = isPlayerAttacking ? 
		(1 * BATTLEFIELD_COLS + targetCol) : 
		(2 * BATTLEFIELD_COLS + targetCol);
	specialAttackTargets_[targetCount++] = frontTargetIndex;
	specialAttackTargets_[targetCount++] = frontTargetIndex;
	
	// 3. 右斜对位攻击两次
	if (targetCol < BATTLEFIELD_COLS - 1) {
		int rightTargetIndex = isPlayerAttacking ? 
			(1 * BATTLEFIELD_COLS + (targetCol + 1)) : 
			(2 * BATTLEFIELD_COLS + (targetCol + 1));
		specialAttackTargets_[targetCount++] = rightTargetIndex;
		specialAttackTargets_[targetCount++] = rightTargetIndex;
	}
}

// 执行斜对位攻击
void BattleState::executeDiagonalAttack(int attackerIndex, int targetCol, bool isPlayerAttacking, int damage) {
	// 攻击左斜对位
	if (targetCol > 0) {
		int leftTargetIndex = isPlayerAttacking ? 
			(1 * BATTLEFIELD_COLS + (targetCol - 1)) : 
			(2 * BATTLEFIELD_COLS + (targetCol - 1));
		attackTarget(attackerIndex, leftTargetIndex, damage);
	}
	
	// 攻击右斜对位
	if (targetCol < BATTLEFIELD_COLS - 1) {
		int rightTargetIndex = isPlayerAttacking ? 
			(1 * BATTLEFIELD_COLS + (targetCol + 1)) : 
			(2 * BATTLEFIELD_COLS + (targetCol + 1));
		attackTarget(attackerIndex, rightTargetIndex, damage);
	}
}

// 执行三向攻击
void BattleState::executeTripleAttack(int attackerIndex, int targetCol, bool isPlayerAttacking, int damage) {
	// 攻击对位
	int frontTargetIndex = isPlayerAttacking ? 
		(1 * BATTLEFIELD_COLS + targetCol) : 
		(2 * BATTLEFIELD_COLS + targetCol);
	attackTarget(attackerIndex, frontTargetIndex, damage);
	
	// 攻击左斜对位
	if (targetCol > 0) {
		int leftTargetIndex = isPlayerAttacking ? 
			(1 * BATTLEFIELD_COLS + (targetCol - 1)) : 
			(2 * BATTLEFIELD_COLS + (targetCol - 1));
		attackTarget(attackerIndex, leftTargetIndex, damage);
	}
	
	// 攻击右斜对位
	if (targetCol < BATTLEFIELD_COLS - 1) {
		int rightTargetIndex = isPlayerAttacking ? 
			(1 * BATTLEFIELD_COLS + (targetCol + 1)) : 
			(2 * BATTLEFIELD_COLS + (targetCol + 1));
		attackTarget(attackerIndex, rightTargetIndex, damage);
	}
}

// 执行双重攻击
void BattleState::executeTwiceAttack(int attackerIndex, int targetCol, bool isPlayerAttacking, int damage) {
	int targetIndex = isPlayerAttacking ? 
		(1 * BATTLEFIELD_COLS + targetCol) : 
		(2 * BATTLEFIELD_COLS + targetCol);
	
	// 第一次攻击
	attackTarget(attackerIndex, targetIndex, damage);
	
	// 第二次攻击（如果目标还活着）
	if (battlefield_[targetIndex].isAlive) {
		attackTarget(attackerIndex, targetIndex, damage);
	}
}

// 执行普通攻击
void BattleState::executeNormalAttack(int attackerIndex, int targetCol, bool isPlayerAttacking, int damage) {
	int targetIndex = isPlayerAttacking ? 
		(1 * BATTLEFIELD_COLS + targetCol) : 
		(2 * BATTLEFIELD_COLS + targetCol);
	
	attackTarget(attackerIndex, targetIndex, damage);
}

// 攻击目标
void BattleState::attackTarget(int attackerIndex, int targetIndex, int damage) {
	if (targetIndex < 0 || targetIndex >= TOTAL_BATTLEFIELD_SLOTS) return;
	
	const auto& attacker = battlefield_[attackerIndex];
	const auto& target = battlefield_[targetIndex];
	
	// 检查攻击者是否有空袭印记
	bool hasAirStrike = hasMark(attacker.card, u8"空袭");
	
	// 检查目标是否有效
	if (!target.isAlive) {
		// 目标已死亡，攻击敌方本体
		if (attacker.isPlayer) {
			// 玩家攻击敌方本体
			enemyHealth_ -= damage;
			if (enemyHealth_ < 0) enemyHealth_ = 0;
		} else {
			// 敌方攻击玩家本体
			playerHealth_ -= damage;
			if (playerHealth_ < 0) playerHealth_ = 0;
		}
		return;
	}
	if (attacker.isPlayer == target.isPlayer) return; // 不能攻击友军
	
	// 空袭机制处理
	if (hasAirStrike) {
		// 检查对位是否有高跳印记
		bool targetHasHighJump = hasMark(target.card, u8"高跳");
		
		if (!targetHasHighJump) {
			// 对位没有高跳印记，空袭直接攻击本体
			if (attacker.isPlayer) {
				// 玩家空袭攻击敌方本体
				enemyHealth_ -= damage;
				if (enemyHealth_ < 0) enemyHealth_ = 0;
			} else {
				// 敌方空袭攻击玩家本体
				playerHealth_ -= damage;
				if (playerHealth_ < 0) playerHealth_ = 0;
			}
			return;
		}
		// 对位有高跳印记，空袭攻击对位的高跳卡牌
		// 计算穿透伤害：如果攻击力大于对位血量，多余伤害穿透到第一行高跳卡牌
		int remainingDamage = damage - target.health;
		if (remainingDamage > 0) {
			// 有穿透伤害，检查第一行是否有高跳卡牌
			if (attacker.isPlayer) {
				// 玩家攻击：检查敌方第一行（row 0）是否有高跳卡牌
				int firstRowTargetIndex = targetIndex - BATTLEFIELD_COLS; // 第一行对应位置
				if (firstRowTargetIndex >= 0 && firstRowTargetIndex < BATTLEFIELD_COLS) {
					const auto& firstRowTarget = battlefield_[firstRowTargetIndex];
					if (firstRowTarget.isAlive && hasMark(firstRowTarget.card, u8"高跳")) {
						// 第一行有高跳卡牌，承受穿透伤害
						battlefield_[firstRowTargetIndex].health -= remainingDamage;
						if (battlefield_[firstRowTargetIndex].health <= 0) {
							battlefield_[firstRowTargetIndex].isAlive = false;
							cardsToDestroy_.push_back(firstRowTargetIndex);
							isDestroyAnimating_ = true;
							destroyAnimTime_ = 0.0f;
						}
					}
				}
			} else {
				// 敌方攻击：检查玩家第一行（row 2）是否有高跳卡牌
				int firstRowTargetIndex = targetIndex + BATTLEFIELD_COLS; // 第一行对应位置
				if (firstRowTargetIndex >= 2 * BATTLEFIELD_COLS && firstRowTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
					const auto& firstRowTarget = battlefield_[firstRowTargetIndex];
					if (firstRowTarget.isAlive && hasMark(firstRowTarget.card, u8"高跳")) {
						// 第一行有高跳卡牌，承受穿透伤害
						battlefield_[firstRowTargetIndex].health -= remainingDamage;
						if (battlefield_[firstRowTargetIndex].health <= 0) {
							battlefield_[firstRowTargetIndex].isAlive = false;
							cardsToDestroy_.push_back(firstRowTargetIndex);
							isDestroyAnimating_ = true;
							destroyAnimTime_ = 0.0f;
						}
					}
				}
			}
		}
	}
	
	// 执行正常攻击
	battlefield_[targetIndex].health -= damage;
	
	// 检查目标是否死亡
	if (battlefield_[targetIndex].health <= 0) {
		battlefield_[targetIndex].isAlive = false;
		cardsToDestroy_.push_back(targetIndex);
		isDestroyAnimating_ = true;
		destroyAnimTime_ = 0.0f;
	}
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
	
	// 显示魂骨数量（战斗区域和道具区域之间，白色骨头图案）
	if (boneCount_ > 0) {
		int boneSize = 20; // 骨头图案大小
		int boneSpacing = 25; // 骨头间距
		int columnSpacing = 20; // 列间距
		int maxBonesPerColumn = 8; // 每列最多5个
		
		int startX = itemAreaRect_.x + itemAreaRect_.w + 30; // 道具区域右侧+30像素，增加间距
		
		// 计算安全区域：在道具区域下方和战斗区域上方之间
		int safeTop = itemAreaRect_.y + itemAreaRect_.h - 200; // 道具区域下方+10像素（向上移动）
		int safeBottom = battlefieldRect_.y - 30; // 战斗区域上方-30像素（向上移动）
		int safeHeight = safeBottom - safeTop;
		int bonesInFirstColumn = std::min(boneCount_, maxBonesPerColumn);
		int totalBoneHeight = bonesInFirstColumn * boneSpacing;
		
		// 确保不超出安全区域
		int startY;
		if (totalBoneHeight <= safeHeight) {
			startY = safeTop + (safeHeight - totalBoneHeight) / 2; // 在安全区域内居中
		} else {
			startY = safeTop; // 如果太高，从顶部开始
		}
		
		// 计算需要的列数
		int columns = (boneCount_ + maxBonesPerColumn - 1) / maxBonesPerColumn;
		int totalWidth = columns * (boneSize + columnSpacing) - columnSpacing;
		
		/*// 绘制魂骨背景框
		SDL_SetRenderDrawColor(r, 50, 50, 50, 150);
		SDL_Rect boneBg{startX - 10, startY - 10, totalWidth + 20, totalBoneHeight + 20};
		SDL_RenderFillRect(r, &boneBg);
		SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
		SDL_RenderDrawRect(r, &boneBg);*/
		
		// 绘制白色骨头图案
		for (int i = 0; i < boneCount_; ++i) {
			int column = i / maxBonesPerColumn;
			int row = i % maxBonesPerColumn;
			int x = startX + column * (boneSize + columnSpacing);
			int y = startY + row * boneSpacing;
			
			// 绘制骨头图案（白色）
			SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
			
			// 骨头主体（椭圆形）
			SDL_Rect boneBody{x + 2, y + 4, boneSize - 4, boneSize - 8};
			SDL_RenderFillRect(r, &boneBody);
			
			// 骨头两端（圆形）
			SDL_Rect boneEnd1{x, y + 2, 6, 6};
			SDL_Rect boneEnd2{x + boneSize - 6, y + 2, 6, 6};
			SDL_Rect boneEnd3{x, y + boneSize - 8, 6, 6};
			SDL_Rect boneEnd4{x + boneSize - 6, y + boneSize - 8, 6, 6};
			
			SDL_RenderFillRect(r, &boneEnd1);
			SDL_RenderFillRect(r, &boneEnd2);
			SDL_RenderFillRect(r, &boneEnd3);
			SDL_RenderFillRect(r, &boneEnd4);
			
			// 骨头边框
			SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
			SDL_RenderDrawRect(r, &boneBody);
			SDL_RenderDrawRect(r, &boneEnd1);
			SDL_RenderDrawRect(r, &boneEnd2);
			SDL_RenderDrawRect(r, &boneEnd3);
			SDL_RenderDrawRect(r, &boneEnd4);
		}
	}
	
	
	// 显示敌人生命值
	if (enemyFont_) {
		SDL_Color healthColor{255, 100, 100, 255}; // 红色
		std::string healthText = "敌人生命值: " + std::to_string(enemyHealth_);
		SDL_Surface* healthSurface = TTF_RenderUTF8_Blended(enemyFont_, healthText.c_str(), healthColor);
		if (healthSurface) {
			SDL_Texture* healthTexture = SDL_CreateTextureFromSurface(r, healthSurface);
			if (healthTexture) {
				SDL_Rect healthRect{enemyAreaRect_.x + 10, enemyAreaRect_.y + 50, healthSurface->w, healthSurface->h};
				SDL_RenderCopy(r, healthTexture, nullptr, &healthRect);
				SDL_DestroyTexture(healthTexture);
			}
			SDL_FreeSurface(healthSurface);
		}
		
		// 显示玩家生命值
		SDL_Color playerHealthColor{100, 255, 100, 255}; // 绿色
		std::string playerHealthText = "玩家生命值: " + std::to_string(playerHealth_);
		SDL_Surface* playerHealthSurface = TTF_RenderUTF8_Blended(enemyFont_, playerHealthText.c_str(), playerHealthColor);
		if (playerHealthSurface) {
			SDL_Texture* playerHealthTexture = SDL_CreateTextureFromSurface(r, playerHealthSurface);
			if (playerHealthTexture) {
				SDL_Rect playerHealthRect{enemyAreaRect_.x + 10, enemyAreaRect_.y + 80, playerHealthSurface->w, playerHealthSurface->h};
				SDL_RenderCopy(r, playerHealthTexture, nullptr, &playerHealthRect);
				SDL_DestroyTexture(playerHealthTexture);
			}
			SDL_FreeSurface(playerHealthSurface);
		}
	}
	
	// 显示伤害数值（在敌人区域中央）
	if (showDamage_ && totalDamageDealt_ > 0 && enemyFont_) {
		SDL_Color damageColor{255, 255, 0, 255}; // 黄色
		std::string damageText = "-" + std::to_string(totalDamageDealt_);
		SDL_Surface* damageSurface = TTF_RenderUTF8_Blended(enemyFont_, damageText.c_str(), damageColor);
		if (damageSurface) {
			SDL_Texture* damageTexture = SDL_CreateTextureFromSurface(r, damageSurface);
			if (damageTexture) {
				// 在敌人区域中央显示伤害
				SDL_Rect damageRect{enemyAreaRect_.x + enemyAreaRect_.w / 2 - damageSurface->w / 2, 
									enemyAreaRect_.y + enemyAreaRect_.h / 2 - damageSurface->h / 2, 
									damageSurface->w, damageSurface->h};
				
				// 绘制半透明背景
				SDL_SetRenderDrawColor(r, 0, 0, 0, 150);
				SDL_Rect damageBg{damageRect.x - 10, damageRect.y - 5, damageRect.w + 20, damageRect.h + 10};
				SDL_RenderFillRect(r, &damageBg);
				
				// 绘制伤害数值
				SDL_RenderCopy(r, damageTexture, nullptr, &damageRect);
				SDL_DestroyTexture(damageTexture);
			}
			SDL_FreeSurface(damageSurface);
		}
	}
	
	// 显示上帝模式状态
	if (godMode_ && enemyFont_) {
		SDL_Color godModeColor{255, 255, 0, 255}; // 黄色
		std::string godModeText = "上帝模式 - 悬停敌方区域按A键生成守宫，按S键生成雪尾鼬生";
		SDL_Surface* godModeSurface = TTF_RenderUTF8_Blended(enemyFont_, godModeText.c_str(), godModeColor);
		if (godModeSurface) {
			SDL_Texture* godModeTexture = SDL_CreateTextureFromSurface(r, godModeSurface);
			if (godModeTexture) {
				SDL_Rect godModeRect{enemyAreaRect_.x + 10, enemyAreaRect_.y + 110, godModeSurface->w, godModeSurface->h};
				SDL_RenderCopy(r, godModeTexture, nullptr, &godModeRect);
				SDL_DestroyTexture(godModeTexture);
			}
			SDL_FreeSurface(godModeSurface);
		}
	}
	
	// 显示状态消息（在敌人区域）
	if (!statusMessage_.empty() && enemyFont_) {
		SDL_Color statusColor{255, 255, 255, 255}; // 白色文字
		SDL_Surface* statusSurface = TTF_RenderUTF8_Blended(enemyFont_, statusMessage_.c_str(), statusColor);
		if (statusSurface) {
			SDL_Texture* statusTexture = SDL_CreateTextureFromSurface(r, statusSurface);
			if (statusTexture) {
				// 在敌人区域下方显示状态消息
				SDL_Rect statusRect{enemyAreaRect_.x + enemyAreaRect_.w / 2 - statusSurface->w / 2, 
								   enemyAreaRect_.y + enemyAreaRect_.h - 100, 
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