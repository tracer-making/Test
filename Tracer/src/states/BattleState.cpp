#include "BattleState.h"
#include "../core/EngraveStore.h"
#include "TestState.h"
#include "MapExploreState.h"
#include "../core/App.h"
#include "../core/Deck.h"

#include "../core/Cards.h"
#include <SDL.h>

#include <SDL_ttf.h>
#include <cmath>
#include <cstdlib>
#include <ctime>

// 可获得的道具列表定义
const std::vector<std::string> BattleState::AVAILABLE_ITEMS = {
	"yinyang_pei", "mobao_ping", "wangchuan_shi", "fengya_shan", "rigui",
	"duanyinjian", "tunmohao", "fuhunsuo", "wuzitianshu", "xuanmuping",
	"tianjiluopan", "sanguiping"
};

#include <algorithm>
#include <sstream>
#include <string>
#include "../core/WenMaiStore.h"

BattleState::BattleState() = default;


BattleState::~BattleState() {
	if (titleFont_) TTF_CloseFont(titleFont_);
	if (cardNameFont_) TTF_CloseFont(cardNameFont_);
	if (cardStatFont_) TTF_CloseFont(cardStatFont_);
	if (infoFont_) TTF_CloseFont(infoFont_);
	if (enemyFont_) TTF_CloseFont(enemyFont_);
	if (backButton_) delete backButton_;
}

void BattleState::grantGravediggerBones(bool countEnemySide) {
    int count = 0;
    for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
        if (!battlefield_[i].isAlive) continue;
        bool isEnemy = !battlefield_[i].isPlayer;
        if (countEnemySide != isEnemy) continue; // 仅统计指定一侧
        if (hasMark(battlefield_[i].card, std::string(u8"掘墓人"))) {
            count += 1;
        }
    }
    if (count > 0) {
        boneCount_ += count;
        statusMessage_ = (countEnemySide ? "掘墓人(敌方)：回合结束+" : "掘墓人(我方)：回合结束+")
            + std::to_string(count) + " 魂骨（总计 " + std::to_string(boneCount_) + ")";
    }
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
		backButton_->setRect({ 20, 20, 100, 35 });
		backButton_->setText("返回");
		if (infoFont_) backButton_->setFont(infoFont_, app.getRenderer());
		backButton_->setOnClick([this]() {
			pendingGoMapExplore_ = true;
			});
	}

	// 移除结束回合按钮，功能移到砚台上

	// 移除抽牌按钮，改为检测牌堆点击
}

void BattleState::onExit(App& app) {
	// 更新牌库中卡牌的数值变化（如蛇自环的永久加成）
	auto& store = DeckStore::instance();
	for (const auto& update : pendingCardUpdates_) {
		const std::string& instanceId = update.first;
		int newAttack = update.second.first;
		int newHealth = update.second.second;
		
		for (auto& card : store.library()) {
			if (card.instanceId == instanceId) {
				std::cout << "[BATTLE] Updating library card on exit: " << card.name << " (ID:" << card.id << ", Instance:" << card.instanceId << ") Attack:" << card.attack << "->" << newAttack << " Health:" << card.health << "->" << newHealth << std::endl;
				card.attack = newAttack;
				card.health = newHealth;
				break;
			}
		}
	}
	
	// 清理待更新记录
	pendingCardUpdates_.clear();
	
	// 清理资源
	// 注意：不重置全局牌库，因为战斗只是使用牌库，不应该修改它
}

void BattleState::handleEvent(App& app, const SDL_Event& e) {
	// 处理按钮事件
	if (backButton_) backButton_->handleEvent(e);

	// 处理键盘事件
	if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_t) {
			// T键切换上帝模式
			godMode_ = !godMode_;
				// 进入/退出上帝模式时不再输出到状态消息，由渲染侧显示帮助
		}
        else if (e.key.keysym.sym == SDLK_h && godMode_) {
            // H键：锁定玩家本体血量与墨尺计数
            lockPlayerHealth_ = !lockPlayerHealth_;
            if (lockPlayerHealth_) {
                lockedPlayerHealthValue_ = playerHealth_;
                lockedMeterNetValue_ = meterNet_;
                lockedMeterDisplayPos_ = meterDisplayPos_;
                lockedMeterActual_ = meterActualPos_;
                lockedMeterTarget_ = meterTargetPos_;
				// 不输出状态消息
            } else {
				// 不输出状态消息
            }
        }
        else if (e.key.keysym.sym == SDLK_j && godMode_) {
            // J键：魂骨+1
            boneCount_ += 1;
			// 不输出状态消息
        }
        else if (e.key.keysym.sym == SDLK_a && godMode_) {
            // A键在悬停位置生成狼戎酋首
			if (hoveredBattlefieldIndex_ >= 0 && hoveredBattlefieldIndex_ < TOTAL_BATTLEFIELD_SLOTS) {
				int row = hoveredBattlefieldIndex_ / BATTLEFIELD_COLS;
				if (row < 2) { // 只能在敌方区域（前两行）生成
					if (!battlefield_[hoveredBattlefieldIndex_].isAlive) {
                        // 生成巴蛇卡牌
                        Card bashe = CardDB::instance().make("bashe");
                        if (!bashe.id.empty()) {
                            battlefield_[hoveredBattlefieldIndex_].card = bashe;
							battlefield_[hoveredBattlefieldIndex_].isAlive = true;
                            battlefield_[hoveredBattlefieldIndex_].health = bashe.health;
							battlefield_[hoveredBattlefieldIndex_].isPlayer = false; // 敌方卡牌
							battlefield_[hoveredBattlefieldIndex_].placedTurn = currentTurn_;
							battlefield_[hoveredBattlefieldIndex_].oneTurnGrowthApplied = false;
							// 不输出状态消息
						}
					}
					else {
						// 不输出状态消息
					}
				}
				else {
					// 不输出状态消息
				}
			}
			else {
				// 不输出状态消息
			}
		}
        else if (e.key.keysym.sym == SDLK_s && godMode_) {
            // S键在悬停位置生成刀笔吏
			if (hoveredBattlefieldIndex_ >= 0 && hoveredBattlefieldIndex_ < TOTAL_BATTLEFIELD_SLOTS) {
				int row = hoveredBattlefieldIndex_ / BATTLEFIELD_COLS;
				if (row < 2) { // 只能在敌方区域（前两行）生成
					if (!battlefield_[hoveredBattlefieldIndex_].isAlive) {
                        // 生成刀笔吏卡牌（三向攻击）
                        Card daobi = CardDB::instance().make("daobi_li");
                        if (!daobi.id.empty()) {
                            battlefield_[hoveredBattlefieldIndex_].card = daobi;
							battlefield_[hoveredBattlefieldIndex_].isAlive = true;
                            battlefield_[hoveredBattlefieldIndex_].health = daobi.health;
							battlefield_[hoveredBattlefieldIndex_].isPlayer = false; // 敌方卡牌
							battlefield_[hoveredBattlefieldIndex_].placedTurn = currentTurn_;
							battlefield_[hoveredBattlefieldIndex_].oneTurnGrowthApplied = false;
                            // 不输出状态消息
						}
					}
					else {
						// 不输出状态消息
					}
				}
				else {
					// 不输出状态消息
				}
			}
			else {
				// 不输出状态消息
			}
		}
        else if (e.key.keysym.sym == SDLK_d && godMode_) {
            // D键在悬停位置生成铁兽夹
			if (hoveredBattlefieldIndex_ >= 0 && hoveredBattlefieldIndex_ < TOTAL_BATTLEFIELD_SLOTS) {
				int row = hoveredBattlefieldIndex_ / BATTLEFIELD_COLS;
				if (row < 2) { // 只能在敌方区域（前两行）生成
					if (!battlefield_[hoveredBattlefieldIndex_].isAlive) {
                        // 生成铁兽夹卡牌
                        Card tieshou = CardDB::instance().make("tieshou_jia");
                        if (!tieshou.id.empty()) {
                            battlefield_[hoveredBattlefieldIndex_].card = tieshou;
							battlefield_[hoveredBattlefieldIndex_].isAlive = true;
                            battlefield_[hoveredBattlefieldIndex_].health = tieshou.health;
							battlefield_[hoveredBattlefieldIndex_].isPlayer = false; // 敌方卡牌
							// 不输出状态消息
						}
					}
					else {
						// 不输出状态消息
					}
				}
				else {
					// 不输出状态消息
				}
			}
			else {
				// 不输出状态消息
			}
		}
		else if (e.key.keysym.sym == SDLK_f && godMode_) {
            // F键在悬停位置生成全向打击
			if (hoveredBattlefieldIndex_ >= 0 && hoveredBattlefieldIndex_ < TOTAL_BATTLEFIELD_SLOTS) {
				int row = hoveredBattlefieldIndex_ / BATTLEFIELD_COLS;
				if (row < 2) { // 只能在敌方区域（前两行）生成
					if (!battlefield_[hoveredBattlefieldIndex_].isAlive) {
                        // 生成全向打击卡牌
                        Card quanxiang = CardDB::instance().make("quanxiang_daji");
                        if (!quanxiang.id.empty()) {
                            battlefield_[hoveredBattlefieldIndex_].card = quanxiang;
							battlefield_[hoveredBattlefieldIndex_].isAlive = true;
                            battlefield_[hoveredBattlefieldIndex_].health = quanxiang.health;
							battlefield_[hoveredBattlefieldIndex_].isPlayer = false; // 敌方卡牌
							// 不输出状态消息
						}
					}
					else {
						// 不输出状态消息
					}
				}
				else {
					// 不输出状态消息
				}
			}
			else {
				// 不输出状态消息
			}
		}
		else if (e.key.keysym.sym == SDLK_b && godMode_) {
            // B键：获得一个随机道具（前提是道具没满）
            if (playerItems_.size() < MAX_ITEMS) {
                // 获取随机道具
                std::string selectedItem = getRandomItem();
                if (!selectedItem.empty()) {
                    addItem(selectedItem, 1);
					// 不输出状态消息
                } else {
					// 不输出状态消息
                }
            } else {
				// 不输出状态消息
            }
        }
		else if (e.key.keysym.sym == SDLK_v && godMode_) {
			// V键：打开检索牌界面
			isSearchingDeck_ = true;
			selectedDeckCardIndex_ = -1;
			searchAllCardsMode_ = false;
			// 不输出状态消息
		}
		else if (e.key.keysym.sym == SDLK_n && godMode_) {
			// N键：打开全卡库检索
			isSearchingDeck_ = true;
			selectedDeckCardIndex_ = -1;
			searchAllCardsMode_ = true;
			searchCandidates_ = CardDB::instance().allIds();
			// 不输出状态消息
		}
		else if (e.key.keysym.sym == SDLK_g && godMode_) {
			// G键：与ASDF生成一致，要求敌方区域(前两行)且悬停格为空，随后全卡库选择生成到该悬停格
			if (hoveredBattlefieldIndex_ >= 0 && hoveredBattlefieldIndex_ < TOTAL_BATTLEFIELD_SLOTS) {
				int row = hoveredBattlefieldIndex_ / BATTLEFIELD_COLS;
				if (row < 2 && !battlefield_[hoveredBattlefieldIndex_].isAlive) {
					godSpawnMode_ = true;
					godSpawnIndex_ = hoveredBattlefieldIndex_;
					isSearchingDeck_ = true;
					selectedDeckCardIndex_ = -1;
					searchAllCardsMode_ = true;
					searchCandidates_ = CardDB::instance().allIds();
				}
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
		
		// 重置道具悬停状态
		for (int i = 0; i < MAX_ITEMS; ++i) {
			isItemHovered_[i] = false;
		}
		hoveredItemIndex_ = -1;

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
		for (size_t i = 0; i < handCardRects_.size() && i < handCards_.size(); ++i) {
			const auto& rect = handCardRects_[i];
			if (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
				mouseY >= rect.y && mouseY <= rect.y + rect.h) {
				hoveredHandCardIndex_ = static_cast<int>(i);
				break;
			}
		}
		
		// 检查道具悬停
		for (int i = 0; i < MAX_ITEMS; ++i) {
			if (mouseX >= itemSlots_[i].x && mouseX <= itemSlots_[i].x + itemSlots_[i].w &&
				mouseY >= itemSlots_[i].y && mouseY <= itemSlots_[i].y + itemSlots_[i].h) {
				isItemHovered_[i] = true;
				hoveredItemIndex_ = i;
				break;
			}
		}
	}
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		int mouseX = e.button.x;
		int mouseY = e.button.y;

		// 检索状态：处理牌堆/全卡库卡牌选择
		if (isSearchingDeck_) {
			// 计算牌堆卡牌的显示区域 - 与渲染逻辑保持一致
			int cardWidth = searchAllCardsMode_ ? 90 : 120;   // 全卡库更小
			int cardHeight = searchAllCardsMode_ ? 135 : 180; // 全卡库更小
			int cardSpacing = searchAllCardsMode_ ? 10 : 15;  // 全卡库更紧凑
			int rowSpacing = searchAllCardsMode_ ? 14 : 20;   // 全卡库更紧凑
			
			auto& store = DeckStore::instance();
			int totalCards = static_cast<int>(searchAllCardsMode_ ? searchCandidates_.size() : store.library().size());
			if (totalCards == 0) return;
			
			// 根据屏幕宽度和卡牌数量动态计算每行卡牌数
			int maxCardsPerRow = (screenW_ - 100) / (cardWidth + cardSpacing);
			int cardsPerRow = std::min(maxCardsPerRow, totalCards);
			if (cardsPerRow < 1) cardsPerRow = 1;
			
			// 计算总行数
			int totalRows = (totalCards + cardsPerRow - 1) / cardsPerRow;
			
			// 计算每行实际卡牌数（最后一行可能不满）
			int cardsInLastRow = totalCards % cardsPerRow;
			if (cardsInLastRow == 0) cardsInLastRow = cardsPerRow;
			
			// 计算整体布局的起始位置（水平和垂直居中）
			int totalLayoutHeight = totalRows * cardHeight + (totalRows - 1) * rowSpacing;
			int startY = (screenH_ - totalLayoutHeight) / 2;
			
			for (size_t i = 0; i < (searchAllCardsMode_ ? searchCandidates_.size() : store.library().size()); ++i) {
				int row = static_cast<int>(i) / cardsPerRow;
				int col = static_cast<int>(i) % cardsPerRow;
				
				// 计算当前行的卡牌数
				int cardsInCurrentRow = (row == totalRows - 1) ? cardsInLastRow : cardsPerRow;
				
				// 计算当前行的起始位置（居中）
				int currentRowWidth = cardsInCurrentRow * cardWidth + (cardsInCurrentRow - 1) * cardSpacing;
				int currentRowStartX = (screenW_ - currentRowWidth) / 2;
				
				int cardX = currentRowStartX + col * (cardWidth + cardSpacing);
				int cardY = startY + row * (cardHeight + rowSpacing);
				
				if (mouseX >= cardX && mouseX <= cardX + cardWidth &&
					mouseY >= cardY && mouseY <= cardY + cardHeight) {
					// 选中这张卡牌
					selectedDeckCardIndex_ = static_cast<int>(i);
					
					// 确定选中的卡ID
					std::string selectedCardId = searchAllCardsMode_ ? searchCandidates_[i] : store.library()[i].id;
					// 仅当非全卡库模式时，从玩家牌堆移除该卡
					if (!searchAllCardsMode_) {
						store.library().erase(store.library().begin() + i);
					}
					
					// 生成卡对象
					Card selectedCard = CardDB::instance().make(selectedCardId);
					if (!selectedCard.id.empty()) {
						if (godSpawnMode_ && godSpawnIndex_ >= 0 && godSpawnIndex_ < TOTAL_BATTLEFIELD_SLOTS) {
							// 上帝模式生成到指定格（前两行视为敌方）
							SDL_Rect keepRect = battlefield_[godSpawnIndex_].rect;
							int rowIdx = godSpawnIndex_ / BATTLEFIELD_COLS;
							bool enemySide = (rowIdx < 2);
							battlefield_[godSpawnIndex_].card = selectedCard;
							battlefield_[godSpawnIndex_].isAlive = true;
							battlefield_[godSpawnIndex_].health = selectedCard.health;
							battlefield_[godSpawnIndex_].isPlayer = !enemySide ? true : false;
							battlefield_[godSpawnIndex_].rect = keepRect;
							battlefield_[godSpawnIndex_].placedTurn = currentTurn_;
							battlefield_[godSpawnIndex_].oneTurnGrowthApplied = false;
							statusMessage_ = std::string("上帝生成：") + selectedCard.name;
						} else {
							// 默认行为：加入手牌
							Card card = selectedCard;
							// 意境系统：应用已组合的意境效果到卡牌
							EngraveStore::instance().applyToCard(card);
							handCards_.push_back(card);
							layoutHandCards();
							statusMessage_ = std::string("检索：获得手牌 ") + card.name;
						}
					}
					
					// 退出检索/上帝生成状态
					isSearchingDeck_ = false;
					selectedDeckCardIndex_ = -1;
					searchAllCardsMode_ = false;
					godSpawnMode_ = false;
					godSpawnIndex_ = -1;
					return;
				}
			}
			return; // 检索状态下不处理其他点击
		}

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
			}
			else if (hasDrawnThisTurn_) {
				statusMessage_ = "本回合已抽过牌！";
			}
			else if (inkPileCount_ > 0) {
				Card inkCard = CardDB::instance().make("moding");
				if (!inkCard.id.empty()) {
					// 意境系统：应用已组合的意境效果到卡牌
					EngraveStore::instance().applyToCard(inkCard);
					// 随机印记效果：加入手牌时，删去随机印记并添加任意一个印记
					applyRandomMarkEffect(inkCard);
					handCards_.push_back(inkCard);
					inkPileCount_--;
					hasDrawnThisTurn_ = true;
					mustDrawThisTurn_ = false; // 抽牌后取消强制抽牌要求
					layoutHandCards();
					statusMessage_ = "从墨锭牌堆抽到：" + inkCard.name;
				}
			}
			else {
				statusMessage_ = "墨锭牌堆已空！";
			}
		}
		else if (mouseX >= playerPileRect_.x && mouseX <= playerPileRect_.x + playerPileRect_.w &&
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
			}
			else if (hasDrawnThisTurn_) {
				statusMessage_ = "本回合已抽过牌！";
			}
			else {
				// 从战斗牌堆中随机抽取一张牌
				if (!battleDeck_.empty()) {
					// 随机选择一张牌
					int randomIndex = rand() % battleDeck_.size();
					Card newCard = battleDeck_[randomIndex];
					// 从战斗牌堆中移除
					battleDeck_.erase(battleDeck_.begin() + randomIndex);
					// 应用意境刻画（根据卡的category给予绑定的印记）
					EngraveStore::instance().applyToCard(newCard);
					// 随机印记效果：加入手牌时，删去随机印记并添加任意一个印记
					applyRandomMarkEffect(newCard);
					handCards_.push_back(newCard);
					auto& store = DeckStore::instance();
					store.hand().push_back(newCard);
					hasDrawnThisTurn_ = true;
					mustDrawThisTurn_ = false; // 抽牌后取消强制抽牌要求
					layoutHandCards();
					statusMessage_ = "从玩家牌堆抽到：" + newCard.name;
				} else {
				statusMessage_ = "玩家牌堆已空！";
				}
			}
		}
		else if (mouseX >= inkStoneRect_.x && mouseX <= inkStoneRect_.x + inkStoneRect_.w &&
			mouseY >= inkStoneRect_.y && mouseY <= inkStoneRect_.y + inkStoneRect_.h) {
			// 点击砚台（结束回合）
			if (currentPhase_ == GamePhase::PlayerTurn) {
				// 检查是否必须抽牌
				if (mustDrawThisTurn_) {
					statusMessage_ = "必须先抽牌才能结束回合！";
				}
				else if (isSacrificing_) {
					statusMessage_ = "献祭模式中，请点击场上卡牌献祭或右键取消！";
				}
				else if (showSacrificeInk_ || selectedHandCard_ >= 0) {
					statusMessage_ = "请先打出选中的卡牌！";
				}
				else {
					endTurn();
				}
			}
		}

		// 检查手牌点击
		for (size_t i = 0; i < handCardRects_.size() && i < handCards_.size(); ++i) {
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
					}
					else {
						isSacrificing_ = false;
						statusMessage_ = "已选择手牌：" + handCards_[i].name + "（消耗 " + std::to_string(boneCost) + " 个魂骨）";
					}
				}
				else if (handCards_[i].sacrificeCost > 0) {
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
				}
				else {
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

					// 断因剑目标选择模式
					if (isSelectingTarget_) {
						// 只能选择敌人第二行（第一行，row=1）的卡牌
						if (row == 1 && battlefield_[i].isAlive && !battlefield_[i].isPlayer) {
							// 选中目标，直接摧毁
							selectedTargetIndex_ = i;
							// 摧毁卡牌
							battlefield_[i].isAlive = false;
							battlefield_[i].health = 0;
							// 退出选择模式
							isSelectingTarget_ = false;
							statusMessage_ = u8"断因剑摧毁了 " + battlefield_[i].card.name + "！";
						} else {
							statusMessage_ = u8"断因剑只能选择敌人第二行的卡牌！";
						}
						return;
					}

					// 吞墨毫目标选择模式
					if (isSelectingTunmohaoTarget_) {
						// 只能选择敌人第二行（第一行，row=1）的卡牌
						if (row == 1 && battlefield_[i].isAlive && !battlefield_[i].isPlayer) {
							// 选中目标，直接摧毁
							selectedTunmohaoTargetIndex_ = i;
							// 摧毁卡牌
							battlefield_[i].isAlive = false;
							battlefield_[i].health = 0;
							// 退出选择模式
							isSelectingTunmohaoTarget_ = false;
							// 手牌获得一张狼皮
							Card langpiCard = CardDB::instance().make("langpi");
							if (langpiCard.id != "unknown") {
								EngraveStore::instance().applyToCard(langpiCard);
								handCards_.push_back(langpiCard);
								layoutHandCards();
								statusMessage_ = u8"吞墨毫摧毁了 " + battlefield_[i].card.name + u8"！获得狼皮！";
							} else {
								statusMessage_ = u8"吞墨毫摧毁了 " + battlefield_[i].card.name + u8"！但无法获得狼皮！";
							}
						} else {
							statusMessage_ = u8"吞墨毫只能选择敌人第二行的卡牌！";
						}
						return;
					}

					// 缚魂锁目标选择模式
					if (isSelectingFuhunsuoTarget_) {
						// 只能选择敌人第二行（第一行，row=1）的卡牌
						if (row == 1 && battlefield_[i].isAlive && !battlefield_[i].isPlayer) {
							// 检查对应的我方第三行位置是否为空
							int targetCol = i % BATTLEFIELD_COLS;
							int playerTargetIndex = 2 * BATTLEFIELD_COLS + targetCol; // 我方第三行对应位置
							
							if (playerTargetIndex >= 0 && playerTargetIndex < TOTAL_BATTLEFIELD_SLOTS && 
								!battlefield_[playerTargetIndex].isAlive) {
								// 目标位置为空，可以移动
								selectedFuhunsuoTargetIndex_ = i;
								
								// 保存原卡牌信息
								Card originalCard = battlefield_[i].card;
								
								// 设置动画参数
								fuhunsuoFromIndex_ = i;
								fuhunsuoToIndex_ = playerTargetIndex;
								fuhunsuoFromRect_ = battlefield_[i].rect;
								fuhunsuoToRect_ = battlefield_[playerTargetIndex].rect;
								
                                // 从敌人位置移除（因移动而死亡，不获得魂骨）
                                battlefield_[i].isAlive = false;
                                battlefield_[i].isMovedToDeath = true;
								battlefield_[i].health = 0;
								
								// 设置目标位置（临时设置为存活，用于动画显示）
								battlefield_[playerTargetIndex].card = originalCard;
								battlefield_[playerTargetIndex].isAlive = true; // 临时设置为存活，用于动画显示
								battlefield_[playerTargetIndex].isPlayer = true; // 变为玩家控制
								battlefield_[playerTargetIndex].health = originalCard.health;
								
								// 开始动画
								isFuhunsuoAnimating_ = true;
								fuhunsuoAnimTime_ = 0.0f;
								
								// 退出选择模式
								isSelectingFuhunsuoTarget_ = false;
								statusMessage_ = u8"缚魂锁将 " + originalCard.name + " 移动到我方！";
							} else {
								statusMessage_ = u8"缚魂锁：对应的我方第三行位置不是空位！";
							}
						} else {
							statusMessage_ = u8"缚魂锁只能选择敌人第二行的卡牌！";
						}
						return;
					}

					if (isSacrificing_) {
						// Sacrifice mode: click battlefield cards to sacrifice

						if (battlefield_[i].isAlive && battlefield_[i].isPlayer && !battlefield_[i].isSacrificed) {
							// 献祭这张卡牌（点击就加点数，出现墨滴，标记为已献祭）


							// 计算献祭获得的墨量（检查优质祭品印记）
							int inkGained = battlefield_[i].card.sacrificeCost;
							if (hasMark(battlefield_[i].card, u8"优质祭品")) {
								inkGained = 3; // 优质祭品提供3点墨量
								currentSacrificeInk_ += inkGained;
								statusMessage_ = "优质祭品！获得3点墨量，当前：" + std::to_string(currentSacrificeInk_) + "/" + std::to_string(sacrificeTargetCost_);
							}
							else {
								currentSacrificeInk_++;
								statusMessage_ = "献祭获得 " + std::to_string(inkGained) + " 点墨量，当前：" + std::to_string(currentSacrificeInk_) + "/" + std::to_string(sacrificeTargetCost_);
							}


							battlefield_[i].isSacrificed = true; // 标记为已献祭

							// 检查是否献祭足够
							if (currentSacrificeInk_ >= sacrificeTargetCost_) {
								// 献祭完成，增加献祭次数
								sacrificeCountThisTurn_++;
								
								// 献祭完成，开始摧毁动画
								isSacrificing_ = false;
								showSacrificeInk_ = true;  // 开始显示献墨量

								// 收集待摧毁的卡牌，检查生生不息印记
                                cardsToDestroy_.clear();
                                sacrificedCards_.clear(); // 清空之前被献祭的卡牌信息
                                for (int j = 0; j < TOTAL_BATTLEFIELD_SLOTS; ++j) {
									if (battlefield_[j].isSacrificed) {
										// 检查是否有生生不息印记
										bool hasImmortal = hasMark(battlefield_[j].card, u8"生生不息");
                                        bool hasMorph = hasMark(battlefield_[j].card, u8"形态转换");

                                        if (hasImmortal) {
                                            // 生生不息：不死亡，但依然产生魂骨
                                            boneCount_++;
                                            statusMessage_ = "生生不息！获得魂骨！当前魂骨数量: " + std::to_string(boneCount_);
                                            // 若同时带有"形态转换"，在此分支中进行形态变换
                                            if (hasMorph) {
                                                std::string currentId = battlefield_[j].card.id;
                                                std::string nextId;
                                                if (currentId == "dishisanzi") nextId = "dishisanzi_juexing";
                                                else if (currentId == "dishisanzi_juexing") nextId = "dishisanzi";
                                                if (!nextId.empty()) {
                                                    SDL_Rect keepRect = battlefield_[j].rect;
                                                    bool keepIsPlayer = battlefield_[j].isPlayer;
                                                    Card next = CardDB::instance().make(nextId);
                                                    if (!next.id.empty()) {
                                                        battlefield_[j].card = next;
                                                        battlefield_[j].health = next.health;
                                                        battlefield_[j].rect = keepRect;
                                                        battlefield_[j].isPlayer = keepIsPlayer;
                                                        battlefield_[j].oneTurnGrowthApplied = false;
                                                        battlefield_[j].placedTurn = currentTurn_;
                                                        statusMessage_ += " | 形态转换：" + next.name;
                                                    }
                                                }
                                            }
                                            // 取消献祭标记，单位保留
                                            battlefield_[j].isSacrificed = false;
                                        }
										else {
											// 正常献祭：记录被献祭的卡牌信息（用于一口之量印记）
											SacrificedCard sacrificedCard;
											sacrificedCard.card = battlefield_[j].card;
											sacrificedCard.attack = battlefield_[j].card.attack;
											sacrificedCard.health = battlefield_[j].health;
											sacrificedCards_.push_back(sacrificedCard);
											
											// 标记为死亡
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
								}
								else {
									statusMessage_ = "献祭完成！生生不息卡牌存活！";
								}
							}
						}
						else if (battlefield_[i].isSacrificed) {
							statusMessage_ = "这张卡牌已经被献祭了";
						}
						else {
							statusMessage_ = "只能献祭己方卡牌";
						}
					}
					else if (selectedHandCard_ >= 0) {
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
									}
									else {
										playCard(selectedHandCard_, i);
										selectedHandCard_ = -1;
										statusMessage_ = "卡牌已放置在第三行";
									}
								}
								else if (handCards_[selectedHandCard_].sacrificeCost > 0 && isSacrificing_) {
									// 需要献祭的卡牌
									statusMessage_ = "需要先献祭才能打出此卡牌";
								}
								else {
									// 0献墨点数或献祭已完成，直接打出
									playCard(selectedHandCard_, i);
									selectedHandCard_ = -1;
									statusMessage_ = "卡牌已放置在第三行";
								}
							}
							else {
								statusMessage_ = "该位置已有卡牌";
							}
						}
						else {
							statusMessage_ = "只能将卡牌放在最下面一行";
						}
					}
					break;
				}
			}
		}

		// 检查道具点击
		for (int i = 0; i < MAX_ITEMS; ++i) {
			if (mouseX >= itemSlots_[i].x && mouseX <= itemSlots_[i].x + itemSlots_[i].w &&
				mouseY >= itemSlots_[i].y && mouseY <= itemSlots_[i].y + itemSlots_[i].h) {
				// 检查是否有道具
				if (i < playerItems_.size()) {
					const Item& item = playerItems_[i];
					
					// 检查是否必须抽牌
					if (mustDrawThisTurn_) {
						statusMessage_ = "必须先抽牌才能使用道具！";
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
					
					// 使用道具
					useItem(item.id);
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

		// 检查是否在断因剑目标选择模式，右键取消选择
		if (isSelectingTarget_) {
			// 取消断因剑目标选择模式，返还道具
			isSelectingTarget_ = false;
			selectedTargetIndex_ = -1;
			addItem("duanyinjian", 1);  // 返还断因剑道具
			statusMessage_ = "已取消断因剑目标选择，道具已返还";
			return;
		}

		// 检查是否在吞墨毫目标选择模式，右键取消选择
		if (isSelectingTunmohaoTarget_) {
			// 取消吞墨毫目标选择模式，返还道具
			isSelectingTunmohaoTarget_ = false;
			selectedTunmohaoTargetIndex_ = -1;
			addItem("tunmohao", 1);  // 返还吞墨毫道具
			statusMessage_ = "已取消吞墨毫目标选择，道具已返还";
			return;
		}

		// 检查是否在缚魂锁目标选择模式，右键取消选择
		if (isSelectingFuhunsuoTarget_) {
			// 取消缚魂锁目标选择模式，返还道具
			isSelectingFuhunsuoTarget_ = false;
			selectedFuhunsuoTargetIndex_ = -1;
			addItem("fuhunsuo", 1);  // 返还缚魂锁道具
			statusMessage_ = "已取消缚魂锁目标选择，道具已返还";
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
			}
			else {
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
	
	if (pendingGoMapExplore_) {
		pendingGoMapExplore_ = false;
		app.setState(std::unique_ptr<State>(static_cast<State*>(new MapExploreState())));
		return;
	}

	// 敌人前进动画更新（若有）
	updateEnemyAdvance(dt);

	// 水袭印记状态更新
	updateWaterAttackMarks();
	
	// 缚魂锁移动动画更新
	updateFuhunsuoAnimation(dt);

	// 成长动画推进
	if (isGrowthAnimating_) {
		growthAnimTime_ += dt;
		if (growthAnimTime_ >= growthAnimDuration_) {
			// 执行成长
			for (const auto& step : pendingGrowth_) {
				if (step.index < 0 || step.index >= TOTAL_BATTLEFIELD_SLOTS) continue;
				auto& bf = battlefield_[step.index];
				if (!bf.isAlive || bf.oneTurnGrowthApplied == true) continue;
				if (step.willTransform) {
					SDL_Rect keepRect = bf.rect;
					bool keepIsPlayer = bf.isPlayer;
					
					// 保存当前战场上的实际数值
					int currentAttack = bf.card.attack;
					int currentHealth = bf.health;
					
					// 计算目标卡牌与原始卡牌的差值
					int attackDiff = step.targetCard.attack - step.addAttack; // step.addAttack 是原始卡牌的attack
					int healthDiff = step.targetCard.health - step.addHealth; // step.addHealth 是原始卡牌的health
					
					// 更新卡牌属性
					bf.card = step.targetCard;
					
					// 应用差值到当前数值
					bf.card.attack = currentAttack + attackDiff;
					bf.card.health = currentHealth + healthDiff;
					bf.health = bf.card.health;
					
					bf.rect = keepRect;
					bf.isPlayer = keepIsPlayer;
					bf.oneTurnGrowthApplied = true;
				}
				else {
					bf.card.attack += step.addAttack;
					bf.card.health += step.addHealth;
					bf.health += step.addHealth;
					bf.oneTurnGrowthApplied = true;
				}
			}
			pendingGrowth_.clear();
			isGrowthAnimating_ = false;
			growthAnimTime_ = 0.0f;

			if (growthForEnemyAttack_) {
				// 敌人攻击前成长：成长完成后再进入敌人攻击
				growthForEnemyAttack_ = false;
				attackingCards_.clear();
				for (int col = 0; col < BATTLEFIELD_COLS; ++col) {
					int enemyIndex = 1 * BATTLEFIELD_COLS + col;  // 第二行（敌方攻击行）
					if (battlefield_[enemyIndex].isAlive && !battlefield_[enemyIndex].isPlayer &&
						battlefield_[enemyIndex].card.attack > 0) {
						attackingCards_.push_back(enemyIndex);
					}
				}
				if (!attackingCards_.empty()) {
					isAttackAnimating_ = true;
					attackAnimTime_ = 0.0f;
					currentAttackingIndex_ = 0;
					isPlayerAttacking_ = false;
					statusMessage_ = "敌方开始攻击！";
					std::cout << "4" << std::endl;
				}
				else {
					// 无可攻击则如常切回玩家回合并在回合开始处再尝试玩家成长
					std::cout << "3" << std::endl;
					// 清理风雅扇的空袭效果（一回合后失效）
					for (int slot : fengyaShanAirstrikeSlots_) {
						if (slot >= 0 && slot < TOTAL_BATTLEFIELD_SLOTS) {
							auto& bf = battlefield_[slot];
							if (bf.isAlive && bf.isPlayer) {
								// 移除空袭印记
								auto it = std::find(bf.card.marks.begin(), bf.card.marks.end(), std::string(u8"空袭"));
								if (it != bf.card.marks.end()) {
									bf.card.marks.erase(it);
								}
							}
						}
					}
					fengyaShanAirstrikeSlots_.clear();
					
					// 敌方回合结束，进行胜负判定
					checkGameOver();
					if (currentPhase_ == GamePhase::GameOver) return; // 游戏结束，不再继续
					
					currentPhase_ = GamePhase::PlayerTurn;
					// 重置本回合献祭次数
					sacrificeCountThisTurn_ = 0;
					if (scheduleTurnStartGrowth()) return;
					
					// 清理风雅扇的空袭效果（回合开始时清理）
					for (int slot : fengyaShanAirstrikeSlots_) {
						if (slot >= 0 && slot < TOTAL_BATTLEFIELD_SLOTS) {
							auto& bf = battlefield_[slot];
							if (bf.isAlive && bf.isPlayer) {
								// 移除空袭印记
								auto it = std::find(bf.card.marks.begin(), bf.card.marks.end(), std::string(u8"空袭"));
								if (it != bf.card.marks.end()) {
									bf.card.marks.erase(it);
								}
							}
						}
					}
					fengyaShanAirstrikeSlots_.clear();
					
					if (mustDrawThisTurn_) statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始 - 必须先抽牌！"; else statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始";
				}
			}
			else if (growthAtTurnStart_) {
				// 玩家回合开始成长：只显示回合提示
				growthAtTurnStart_ = false;
				if (mustDrawThisTurn_) statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始 - 必须先抽牌！"; else statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始";
			}
		}
	}

	// 更新伤害显示
	if (showDamage_) {
		damageDisplayTime_ += dt;
		if (damageDisplayTime_ >= damageDisplayDuration_) {
			showDamage_ = false;

			// 伤害显示完成，检查是否需要开始移动动画
			if (currentPhase_ == GamePhase::PlayerTurn && !isRushing_ && !isBruteForcing_ && !isProcessingMovementQueue_) {
				// 收集所有需要移动的卡牌，按照从左到右的顺序
				pendingMovementCards_.clear();

				// 检查所有卡牌，收集需要移动的卡牌
				for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
					if (battlefield_[i].isAlive && battlefield_[i].isPlayer) {
						if (hasMark(battlefield_[i].card, u8"冲刺能手") || hasMark(battlefield_[i].card, u8"蛮力冲撞")) {
							pendingMovementCards_.push_back(i);
						}
					}
				}

				// 开始处理移动队列
				if (!pendingMovementCards_.empty()) {
					// 将移动卡牌按位置排序（从左到右）
					std::sort(pendingMovementCards_.begin(), pendingMovementCards_.end(), [](int a, int b) {
						return (a % BATTLEFIELD_COLS) < (b % BATTLEFIELD_COLS);
						});

					isProcessingMovementQueue_ = true;
					processNextMovement();
				}
				else {
					// 如果没有移动卡牌，检查日晷效果
					if (riguiEffectActive_) {
						// 日晷效果：跳过敌方回合，直接回到玩家回合
						riguiEffectActive_ = false; // 使用后失效
						currentPhase_ = GamePhase::PlayerTurn;
						currentTurn_++; // 增加回合数
						hasDrawnThisTurn_ = false; // 重置抽牌状态
						mustDrawThisTurn_ = (currentTurn_ >= 2); // 第二回合开始必须抽牌
						// 重置本回合献祭次数
						sacrificeCountThisTurn_ = 0;
						if (scheduleTurnStartGrowth()) return;
						if (mustDrawThisTurn_) statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始 - 必须先抽牌！"; else statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始";
					} else {
						// 玩家回合结束，进行胜负判定
						checkGameOver();
						if (currentPhase_ == GamePhase::GameOver) return; // 游戏结束，不再继续
						
						// 正常进入敌方回合
					currentPhase_ = GamePhase::EnemyTurn;
					currentTurn_++; // 增加回合数
					hasDrawnThisTurn_ = false; // 重置抽牌状态
					mustDrawThisTurn_ = (currentTurn_ >= 2); // 第二回合开始必须抽牌
					std::cout << "2" << std::endl;
					// 延迟执行敌人回合
					enemyTurn();
					}
				}
			}
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
			}
			else {
				// 普通攻击：执行普通攻击逻辑（以临时攻击力判断是否可打）
				if (attackAnimTime_ >= attackAnimDuration_ / 2.0f && !hasAttacked_) {
					hasAttacked_ = true; // 标记已攻击，防止重复执行

					// 执行普通攻击
					int col = currentCardIndex % BATTLEFIELD_COLS;
					int tempAtk = getDisplayAttackForIndex(currentCardIndex);
					if (tempAtk > 0) {
						executeNormalAttack(currentCardIndex, col, isPlayerAttacking_, tempAtk);
					}
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
		}
		else {
			// 所有卡牌攻击完成
			if (isPlayerAttacking_) {
				// 计算本段玩家伤害
				int playerSegmentDamage = std::max(0, enemyHealthBeforePlayerAttack_ - enemyHealth_);
				playerDamageThisCycle_ += playerSegmentDamage;

				// 抵消逻辑：玩家伤害累加，敌人伤害在其段落扣减
				int deltaTicks = playerSegmentDamage;
				if (deltaTicks > 5) deltaTicks = 5;
				if (deltaTicks < -5) deltaTicks = -5;
				meterNet_ += deltaTicks;
				// 上限不再截断，以便记录超出；仍保留下限-5
				if (meterNet_ < -5) meterNet_ = -5;
				
				// 更新显示值（实际值已在攻击时更新）
				meterTargetPos_ = std::min(5, meterActualPos_);
				meterStartPos_ = meterDisplayPos_;
				meterAnimTime_ = 0.0f;
				isMeterAnimating_ = true;
				lastPlayerDamage_ = playerSegmentDamage;

				// 玩家攻击完成，显示伤害并准备移动动画
				isAttackAnimating_ = false;
				attackAnimTime_ = 0.0f;
				attackingCards_.clear();
				currentAttackingIndex_ = 0;

				if (totalDamageDealt_ > 0) {
					showDamage_ = true;
					damageDisplayTime_ = 0.0f;
					statusMessage_ = "造成 " + std::to_string(totalDamageDealt_) + " 点伤害！";
				}
				else {
					statusMessage_ = "我方攻击完成";
					// 即使没有伤害，也要检查移动动画
					showDamage_ = true;
					damageDisplayTime_ = 0.0f;
				}

				// 移动与转入敌方回合的判定，统一在showDamage_结束后的分支执行
			}
			else {
				// 敌方攻击完成，计算本段敌人伤害
				int enemySegmentDamage = std::max(0, playerHealthBeforeEnemyTurn_ - playerHealth_);
				enemyDamageThisCycle_ += enemySegmentDamage;

				// 敌人伤害抵消玩家：记为负向刻度
				int deltaTicks = -enemySegmentDamage;
				if (deltaTicks > 5) deltaTicks = 5;
				if (deltaTicks < -5) deltaTicks = -5;
				meterNet_ += deltaTicks;
				// 上限不再截断，以便记录超出；仍保留下限-5
				if (meterNet_ < -5) meterNet_ = -5;
				
				// 更新显示值（实际值已在攻击时更新）
				meterTargetPos_ = std::min(5, meterActualPos_);
				meterStartPos_ = meterDisplayPos_;
				meterAnimTime_ = 0.0f;
				isMeterAnimating_ = true;
				lastEnemyDamage_ = enemySegmentDamage;

				// 敌方攻击完成，检查是否有可以移动的敌方卡牌
				isAttackAnimating_ = false;
				attackAnimTime_ = 0.0f;
				attackingCards_.clear();
				currentAttackingIndex_ = 0;

				// 检查是否有卡牌需要冲刺能手或蛮力冲撞
				bool hasEnemyRushingCard = false;
				bool hasEnemyBruteForceCard = false;

				for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
					if (battlefield_[i].isAlive && !battlefield_[i].isPlayer) {
						if (hasMark(battlefield_[i].card, u8"冲刺能手")) {
							hasEnemyRushingCard = true;
						}
						if (hasMark(battlefield_[i].card, u8"蛮力冲撞")) {
							hasEnemyBruteForceCard = true;
						}
					}
				}

				if (hasEnemyRushingCard || hasEnemyBruteForceCard) {
					// 有移动卡牌，准备开始移动
					statusMessage_ = "敌方攻击完成，准备移动...";
					// 收集可以移动的敌方卡牌
					pendingMovementCards_.clear();
					for (int col = 0; col < BATTLEFIELD_COLS; ++col) {
						int enemyIndex = 1 * BATTLEFIELD_COLS + col;  // 第二行（敌方攻击行）
						if (battlefield_[enemyIndex].isAlive && !battlefield_[enemyIndex].isPlayer) {
							if (hasMark(battlefield_[enemyIndex].card, u8"冲刺能手") || hasMark(battlefield_[enemyIndex].card, u8"蛮力冲撞")) {
								pendingMovementCards_.push_back(enemyIndex);
							}
						}
					}

					if (!pendingMovementCards_.empty()) {
						// 将移动卡牌按位置排序（从左到右）
						std::sort(pendingMovementCards_.begin(), pendingMovementCards_.end(), [](int a, int b) {
							return (a % BATTLEFIELD_COLS) < (b % BATTLEFIELD_COLS);
						});

						isProcessingMovementQueue_ = true;
						processNextMovement();
						statusMessage_ = "敌方卡牌开始移动！";
					}
					else {
                    // 如果没有可以移动的卡牌，直接回到玩家回合
                    // 掘墓人：敌方回合结束时统计敌方单位
                    grantGravediggerBones(true);
                    
                    // 敌方回合结束，进行胜负判定
                    checkGameOver();
                    if (currentPhase_ == GamePhase::GameOver) return; // 游戏结束，不再继续
                    
                    currentPhase_ = GamePhase::PlayerTurn;
                    // 切回玩家回合：先结算我方回合开始成长（若有成长则先返回，动画完毕后继续）
                    if (scheduleTurnStartGrowth()) return;
						if (mustDrawThisTurn_) {
							statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始 - 必须先抽牌！";
						}
						else {
							statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始";
						}
					}
				}
				else {
                // 没有移动卡牌，直接回到玩家回合
                // 掘墓人：敌方回合结束时统计敌方单位
                grantGravediggerBones(true);
                
                // 清理风雅扇的空袭效果（一回合后失效）
                for (int slot : fengyaShanAirstrikeSlots_) {
                    if (slot >= 0 && slot < TOTAL_BATTLEFIELD_SLOTS) {
                        auto& bf = battlefield_[slot];
                        if (bf.isAlive && bf.isPlayer) {
                            // 移除空袭印记
                            auto it = std::find(bf.card.marks.begin(), bf.card.marks.end(), std::string(u8"空袭"));
                            if (it != bf.card.marks.end()) {
                                bf.card.marks.erase(it);
                            }
                        }
                    }
                }
                fengyaShanAirstrikeSlots_.clear();
                
                currentPhase_ = GamePhase::PlayerTurn;
                // 重置本回合献祭次数
                sacrificeCountThisTurn_ = 0;
                // 切回玩家回合：先结算我方回合开始成长
                if (scheduleTurnStartGrowth()) return;
					if (mustDrawThisTurn_) {
						statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始 - 必须先抽牌！";
					}
					else {
						statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始";
					}
				}
			}
		}
	}

	// 处理特殊攻击动画
	if (isSpecialAttackAnimating_) {
		attackAnimTime_ += dt;

		// 检查当前目标是否有效
		if (currentTargetIndex_ < 8 && specialAttackTargets_[currentTargetIndex_] != -1) {
			int currentTargetIndex = specialAttackTargets_[currentTargetIndex_];

			// 攻击动画进行到一半时执行攻击逻辑
			if (attackAnimTime_ >= attackAnimDuration_ / 2.0f && !hasAttacked_) {
				hasAttacked_ = true;

				// 执行对当前目标的攻击
				int attackerIndex = attackingCards_[currentAttackingIndex_];
				int damage = getDisplayAttackForIndex(attackerIndex);
				attackTarget(attackerIndex, currentTargetIndex, damage);
			}

			// 当前目标攻击完成，切换到下一个目标
			if (attackAnimTime_ >= attackAnimDuration_) {
				currentTargetIndex_++;
				attackAnimTime_ = 0.0f;
				hasAttacked_ = false;

				// 检查是否所有目标都攻击完成
				if (currentTargetIndex_ >= 8 || specialAttackTargets_[currentTargetIndex_] == -1) {
					isSpecialAttackAnimating_ = false;
					// 继续正常的攻击流程
					currentAttackingIndex_++;
					attackAnimTime_ = 0.0f;
					hasAttacked_ = false;

					// 特殊攻击完成，回到正常攻击流程
					// 不在这里检查是否所有卡牌都攻击完成，让正常攻击流程处理
				}
			}
		}
		else {
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

			// 不死印记：改为即时回手，已在死亡检测处处理，这里无需再添加
		}
	}

	// 处理冲刺能手动画
	updateRushing(dt);

	// 处理蛮力冲撞动画
	updateBruteForce(dt);


	// 处理被推动卡牌动画
	updatePushedAnimation(dt);

	// 检测卡牌死亡，获得魂骨
	for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
		bool wasAlive = previousCardStates_[i];
		bool isAlive = battlefield_[i].isAlive && battlefield_[i].isPlayer;

		// 如果卡牌从存活变为死亡，获得魂骨
		if (wasAlive && !isAlive) {
			// 忽略因"移动导致的死亡"（如位移清空等），不触发不死或食尸鬼
			if (battlefield_[i].isMovedToDeath) {
				// 清除标记并跳过
				battlefield_[i].isMovedToDeath = false;
				continue;
			}
			// 全局抑制：若本帧发生位移清位，则不加骨
			if (suppressBoneGainThisFrame_) { continue; }

			// 不死印记：我方单位死亡时，立刻回到手牌（即时渲染）
			if (battlefield_[i].isPlayer) {
				const Card& deadCard = battlefield_[i].card;
				if (hasMark(deadCard, std::string(u8"不死印记"))) {
					Card fresh = deadCard; // 直接复制原卡，保持实例ID
					if (!fresh.id.empty()) {
						// 螣蛇自环：每次死亡永久+1/+1（以卡面基础数值为基准）
						if (hasMark(deadCard, std::string(u8"每次死亡数值+1")) || deadCard.id == std::string("tengshe_zihuan")) {
							fresh.attack = deadCard.attack + 1;
							fresh.health = deadCard.health + 1;
							
							std::cout << "[BATTLE] Tengshe died: " << deadCard.name << " (ID:" << deadCard.id << ", Instance:" << deadCard.instanceId << ") Attack:" << deadCard.attack << "->" << fresh.attack << " Health:" << deadCard.health << "->" << fresh.health << std::endl;
							
							// 记录需要更新的卡牌信息，等退出战斗时再更新牌库
							pendingCardUpdates_[deadCard.instanceId] = {fresh.attack, fresh.health};
						}
						// 意境系统：应用已组合的意境效果到卡牌
						EngraveStore::instance().applyToCard(fresh);
						handCards_.push_back(fresh);
						// 立刻更新手牌布局以立即渲染
						layoutHandCards();
						statusMessage_ = std::string("不死回手：") + fresh.name;
					}
				}
				// 食尸鬼：若手牌中存在"食尸鬼"，在己方单位死亡时自动打出到该位置
				else {
					int ghoulHandIdx = -1;
					for (size_t h = 0; h < handCards_.size(); ++h) {
						for (const auto& mk : handCards_[h].marks) {
							if (mk == std::string(u8"食尸鬼")) { ghoulHandIdx = static_cast<int>(h); break; }
						}
						if (ghoulHandIdx != -1) break;
					}
					if (ghoulHandIdx != -1) {
						Card ghoul = handCards_[ghoulHandIdx];
						handCards_.erase(handCards_.begin() + ghoulHandIdx);
						layoutHandCards();
						// 占位
						battlefield_[i].card = ghoul;
						battlefield_[i].isAlive = true;
						battlefield_[i].health = ghoul.health;
						battlefield_[i].isPlayer = true;
						battlefield_[i].placedTurn = currentTurn_;
						battlefield_[i].oneTurnGrowthApplied = false;
						statusMessage_ = std::string("食尸鬼自动登场：") + ghoul.name;

						// 若该索引在摧毁动画队列中，移除以避免渲染冲突
						for (auto it = cardsToDestroy_.begin(); it != cardsToDestroy_.end(); ) {
							if (*it == i) it = cardsToDestroy_.erase(it); else ++it;
						}
					}
				}
			}
			// 检查是否是生生不息卡牌在献祭时死亡（这种情况已经在献祭逻辑中处理了魂骨）
			bool wasSacrificed = battlefield_[i].isSacrificed;
			bool hasImmortal = hasMark(battlefield_[i].card, u8"生生不息");
			bool isMovedToDeath = battlefield_[i].isMovedToDeath; // 检查是否因移动死亡

            // 如果不是生生不息卡牌在献祭时死亡，且不是因移动死亡，则获得魂骨
            if (!(wasSacrificed && hasImmortal) && !isMovedToDeath) {
                boneCount_++;
                // 骨王：死亡时额外获得3根骨头（总计4根）
                if (hasMark(battlefield_[i].card, std::string(u8"骨王"))) {
                    boneCount_ += 3;
                }
                statusMessage_ = "获得魂骨！当前魂骨数量: " + std::to_string(boneCount_);
				// 清理一次性标志
				battlefield_[i].isMovedToDeath = false;
            }
		}

		// 更新状态记录
		previousCardStates_[i] = isAlive;
	}

	// 拾荒者印记：检查敌方卡牌死亡，如果有玩家的拾荒者在场，也产生魂骨
	for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
		bool wasAlive = previousEnemyCardStates_[i];
		bool isAlive = battlefield_[i].isAlive && !battlefield_[i].isPlayer;

		// 如果敌方卡牌从存活变为死亡，检查是否有玩家的拾荒者在场
		if (wasAlive && !isAlive) {
			// 忽略因"移动导致的死亡"
			if (battlefield_[i].isMovedToDeath) {
				battlefield_[i].isMovedToDeath = false;
				continue;
			}

			// 检查场上是否有玩家的拾荒者印记的卡牌
			bool hasPlayerScavenger = false;
			std::string scavengerName = "";
			for (int j = 0; j < TOTAL_BATTLEFIELD_SLOTS; ++j) {
				if (battlefield_[j].isAlive && battlefield_[j].isPlayer && hasMark(battlefield_[j].card, std::string(u8"拾荒者"))) {
					hasPlayerScavenger = true;
					scavengerName = battlefield_[j].card.name;
					break;
				}
			}

			if (hasPlayerScavenger) {
				boneCount_++;
				statusMessage_ = std::string("拾荒者：") + scavengerName + " 获得魂骨！当前魂骨数量: " + std::to_string(boneCount_);
			}
		}

		// 更新敌方状态记录
		previousEnemyCardStates_[i] = isAlive;
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
			}
			else {
				// 非选中的卡牌保持静止
				handCardFloatY_[i] = 0.0f;
			}
		}
	}

	// 游戏结束判定已移至回合结束时进行

    // 墨尺指针动画插值（若锁定则保持不变）
    if (isMeterAnimating_) {
        if (!lockPlayerHealth_) {
            meterAnimTime_ += dt;
            float t = std::min(1.0f, meterAnimTime_ / meterAnimDuration_);
            // easeInOut 曲线
            float tEase = (1.0f - std::cos(3.1415926f * t)) * 0.5f;
            meterDisplayPos_ = meterStartPos_ + (meterTargetPos_ - meterStartPos_) * tEase;
            if (t >= 1.0f) {
                isMeterAnimating_ = false;
                meterDisplayPos_ = static_cast<float>(meterTargetPos_);
            }
        } else {
            // 锁定：强制还原
            meterNet_ = lockedMeterNetValue_;
            meterDisplayPos_ = lockedMeterDisplayPos_;
            meterActualPos_ = lockedMeterActual_;
            meterTargetPos_ = lockedMeterTarget_;
            isMeterAnimating_ = false;
        }
    }
}
void BattleState::render(App& app) {
	SDL_Renderer* renderer = app.getRenderer();


	// 绘制战斗背景
	SDL_SetRenderDrawColor(renderer, 20, 24, 34, 255);
	SDL_Rect bgRect = { 0, 0, screenW_, screenH_ };
	SDL_RenderFillRect(renderer, &bgRect);


	// 渲染各个区域
	renderBattlefield(app);
	renderHandCards(app);
	renderUI(app);
	
	// 渲染已组合意境提示
	renderEngravingHints(app);
	
	// 检索状态：渲染牌堆/全卡库卡牌选择界面
	if (isSearchingDeck_) {
		renderDeckSelection(app);
	}
	
}

// 私有方法实现
void BattleState::initializeBattle() {
	// 初始化游戏状态
	currentPhase_ = GamePhase::PlayerTurn;
	currentTurn_ = 1;
	playerHealth_ = 20;
	enemyHealth_ = 100;
	maxInk_ = 10;
	selectedHandCard_ = -1;
	statusMessage_ = "战斗开始！";

	// 初始化护主翻面状态数组（默认不翻面）
	for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
		waterAttackFlipped_[i] = false;
	}

	// 清空战场
	for (auto& card : battlefield_) {
		card.isAlive = false;
		card.health = 0;
	}

	// 开局抽牌：1张墨锭 + 3张玩家牌
	handCards_.clear();
	battleDeck_.clear();

	// 初始化随机数种子
	srand(static_cast<unsigned int>(time(nullptr)));

	// 使用全局牌堆，但维护本地手牌副本
	auto& store = DeckStore::instance();

	// 将全局牌库复制到战斗牌堆
	battleDeck_ = store.library();

	// 每次进入战斗都重新抽取手牌，不保留之前的手牌
	// 抽1张墨锭（必须是墨锭）
	Card inkCard = CardDB::instance().make("moding");
	if (!inkCard.id.empty()) {
		handCards_.push_back(inkCard);
		store.hand().push_back(inkCard);
	}

	// 抽3张玩家牌（从战斗牌堆中随机抽取并移除）
	for (int i = 0; i < 3 && !battleDeck_.empty(); ++i) {
		// 随机选择一张牌
		int randomIndex = rand() % battleDeck_.size();
		Card card = battleDeck_[randomIndex];
		// 从战斗牌堆中移除
		battleDeck_.erase(battleDeck_.begin() + randomIndex);
		
		// 意境系统：应用已组合的意境效果到卡牌
		EngraveStore::instance().applyToCard(card);
		
		// 随机印记效果：加入手牌时，删去随机印记并添加任意一个印记
		applyRandomMarkEffect(card);
		handCards_.push_back(card);
		store.hand().push_back(card);
	}

	// 调试信息：确认手牌数量
	statusMessage_ = "开局手牌数量: " + std::to_string(handCards_.size()) + " (1张墨锭 + 3张玩家牌)";

	// 初始化牌堆
	inkPileCount_ = 10;  // 墨锭牌堆只有10张

	// 初始化回合制系统
	hasDrawnThisTurn_ = false;
	mustDrawThisTurn_ = false;  // 第一回合不需要强制抽牌

	// 初始化战斗系统
	enemyHealth_ = 100;
	totalDamageDealt_ = 0;
	showDamage_ = false;
	damageDisplayTime_ = 0.0f;

	// 初始化魂骨系统
	boneCount_ = 0;
	previousCardStates_.resize(TOTAL_BATTLEFIELD_SLOTS, false);
	previousEnemyCardStates_.resize(TOTAL_BATTLEFIELD_SLOTS, false);

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
	
	// 初始化道具系统
	playerItems_.clear();
	addItem("fuhunsuo", 1);  // 缚魂锁
	addItem("yinyang_pei", 1);  // 阴阳佩
	addItem("mobao_ping", 1);  // 墨宝瓶
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
	}
	else {
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
	battlefield_[battlefieldIndex].placedTurn = currentTurn_;
	battlefield_[battlefieldIndex].oneTurnGrowthApplied = false;

	// 滋生寄生虫：打出时若对位为空，生成对位单位（10%玄乌之卵，否则破碎的卵）
	if (hasMark(card, std::string(u8"滋生寄生虫"))) {
		int col = battlefieldIndex % BATTLEFIELD_COLS;
		int opposeIndex = 1 * BATTLEFIELD_COLS + col; // 敌方对位
		if (opposeIndex >= 0 && opposeIndex < TOTAL_BATTLEFIELD_SLOTS && !battlefield_[opposeIndex].isAlive) {
			int r = rand() % 100; // 简易概率
			Card egg = CardDB::instance().make(r < 10 ? "xuanwu_zhi" : "posui_deluan");
			if (!egg.id.empty()) {
				battlefield_[opposeIndex].card = egg;
				battlefield_[opposeIndex].isAlive = true;
				battlefield_[opposeIndex].health = egg.health;
				battlefield_[opposeIndex].isPlayer = false;
				battlefield_[opposeIndex].placedTurn = currentTurn_;
				battlefield_[opposeIndex].oneTurnGrowthApplied = false;
				statusMessage_ = std::string("滋生寄生虫：对位生成 ") + egg.name;
			}
		}
	}

	// 蚁后印记：打出带有蚁后印记的卡牌时，手牌获得一张抄经工蚁
	if (hasMark(card, std::string(u8"蚁后"))) {
		Card ant = CardDB::instance().make("chaojing_gongyi");
		if (!ant.id.empty()) {
			handCards_.push_back(ant);
			layoutHandCards();
			statusMessage_ = std::string("蚁后：获得手牌 ") + ant.name;
		}
	}

	// 兔窝印记：打出带有兔窝印记的卡牌时，手牌获得一张白毫仔
	if (hasMark(card, std::string(u8"兔窝"))) {
		Card rabbit = CardDB::instance().make("baimao_zi");
		if (!rabbit.id.empty()) {
			handCards_.push_back(rabbit);
			layoutHandCards();
			statusMessage_ = std::string("兔窝：获得手牌 ") + rabbit.name;
		}
	}

	// 丰产之巢印记：打出时手牌获得一张和打出牌一模一样的卡牌，除了没有丰产之巢印记
	if (hasMark(card, std::string(u8"丰产之巢"))) {
		// 创建一张相同的卡牌
		Card copyCard = card;
		
		// 移除丰产之巢印记
		auto it = std::find(copyCard.marks.begin(), copyCard.marks.end(), std::string(u8"丰产之巢"));
		if (it != copyCard.marks.end()) {
			copyCard.marks.erase(it);
		}
		
		// 添加到手牌（应用意境）
		EngraveStore::instance().applyToCard(copyCard);
		handCards_.push_back(copyCard);
		layoutHandCards();
		statusMessage_ = std::string("丰产之巢：获得手牌 ") + copyCard.name;
	}

	// 检索印记：打出时可以从牌堆中选择任意一张牌加入手牌
	if (hasMark(card, std::string(u8"检索"))) {
		// 启动检索状态
		isSearchingDeck_ = true;
		selectedDeckCardIndex_ = -1;
		statusMessage_ = std::string("检索：请从牌堆中选择一张卡牌");
	}

	// 道具商印记：打出时获得一个道具（前提是道具没满）
	if (hasMark(card, std::string(u8"道具商"))) {
		if (playerItems_.size() < MAX_ITEMS) {
			// 获取随机道具
			std::string selectedItem = getRandomItem();
			if (!selectedItem.empty()) {
				addItem(selectedItem, 1);
				statusMessage_ = std::string("道具商：获得道具 ") + selectedItem;
			} else {
				statusMessage_ = "道具商：无法获取随机道具！";
			}
		} else {
			statusMessage_ = "道具商：道具栏已满，无法获得新道具！";
		}
	}

	// 筑坝师印记：打出后将相邻两格空位变为堤坝
	if (hasMark(card, std::string(u8"筑坝师"))) {
		int row = battlefieldIndex / BATTLEFIELD_COLS;
		int col = battlefieldIndex % BATTLEFIELD_COLS;
		int damCount = 0;
		
		// 检查左右相邻位置
		for (int offset : {-1, 1}) {
			int adjacentCol = col + offset;
			if (adjacentCol >= 0 && adjacentCol < BATTLEFIELD_COLS) {
				int adjacentIndex = row * BATTLEFIELD_COLS + adjacentCol;
				if (adjacentIndex >= 0 && adjacentIndex < TOTAL_BATTLEFIELD_SLOTS && !battlefield_[adjacentIndex].isAlive) {
					// 空位，创建堤坝
					Card dam = CardDB::instance().make("diba");
					if (!dam.id.empty()) {
						battlefield_[adjacentIndex].card = dam;
						battlefield_[adjacentIndex].isAlive = true;
						battlefield_[adjacentIndex].health = dam.health;
						battlefield_[adjacentIndex].isPlayer = true; // 堤坝属于玩家
						battlefield_[adjacentIndex].placedTurn = currentTurn_;
						battlefield_[adjacentIndex].oneTurnGrowthApplied = false;
						damCount++;
					}
				}
			}
		}
		
		if (damCount > 0) {
			statusMessage_ = std::string("筑坝师：创建了 ") + std::to_string(damCount) + " 个堤坝！";
		}
	}

	// 一口之量印记：检查是否有被献祭的卡牌，如果有，为打出的卡牌增加攻击力和血量
	if (!sacrificedCards_.empty()) {
		int totalAttackBonus = 0;
		int totalHealthBonus = 0;
		std::string bonusMessage = "";
		
		for (const auto& sacrificedCard : sacrificedCards_) {
			if (hasMark(sacrificedCard.card, std::string(u8"一口之量"))) {
				totalAttackBonus += sacrificedCard.attack;
				totalHealthBonus += sacrificedCard.health;
				if (!bonusMessage.empty()) bonusMessage += " + ";
				bonusMessage += sacrificedCard.card.name + "(" + std::to_string(sacrificedCard.attack) + "/" + std::to_string(sacrificedCard.health) + ")";
			}
		}
		
		if (totalAttackBonus > 0 || totalHealthBonus > 0) {
			// 为打出的卡牌增加攻击力和血量
			battlefield_[battlefieldIndex].card.attack += totalAttackBonus;
			battlefield_[battlefieldIndex].health += totalHealthBonus;
			battlefield_[battlefieldIndex].card.health += totalHealthBonus;
			
			statusMessage_ = std::string("一口之量：") + card.name + " 获得 " + bonusMessage + " 的加成！";
		}
		
		// 清空被献祭的卡牌信息
		sacrificedCards_.clear();
	}

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

	// 我方打出卡牌后：触发敌方护主补位到该对位（若满足条件）
	applyGuardianForPlayerPlay(battlefieldIndex);
}

void BattleState::endTurn() {
	if (currentPhase_ != GamePhase::PlayerTurn) return;

	// 收集所有可以攻击的我方卡牌（从左到右）
	attackingCards_.clear();
	for (int col = 0; col < BATTLEFIELD_COLS; ++col) {
		int playerIndex = 2 * BATTLEFIELD_COLS + col; // 第三行
		if (battlefield_[playerIndex].isAlive && battlefield_[playerIndex].isPlayer) {
				// 使用临时攻击力判断资格（受对位"臭臭/令人生厌"影响）
			int displayAtk = getDisplayAttackForIndex(playerIndex);
				
				// 对于多向攻击，检查是否至少有一个方向可以攻击
				BattleState::AttackType attackType = getCardAttackType(battlefield_[playerIndex].card);
				bool canAttack = false;
				
				if (attackType == BattleState::AttackType::Normal) {
					// 普通攻击：检查对位是否有厌恶情绪
					int opposeIndex = 1 * BATTLEFIELD_COLS + col; // 敌方对位
					if (opposeIndex >= 0 && opposeIndex < TOTAL_BATTLEFIELD_SLOTS && battlefield_[opposeIndex].isAlive) {
						if (hasMark(battlefield_[opposeIndex].card, std::string(u8"厌恶情绪"))) {
							displayAtk = 0; // 厌恶情绪使攻击力变为0
						}
					}
					canAttack = (displayAtk > 0);
				} else {
					// 多向攻击：检查是否至少有一个方向可以攻击
					canAttack = canMultiDirectionAttack(playerIndex, col, true);
				}
				
				if (canAttack) {
			attackingCards_.push_back(playerIndex);
			}
		}
	}

	if (!attackingCards_.empty()) {
		// 记录敌方血量基准，用于计算本段我方造成伤害
		enemyHealthBeforePlayerAttack_ = enemyHealth_;

		// 开始攻击动画
		isAttackAnimating_ = true;
		attackAnimTime_ = 0.0f;
		currentAttackingIndex_ = 0;
		isPlayerAttacking_ = true;
		totalDamageDealt_ = 0;
		statusMessage_ = "我方开始攻击！";
	}
    else {
		// 没有可攻击的卡牌，检查日晷效果
        // 掘墓人：我方回合结束时统计我方单位
        grantGravediggerBones(false);
        
        if (riguiEffectActive_) {
			// 日晷效果：跳过敌方回合，直接回到玩家回合
			riguiEffectActive_ = false; // 使用后失效
			currentPhase_ = GamePhase::PlayerTurn;
			currentTurn_++; // 增加回合数
			hasDrawnThisTurn_ = false; // 重置抽牌状态
			mustDrawThisTurn_ = (currentTurn_ >= 2); // 第二回合开始必须抽牌
			// 重置本回合献祭次数
			sacrificeCountThisTurn_ = 0;
			// 回合开始：抽牌提示前先成长。若有成长，先return，update中落位后再显示提示。
			if (scheduleTurnStartGrowth()) return;
			if (mustDrawThisTurn_) {
				statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始 - 必须先抽牌！";
			}
			else {
				statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始";
			}
		} else {
			// 玩家回合结束，进行胜负判定
			checkGameOver();
			if (currentPhase_ == GamePhase::GameOver) return; // 游戏结束，不再继续
			
			// 正常进入敌方回合
		currentPhase_ = GamePhase::EnemyTurn;
		currentTurn_++; // 增加回合数
		hasDrawnThisTurn_ = false; // 重置抽牌状态
		mustDrawThisTurn_ = (currentTurn_ >= 2); // 第二回合开始必须抽牌
		std::cout << "1" << std::endl;
		// 延迟执行敌人回合
		enemyTurn();
		}
	}
}

void BattleState::enemyTurn() {
	// 敌方攻击开始前：先完成敌人前进（若有）
	if (startEnemyAdvanceIfAny()) {
		return; // 动画完成后会回调进入敌方攻击
	}

	// 敌方前进完成后：敌人攻击前成长（含动画）。若有成长，先return，动画结束后再进入攻击。
	if (scheduleEnemyPreAttackGrowth()) return;

	// 敌人前进与成长完成后：应用我方护主补位到对位（若满足条件）
	applyGuardianForEnemyAttack();

	// 收集所有可以攻击的敌方卡牌（从左到右）
	attackingCards_.clear();
	for (int col = 0; col < BATTLEFIELD_COLS; ++col) {
		int enemyIndex = 1 * BATTLEFIELD_COLS + col;  // 第二行（敌方攻击行）
		if (battlefield_[enemyIndex].isAlive && !battlefield_[enemyIndex].isPlayer) {
			int displayAtk = getDisplayAttackForIndex(enemyIndex);
			
			// 对于多向攻击，检查是否至少有一个方向可以攻击
			BattleState::AttackType attackType = getCardAttackType(battlefield_[enemyIndex].card);
			bool canAttack = false;
			
			if (attackType == BattleState::AttackType::Normal) {
				// 普通攻击：检查对位是否有厌恶情绪
				int opposeIndex = 2 * BATTLEFIELD_COLS + col; // 我方对位
				if (opposeIndex >= 0 && opposeIndex < TOTAL_BATTLEFIELD_SLOTS && battlefield_[opposeIndex].isAlive) {
					if (hasMark(battlefield_[opposeIndex].card, std::string(u8"厌恶情绪"))) {
						displayAtk = 0; // 厌恶情绪使攻击力变为0
					}
				}
				canAttack = (displayAtk > 0);
			} else {
				// 多向攻击：检查是否至少有一个方向可以攻击
				canAttack = canMultiDirectionAttack(enemyIndex, col, false);
			}
			
			if (canAttack) {
			attackingCards_.push_back(enemyIndex);
			}
		}
	}

	if (!attackingCards_.empty()) {
		// 记录玩家血量基准，用于计算本段敌方造成伤害
		playerHealthBeforeEnemyTurn_ = playerHealth_;

		// 开始敌方攻击动画
		isAttackAnimating_ = true;
		attackAnimTime_ = 0.0f;
		currentAttackingIndex_ = 0;
		isPlayerAttacking_ = false;
		statusMessage_ = "敌方开始攻击！";
	}
	else {
		// 没有可攻击的敌方卡牌，直接回到玩家回合
	
	// 清理风雅扇的空袭效果（一回合后失效）
	for (int slot : fengyaShanAirstrikeSlots_) {
		if (slot >= 0 && slot < TOTAL_BATTLEFIELD_SLOTS) {
			auto& bf = battlefield_[slot];
			if (bf.isAlive && bf.isPlayer) {
				// 移除空袭印记
				auto it = std::find(bf.card.marks.begin(), bf.card.marks.end(), std::string(u8"空袭"));
				if (it != bf.card.marks.end()) {
					bf.card.marks.erase(it);
				}
			}
		}
	}
	fengyaShanAirstrikeSlots_.clear();
	
		currentPhase_ = GamePhase::PlayerTurn;
	// 重置本回合献祭次数
	sacrificeCountThisTurn_ = 0;
		// 回合开始：抽牌提示前先成长。若有成长，先return，update中落位后再显示提示。
		if (scheduleTurnStartGrowth()) return;
		if (mustDrawThisTurn_) {
			statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始 - 必须先抽牌！";
		}
		else {
			statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始";
		}
	}
}
// 护主：敌方攻击前（敌人前进与成长完成后），我方护主补位到对位
void BattleState::applyGuardianForEnemyAttack() {
	// 敌方攻击行：第2行（row=1），我方防守行：第3行（row=2）
	for (int col = 0; col < BATTLEFIELD_COLS; ++col) {
		int enemyIndex = 1 * BATTLEFIELD_COLS + col;   // 敌方攻击行位置
		int playerIndex = 2 * BATTLEFIELD_COLS + col;  // 我方防守行对位

		// 条件：敌方对位有敌人卡，且我方对位为空
		if (battlefield_[enemyIndex].isAlive && !battlefield_[enemyIndex].isPlayer && !battlefield_[playerIndex].isAlive) {
			// 在我方防守行中寻找任意带"护主"的卡
			int candidateIndex = -1;
			for (int searchCol = 0; searchCol < BATTLEFIELD_COLS; ++searchCol) {
				int idx = 2 * BATTLEFIELD_COLS + searchCol;
				if (battlefield_[idx].isAlive && battlefield_[idx].isPlayer && hasMark(battlefield_[idx].card, std::string(u8"护主"))) {
					candidateIndex = idx;
					break;
				}
			}

			if (candidateIndex != -1) {
				// 瞬时移位到对位（与推动/冲撞的落位一致，不产生额外动画）
				SDL_Rect targetRect = battlefield_[playerIndex].rect;
				battlefield_[playerIndex] = battlefield_[candidateIndex];
				battlefield_[playerIndex].rect = targetRect;
				battlefield_[candidateIndex].isAlive = false;
				battlefield_[candidateIndex].health = 0;
				battlefield_[candidateIndex].isMovedToDeath = true; // 复用标志，避免魂骨
				statusMessage_ = "护主生效：我方防守到对位";
			}
		}
	}
}

// 护主：我方打出卡牌后，敌方护主补位到该对位
void BattleState::applyGuardianForPlayerPlay(int justPlacedIndex) {
	// justPlacedIndex 一定在我方放置行（row=2），其对位在敌方防守行 row=1
	int col = justPlacedIndex % BATTLEFIELD_COLS;
	int enemyIndex = 1 * BATTLEFIELD_COLS + col;

	// 条件：我方对位存在玩家卡，且敌方对位为空
	if (battlefield_[justPlacedIndex].isAlive && battlefield_[justPlacedIndex].isPlayer && !battlefield_[enemyIndex].isAlive) {
		// 在敌方防守行中寻找任意带"护主"的卡
		int candidateIndex = -1;
		for (int searchCol = 0; searchCol < BATTLEFIELD_COLS; ++searchCol) {
			int idx = 1 * BATTLEFIELD_COLS + searchCol;
			if (battlefield_[idx].isAlive && !battlefield_[idx].isPlayer && hasMark(battlefield_[idx].card, std::string(u8"护主"))) {
				candidateIndex = idx;
				break;
			}
		}

		if (candidateIndex != -1) {
			// 瞬时移位到对位
			SDL_Rect targetRect = battlefield_[enemyIndex].rect;
			battlefield_[enemyIndex] = battlefield_[candidateIndex];
			battlefield_[enemyIndex].rect = targetRect;
			battlefield_[candidateIndex].isAlive = false;
			battlefield_[candidateIndex].health = 0;
			battlefield_[candidateIndex].isMovedToDeath = true;
			statusMessage_ = "护主生效：敌方防守到对位";
		}
	}
}

bool BattleState::startEnemyAdvanceIfAny() {
	// 若已有动画在进行，直接返回
	if (isEnemyAdvancing_) return true;

	// 准备步骤
	prepareEnemyAdvanceSteps();
	if (enemyAdvanceSteps_.empty()) return false;

	// 启动动画
	isEnemyAdvancing_ = true;
	enemyAdvanceStartedThisFrame_ = true;
	enemyAdvanceAnimTime_ = 0.0f;
	statusMessage_ = "敌人前进中...";
	return true;
}

void BattleState::prepareEnemyAdvanceSteps() {
	enemyAdvanceSteps_.clear();
	for (int col = 0; col < BATTLEFIELD_COLS; ++col) {
		int fromIndex = 0 * BATTLEFIELD_COLS + col;
		int toIndex = 1 * BATTLEFIELD_COLS + col;
		if (battlefield_[fromIndex].isAlive && !battlefield_[fromIndex].isPlayer && !battlefield_[toIndex].isAlive) {
			EnemyAdvanceStep step;
			step.fromIndex = fromIndex;
			step.toIndex = toIndex;
			step.fromRect = battlefield_[fromIndex].rect;
			step.toRect = battlefield_[toIndex].rect;
			enemyAdvanceSteps_.push_back(step);
		}
	}
}

void BattleState::updateEnemyAdvance(float dt) {
	if (!isEnemyAdvancing_) return;
	// 启动当帧不累计时间，避免瞬移
	if (enemyAdvanceStartedThisFrame_) {
		enemyAdvanceStartedThisFrame_ = false;
		return;
	}
	enemyAdvanceAnimTime_ += dt;
	if (enemyAdvanceAnimTime_ >= enemyAdvanceAnimDuration_) {
		// 动画结束，执行落位
		executeEnemyAdvance();
		isEnemyAdvancing_ = false;
		enemyAdvanceAnimTime_ = 0.0f;

		// 动画完成后，进入敌方攻击流程
		enemyTurn();
	}
}

void BattleState::executeEnemyAdvance() {
	// 将所有步骤一次性落位
	for (const auto& step : enemyAdvanceSteps_) {
		if (step.fromIndex >= 0 && step.fromIndex < TOTAL_BATTLEFIELD_SLOTS &&
			step.toIndex >= 0 && step.toIndex < TOTAL_BATTLEFIELD_SLOTS) {
			if (battlefield_[step.fromIndex].isAlive && !battlefield_[step.toIndex].isAlive && !battlefield_[step.fromIndex].isPlayer) {
				SDL_Rect targetRect = battlefield_[step.toIndex].rect;
				battlefield_[step.toIndex] = battlefield_[step.fromIndex];
				battlefield_[step.toIndex].rect = targetRect;

				// 敌人进入第二行（攻击行）时，从这一回合开始重新计时成长
				int toRow = step.toIndex / BATTLEFIELD_COLS;
				if (toRow == 1) {
					battlefield_[step.toIndex].placedTurn = currentTurn_;
					battlefield_[step.toIndex].oneTurnGrowthApplied = false;

					// 敌方"滋生寄生虫"进入第二行：若我方对位为空，生成单位（10%玄乌之卵，否则破碎的卵）
					const auto& advCard = battlefield_[step.toIndex].card;
					if (hasMark(advCard, std::string(u8"滋生寄生虫"))) {
						int col = step.toIndex % BATTLEFIELD_COLS;
						int opposeIndex = 2 * BATTLEFIELD_COLS + col; // 我方对位（第三行）
						if (opposeIndex >= 0 && opposeIndex < TOTAL_BATTLEFIELD_SLOTS && !battlefield_[opposeIndex].isAlive) {
							int r = rand() % 100;
							Card egg = CardDB::instance().make(r < 10 ? "xuanwu_zhi" : "posui_deluan");
							if (!egg.id.empty()) {
								battlefield_[opposeIndex].card = egg;
								battlefield_[opposeIndex].isAlive = true;
								battlefield_[opposeIndex].health = egg.health;
								battlefield_[opposeIndex].isPlayer = true; // 生成在我方
								battlefield_[opposeIndex].placedTurn = currentTurn_;
								battlefield_[opposeIndex].oneTurnGrowthApplied = false;
								statusMessage_ = std::string("滋生寄生虫：我方对位生成 ") + egg.name;
							}
						}
					}
				}

                // 敌人推进：原位置被清空（是"移动离开"，非死亡，不加骨），但也不应该触发死亡逻辑
				battlefield_[step.fromIndex].isAlive = false;
				battlefield_[step.fromIndex].health = 0;
                battlefield_[step.fromIndex].isMovedToDeath = true;
			}
		}
	}
	enemyAdvanceSteps_.clear();
}

void BattleState::checkGameOver() {
    // 调试日志：检查墨尺数值
    std::cout << "[METER] check actualPos=" << meterActualPos_ << " displayPos=" << meterTargetPos_ << std::endl;

    // 胜利：实际墨尺 >= 5
    if (meterActualPos_ >= 5) {
        int wenmaiGained = meterActualPos_ - 5;
        currentPhase_ = GamePhase::GameOver;
        if (wenmaiGained > 0) {
            // 添加文脉到全局存储
            WenMaiStore::instance().add(wenmaiGained);
            statusMessage_ = "胜利！获得 " + std::to_string(wenmaiGained) + " 文脉！";
        } else {
            statusMessage_ = "胜利！";
        }
        return;
    }

    // 失败：实际墨尺 <= -5
    if (meterActualPos_ <= -5) {
        currentPhase_ = GamePhase::GameOver;
        statusMessage_ = "游戏失败！";
        return;
    }

	if (playerHealth_ <= 0) {
		currentPhase_ = GamePhase::GameOver;
		statusMessage_ = "游戏失败！";
	}
	else if (enemyHealth_ <= 0) {
		currentPhase_ = GamePhase::GameOver;
		statusMessage_ = "胜利！";
	}
}

void BattleState::renderBattlefield(App& app) {
	SDL_Renderer* r = app.getRenderer();

	// 移除战场背景框


    // 移除整行高亮，仅保留单格高亮

	// 第一遍：绘制所有非攻击中的卡牌
	for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
		const auto& bfCard = battlefield_[i];
		bool isAttacking = false;
		if (isAttackAnimating_ && currentAttackingIndex_ < static_cast<int>(attackingCards_.size())) {
			int currentCardIndex = attackingCards_[currentAttackingIndex_];
			if (currentCardIndex == i) {
				isAttacking = true;
			}
		}
		if (isAttacking) continue;


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

		// 断因剑目标选择高亮效果
		if (isSelectingTarget_) {
			int row = i / BATTLEFIELD_COLS;
			if (row == 1 && bfCard.isAlive && !bfCard.isPlayer) {
				// 高亮敌人第二行的卡牌
				SDL_SetRenderDrawColor(r, 255, 100, 100, 100);
				SDL_Rect highlightRect = bfCard.rect;
				highlightRect.x -= 2;
				highlightRect.y -= 2;
				highlightRect.w += 4;
				highlightRect.h += 4;
				SDL_RenderDrawRect(r, &highlightRect);
			}
		}

		// 吞墨毫目标选择高亮效果
		if (isSelectingTunmohaoTarget_) {
			int row = i / BATTLEFIELD_COLS;
			if (row == 1 && bfCard.isAlive && !bfCard.isPlayer) {
				// 高亮敌人第二行的卡牌（使用不同的颜色）
				SDL_SetRenderDrawColor(r, 100, 255, 100, 100);
				SDL_Rect highlightRect = bfCard.rect;
				highlightRect.x -= 2;
				highlightRect.y -= 2;
				highlightRect.w += 4;
				highlightRect.h += 4;
				SDL_RenderDrawRect(r, &highlightRect);
			}
		}

		// 缚魂锁目标选择高亮效果
		if (isSelectingFuhunsuoTarget_) {
			int row = i / BATTLEFIELD_COLS;
			if (row == 1 && bfCard.isAlive && !bfCard.isPlayer) {
				// 检查对应的我方第三行位置是否为空
				int targetCol = i % BATTLEFIELD_COLS;
				int playerTargetIndex = 2 * BATTLEFIELD_COLS + targetCol;
				bool canMove = (playerTargetIndex >= 0 && playerTargetIndex < TOTAL_BATTLEFIELD_SLOTS && 
							   !battlefield_[playerTargetIndex].isAlive);
				
				// 高亮敌人第二行的卡牌（使用蓝色表示可移动，红色表示不可移动）
				if (canMove) {
					SDL_SetRenderDrawColor(r, 100, 100, 255, 100); // 蓝色：可以移动
				} else {
					SDL_SetRenderDrawColor(r, 255, 100, 100, 100); // 红色：不可移动
				}
				SDL_Rect highlightRect = bfCard.rect;
				highlightRect.x -= 2;
				highlightRect.y -= 2;
				highlightRect.w += 4;
				highlightRect.h += 4;
				SDL_RenderDrawRect(r, &highlightRect);
			}
		}

		// 敌人前进插值渲染
		bool isAdvancingThis = false;
		SDL_Rect advancingRect{};
		if (isEnemyAdvancing_) {
			float t = enemyAdvanceStartedThisFrame_ ? 0.0f : std::min(1.0f, enemyAdvanceAnimTime_ / enemyAdvanceAnimDuration_);
			float t_ease = (1.0f - std::cos(3.1415926f * t)) * 0.5f;
			for (const auto& step : enemyAdvanceSteps_) {
				if (step.fromIndex == i) {
					isAdvancingThis = true;
					float baseX = step.fromRect.x + (step.toRect.x - step.fromRect.x) * t_ease;
					float baseY = step.fromRect.y + (step.toRect.y - step.fromRect.y) * t_ease;
					float centerFromY = step.fromRect.y + step.fromRect.h * 0.5f;
					float centerToY = step.toRect.y + step.toRect.h * 0.5f;
					float deltaCenterY = std::abs(centerToY - centerFromY);
					float overshootMax = std::max(enemyAdvanceMinOvershootPx_, deltaCenterY * enemyAdvanceOvershootScale_);
					float bell; { float mid=0.5f, width=0.38f; float d=std::abs(t-mid)/width; bell = d>=1.0f?0.0f:(1.0f-d);} 
					float overshoot = bell * overshootMax;
					advancingRect.x = static_cast<int>(baseX);
					advancingRect.y = static_cast<int>(baseY + overshoot);
					advancingRect.w = step.fromRect.w;
					advancingRect.h = step.fromRect.h;
					break;
				}
			}
		}

		// 缚魂锁移动插值渲染
		bool isFuhunsuoMovingThis = false;
		SDL_Rect fuhunsuoRect{};
		if (isFuhunsuoAnimating_) {
			float t = std::min(1.0f, fuhunsuoAnimTime_ / fuhunsuoAnimDuration_);
			float t_ease = (1.0f - std::cos(3.1415926f * t)) * 0.5f; // 缓动函数
			if (i == fuhunsuoToIndex_) {
				isFuhunsuoMovingThis = true;
				float baseX = fuhunsuoFromRect_.x + (fuhunsuoToRect_.x - fuhunsuoFromRect_.x) * t_ease;
				float baseY = fuhunsuoFromRect_.y + (fuhunsuoToRect_.y - fuhunsuoFromRect_.y) * t_ease;
				fuhunsuoRect.x = static_cast<int>(baseX);
				fuhunsuoRect.y = static_cast<int>(baseY);
				fuhunsuoRect.w = fuhunsuoFromRect_.w;
				fuhunsuoRect.h = fuhunsuoFromRect_.h;
			}
		}

		bool isMovingAnimating = false;
		if (isRushing_ && rushingCardIndex_ == i) isMovingAnimating = true;
		if (isBruteForcing_ && bruteForceCardIndex_ == i) isMovingAnimating = true;

		bool isPushedAnimating = false;
		if (isPushedAnimating_) {
			for (int cardIndex : pushedCardIndices_) {
				if (cardIndex == i) isPushedAnimating = true; break;
			}
		}

		if ((bfCard.isAlive || isAnimating || isFuhunsuoMovingThis) && !isMovingAnimating && !isPushedAnimating) {
			SDL_Rect renderRect;
			if (isAdvancingThis) {
				renderRect = advancingRect;
			} else if (isFuhunsuoMovingThis) {
				renderRect = fuhunsuoRect;
			} else {
				renderRect = bfCard.rect;
			}

			// 成长轻量动画：对待成长卡片做轻微放大与发光
			if (isGrowthAnimating_) {
				bool willGrowHere = false;
				for (const auto& gs : pendingGrowth_) { if (gs.index == i) { willGrowHere = true; break; } }
				if (willGrowHere) {
					float t = std::min(1.0f, growthAnimTime_ / growthAnimDuration_);
					float tEase = (1.0f - std::cos(3.1415926f * t)) * 0.5f;
					float scale = 1.0f + 0.12f * std::sin(tEase * 3.1415926f);
					int newW = static_cast<int>(renderRect.w * scale);
					int newH = static_cast<int>(renderRect.h * scale);
					renderRect.x += (renderRect.w - newW) / 2;
					renderRect.y += (renderRect.h - newH) / 2;
					renderRect.w = newW;
					renderRect.h = newH;
				}
			}

			if (isAnimating) {
				float scale = 1.0f - animProgress;
				scale = std::max(0.1f, scale);
				int newWidth = static_cast<int>(bfCard.rect.w * scale);
				int newHeight = static_cast<int>(bfCard.rect.h * scale);
				renderRect.x = bfCard.rect.x + (bfCard.rect.w - newWidth) / 2;
				renderRect.y = bfCard.rect.y + (bfCard.rect.h - newHeight) / 2;
				renderRect.w = newWidth;
				renderRect.h = newHeight;
			}

			// 检查是否是水袭卡牌且翻到反面
			bool isWaterAttackFlipped = hasMark(bfCard.card, u8"水袭") && waterAttackFlipped_[i];
			
			if (isWaterAttackFlipped) {
				// 水袭卡牌翻到反面：使用原牌面颜色，只显示水袭符号
				// 先正常渲染卡牌背景（使用CardRenderer的默认颜色）
				Card tempCard = bfCard.card;
				tempCard.health = bfCard.health;
				CardRenderer::renderCard(app, tempCard, renderRect, cardNameFont_, cardStatFont_, false);
				
				// 在卡牌上覆盖半透明遮罩，保持原色但隐藏信息
				SDL_SetRenderDrawColor(r, 0, 0, 0, 100);
				SDL_RenderFillRect(r, &renderRect);
				
				// 绘制水袭符号（波浪形），使用原字体颜色
				SDL_SetRenderDrawColor(r, 200, 200, 200, 255); // 使用原字体颜色
				int centerX = renderRect.x + renderRect.w / 2;
				int centerY = renderRect.y + renderRect.h / 2;
				int symbolSize = 40;
				
				// 绘制波浪符号
				for (int i = 0; i < symbolSize; i += 4) {
					int x = centerX - symbolSize/2 + i;
					int y1 = centerY - 8 + static_cast<int>(4 * std::sin(i * 0.3));
					int y2 = centerY + 8 + static_cast<int>(4 * std::sin(i * 0.3));
					SDL_RenderDrawLine(r, x, y1, x, y2);
				}
			}
			else {
				// 正常渲染卡牌
				Card tempCard = bfCard.card;
				tempCard.health = bfCard.health;
				if (isFuhunsuoMovingThis) {
					// 缚魂锁动画期间，使用目标卡牌的攻击力（已经是玩家控制）
				tempCard.attack = getDisplayAttackForIndex(i);
				} else {
					tempCard.attack = getDisplayAttackForIndex(i);
				}
				CardRenderer::renderCard(app, tempCard, renderRect, cardNameFont_, cardStatFont_, false);
			}

			// 显示献祭符号
			if (isSacrificing_ && bfCard.isPlayer && bfCard.isSacrificed) {
				SDL_SetRenderDrawColor(r, 255, 0, 0, 255);
				int centerX = renderRect.x + renderRect.w / 2;
				int centerY = renderRect.y + renderRect.h / 2;
				int symbolSize = 20;
				SDL_RenderDrawLine(r, centerX - symbolSize / 2, centerY - symbolSize / 2, centerX + symbolSize / 2, centerY + symbolSize / 2);
				SDL_RenderDrawLine(r, centerX + symbolSize / 2, centerY - symbolSize / 2, centerX - symbolSize / 2, centerY + symbolSize / 2);
				SDL_SetRenderDrawColor(r, 255, 100, 100, 200);
				SDL_Rect symbolRect{ centerX - symbolSize / 2 - 2, centerY - symbolSize / 2 - 2, symbolSize + 4, symbolSize + 4 };
				SDL_RenderDrawRect(r, &symbolRect);
			}
		}
		else {
			SDL_SetRenderDrawColor(r, 60, 70, 80, 80);
			SDL_RenderFillRect(r, &bfCard.rect);
			SDL_SetRenderDrawColor(r, 100, 110, 120, 150);
			SDL_RenderDrawRect(r, &bfCard.rect);
		}

		// 战场牌位悬停高亮（无论该格是否有牌）
		if (i == hoveredBattlefieldIndex_) {
			// 固定透明度的背景与描边（无脉动）
			const Uint8 bgAlpha = 25;
			const Uint8 innerAlpha = 70;
			const Uint8 outerAlpha = 35;
			// 背景半透明填充
			SDL_SetRenderDrawColor(r, 255, 240, 180, bgAlpha);
			SDL_RenderFillRect(r, &bfCard.rect);
			// 内描边
			SDL_SetRenderDrawColor(r, 255, 240, 180, innerAlpha);
			SDL_RenderDrawRect(r, &bfCard.rect);
			// 外描边
			SDL_Rect outerSlot = bfCard.rect; outerSlot.x -= 3; outerSlot.y -= 3; outerSlot.w += 6; outerSlot.h += 6;
			SDL_SetRenderDrawColor(r, 255, 220, 120, outerAlpha);
			SDL_RenderDrawRect(r, &outerSlot);
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
				}
				else {
					// 敌方攻击：向下移动
					renderRect.y += static_cast<int>(moveDistance);
				}

				// 使用CardRenderer渲染卡牌，使用当前血量
				Card tempCard = bfCard.card;
				tempCard.health = bfCard.health; // 使用当前血量
				tempCard.attack = getDisplayAttackForIndex(currentCardIndex);
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
					SDL_Rect effectRect = { centerX - i, centerY - i, i * 2, i * 2 };
					SDL_RenderDrawRect(r, &effectRect);
				}
			}
		}
	}

	// 第三遍：绘制特殊攻击的卡牌动画
	if (isSpecialAttackAnimating_ && currentTargetIndex_ < 8 && specialAttackTargets_[currentTargetIndex_] != -1 && currentAttackingIndex_ < static_cast<int>(attackingCards_.size())) {
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
				float deltaX = static_cast<float>(targetCenterX - attackerCenterX);
				float deltaY = static_cast<float>(targetCenterY - attackerCenterY);
				float distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);

				if (distance > 0) {
					// 归一化方向向量
					float dirX = deltaX / distance;
					float dirY = deltaY / distance;

					// 根据方向移动卡牌
					renderRect.x += static_cast<int>(moveDistance * dirX);
					renderRect.y += static_cast<int>(moveDistance * dirY);
				}
			}
			else {
				// 如果没有目标，使用默认的前向移动
				if (isPlayerAttacking_) {
					renderRect.y -= static_cast<int>(moveDistance);
				}
				else {
					renderRect.y += static_cast<int>(moveDistance);
				}
			}

			// 使用CardRenderer渲染卡牌
			Card tempCard = attacker.card;
			tempCard.health = attacker.health;
			tempCard.attack = getDisplayAttackForIndex(attackerIndex);
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
			case BattleState::AttackType::AllDirection:
				SDL_SetRenderDrawColor(r, 255, 0, 255, static_cast<Uint8>(255 * flashIntensity)); // 紫色 - 全向打击
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
				SDL_Rect effectRect = { centerX - i, centerY - i, i * 2, i * 2 };
				SDL_RenderDrawRect(r, &effectRect);
			}
		}
	}
	// 被推动卡牌动画渲染
	if (isPushedAnimating_ && !pushedCardIndices_.empty()) {
		for (size_t i = 0; i < pushedCardIndices_.size(); ++i) {
			int cardIndex = pushedCardIndices_[i];
			int direction = pushedDirections_[i];

			if (cardIndex >= 0 && cardIndex < TOTAL_BATTLEFIELD_SLOTS) {
				const auto& bfCard = battlefield_[cardIndex];
				if (bfCard.isAlive) {
					float pushedProgress = pushedAnimTime_ / pushedAnimDuration_;

					// 计算被推动动画效果
					SDL_Rect renderRect = bfCard.rect;

					// 被推动动画效果：卡牌向目标方向移动（线性移动，无回弹）
					float moveDistance = 120.0f * pushedProgress;
					renderRect.x += static_cast<int>(moveDistance * direction);

					// 闪烁效果
					float flashIntensity = 0.6f + 0.4f * std::sin(pushedProgress * 6.0f);

					// 使用CardRenderer渲染卡牌，使用当前血量
					Card tempCard = bfCard.card;
					tempCard.health = bfCard.health; // 使用当前血量
					CardRenderer::renderCard(app, tempCard, renderRect, cardNameFont_, cardStatFont_, false);

					// 被推动卡牌的特效：蓝色边框
					SDL_SetRenderDrawColor(r, 100, 150, 255, static_cast<Uint8>(255 * flashIntensity));
					SDL_RenderDrawRect(r, &renderRect);

					// 添加外圈高亮效果
					SDL_SetRenderDrawColor(r, 50, 100, 255, static_cast<Uint8>(128 * flashIntensity));
					SDL_Rect outerRect = renderRect;
					outerRect.x -= 2;
					outerRect.y -= 2;
					outerRect.w += 4;
					outerRect.h += 4;
					SDL_RenderDrawRect(r, &outerRect);

					// 添加推动特效：从卡牌中心向外扩散的圆圈
					int centerX = renderRect.x + renderRect.w / 2;
					int centerY = renderRect.y + renderRect.h / 2;
					int effectRadius = static_cast<int>(25.0f * pushedProgress);

					SDL_SetRenderDrawColor(r, 100, 200, 255, static_cast<Uint8>(180 * (1.0f - pushedProgress)));
					for (int j = 0; j < effectRadius; j += 3) {
						SDL_Rect effectRect = { centerX - j, centerY - j, j * 2, j * 2 };
						SDL_RenderDrawRect(r, &effectRect);
					}

					// 添加移动轨迹效果
					int trailLength = static_cast<int>(30.0f * pushedProgress);
					SDL_SetRenderDrawColor(r, 150, 200, 255, static_cast<Uint8>(120 * (1.0f - pushedProgress)));
					for (int j = 0; j < trailLength; j += 5) {
						int trailX = centerX - (j * direction);
						SDL_Rect trailRect = { trailX - 2, centerY - 2, 4, 4 };
						SDL_RenderFillRect(r, &trailRect);
					}
				}
			}
		}
	}

	// 冲刺能手动画渲染（最后渲染，确保在最上层）
	if (isRushing_ && rushingCardIndex_ >= 0 && rushingCardIndex_ < TOTAL_BATTLEFIELD_SLOTS) {
		const auto& bfCard = battlefield_[rushingCardIndex_];
		if (bfCard.isAlive) {
			float rushProgress = rushingAnimTime_ / rushingAnimDuration_;

			// 计算冲刺能手动画效果
			SDL_Rect renderRect = bfCard.rect;

			// 声明变量
			float flashIntensity;
			SDL_Rect shakeRect;

			// 检查是否是摇晃动画
			if (isRushingShaking_) {
				// 摇晃动画：不移动，只有震动效果
				// 闪烁效果
				flashIntensity = 0.5f + 0.5f * std::sin(rushProgress * 8.0f);

				// 摇晃特效：红色边框，震动效果
				float shakeIntensity = 0.3f + 0.7f * std::sin(rushProgress * 16.0f);
				shakeRect = renderRect;
				shakeRect.x += static_cast<int>(shakeIntensity * 3.0f * (rand() % 3 - 1));
				shakeRect.y += static_cast<int>(shakeIntensity * 2.0f * (rand() % 3 - 1));
			}
			else {
				// 正常移动动画：卡牌向目标方向移动（线性移动，无回弹）
				float moveDistance = 120.0f * rushProgress;
				renderRect.x += static_cast<int>(moveDistance * rushingDirection_);

				// 闪烁效果
				flashIntensity = 0.5f + 0.5f * std::sin(rushProgress * 8.0f);

				// 冲刺能手特效：红色边框，震动效果
				float shakeIntensity = 0.3f + 0.7f * std::sin(rushProgress * 16.0f);
				shakeRect = renderRect;
				shakeRect.x += static_cast<int>(shakeIntensity * 3.0f * (rand() % 3 - 1));
				shakeRect.y += static_cast<int>(shakeIntensity * 2.0f * (rand() % 3 - 1));
			}

			// 使用CardRenderer渲染卡牌，使用当前血量
			Card tempCard = bfCard.card;
			tempCard.health = bfCard.health; // 使用当前血量
			CardRenderer::renderCard(app, tempCard, renderRect, cardNameFont_, cardStatFont_, false);

			// 添加冲刺能手特效：红色边框
			SDL_SetRenderDrawColor(r, 255, 100, 100, static_cast<Uint8>(255 * flashIntensity));
			SDL_RenderDrawRect(r, &shakeRect);

			// 添加外圈高亮效果
			SDL_SetRenderDrawColor(r, 255, 50, 50, static_cast<Uint8>(128 * flashIntensity));
			SDL_Rect outerRect = shakeRect;
			outerRect.x -= 3;
			outerRect.y -= 3;
			outerRect.w += 6;
			outerRect.h += 6;
			SDL_RenderDrawRect(r, &outerRect);

			// 添加冲刺特效：从卡牌中心向外扩散的圆圈
			int centerX = renderRect.x + renderRect.w / 2;
			int centerY = renderRect.y + renderRect.h / 2;
			int effectRadius = static_cast<int>(30.0f * rushProgress);

			SDL_SetRenderDrawColor(r, 255, 150, 100, static_cast<Uint8>(200 * (1.0f - rushProgress)));
			for (int i = 0; i < effectRadius; i += 2) {
				SDL_Rect effectRect = { centerX - i, centerY - i, i * 2, i * 2 };
				SDL_RenderDrawRect(r, &effectRect);
			}

			// 检查是否是摇晃动画
			if (!isRushingShaking_) {
				// 只有正常移动动画才显示移动轨迹

				// 添加移动轨迹效果
				int trailLength = static_cast<int>(40.0f * rushProgress);
				SDL_SetRenderDrawColor(r, 255, 200, 100, static_cast<Uint8>(150 * (1.0f - rushProgress)));
				for (int i = 0; i < trailLength; i += 4) {
					int trailX = centerX - (i * rushingDirection_);
					SDL_Rect trailRect = { trailX - 2, centerY - 2, 4, 4 };
					SDL_RenderFillRect(r, &trailRect);
				}
			}
		}
	}

	// 蛮力冲撞动画渲染（最后渲染，确保在最上层）
	if (isBruteForcing_ && bruteForceCardIndex_ >= 0 && bruteForceCardIndex_ < TOTAL_BATTLEFIELD_SLOTS) {
		const auto& bfCard = battlefield_[bruteForceCardIndex_];
		if (bfCard.isAlive) {
			float bruteForceProgress = bruteForceAnimTime_ / bruteForceAnimDuration_;

			// 计算蛮力冲撞动画效果
			SDL_Rect renderRect = bfCard.rect;

			// 声明变量
			float flashIntensity;
			SDL_Rect shakeRect;

			// 检查是否是摇晃动画
			if (isBruteForceShaking_) {
				// 摇晃动画：不移动，只有震动效果
				// 闪烁效果
				flashIntensity = 0.5f + 0.5f * std::sin(bruteForceProgress * 8.0f);

				// 摇晃特效：橙色边框，震动效果
				float shakeIntensity = 0.3f + 0.7f * std::sin(bruteForceProgress * 16.0f);
				shakeRect = renderRect;
				shakeRect.x += static_cast<int>(shakeIntensity * 3.0f * (rand() % 3 - 1));
				shakeRect.y += static_cast<int>(shakeIntensity * 2.0f * (rand() % 3 - 1));
			}
			else {
				// 正常推动动画：卡牌向目标方向移动（线性移动，无回弹）
				float moveDistance = 120.0f * bruteForceProgress;
				renderRect.x += static_cast<int>(moveDistance * bruteForceDirection_);

				// 闪烁效果
				flashIntensity = 0.5f + 0.5f * std::sin(bruteForceProgress * 8.0f);

				// 蛮力冲撞特效：橙色边框，震动效果
				float shakeIntensity = 0.3f + 0.7f * std::sin(bruteForceProgress * 16.0f);
				shakeRect = renderRect;
				shakeRect.x += static_cast<int>(shakeIntensity * 3.0f * (rand() % 3 - 1));
				shakeRect.y += static_cast<int>(shakeIntensity * 2.0f * (rand() % 3 - 1));
			}

			// 使用CardRenderer渲染卡牌，使用当前血量
			Card tempCard = bfCard.card;
			tempCard.health = bfCard.health; // 使用当前血量
			CardRenderer::renderCard(app, tempCard, renderRect, cardNameFont_, cardStatFont_, false);

			SDL_SetRenderDrawColor(r, 255, 150, 50, static_cast<Uint8>(255 * flashIntensity));
			SDL_RenderDrawRect(r, &shakeRect);

			// 添加外圈高亮效果
			SDL_SetRenderDrawColor(r, 255, 100, 0, static_cast<Uint8>(128 * flashIntensity));
			SDL_Rect outerRect = shakeRect;
			outerRect.x -= 4;
			outerRect.y -= 4;
			outerRect.w += 8;
			outerRect.h += 8;
			SDL_RenderDrawRect(r, &outerRect);

			// 添加推动特效：从卡牌中心向外扩散的圆圈
			int centerX = renderRect.x + renderRect.w / 2;
			int centerY = renderRect.y + renderRect.h / 2;
			int effectRadius = static_cast<int>(30.0f * bruteForceProgress);

			SDL_SetRenderDrawColor(r, 255, 200, 100, static_cast<Uint8>(200 * (1.0f - bruteForceProgress)));
			for (int i = 0; i < effectRadius; i += 2) {
				SDL_Rect effectRect = { centerX - i, centerY - i, i * 2, i * 2 };
				SDL_RenderDrawRect(r, &effectRect);
			}

			// 检查是否是摇晃动画
			if (!isBruteForceShaking_) {
				// 只有正常推动动画才显示移动轨迹和方向箭头

				// 添加移动轨迹效果
				int trailLength = static_cast<int>(40.0f * bruteForceProgress);
				SDL_SetRenderDrawColor(r, 255, 200, 100, static_cast<Uint8>(150 * (1.0f - bruteForceProgress)));
				for (int i = 0; i < trailLength; i += 4) {
					int trailX = centerX - (i * bruteForceDirection_);
					SDL_Rect trailRect = { trailX - 2, centerY - 2, 4, 4 };
					SDL_RenderFillRect(r, &trailRect);
				}

				// 添加方向箭头
				SDL_SetRenderDrawColor(r, 255, 150, 50, static_cast<Uint8>(255 * flashIntensity));
				int arrowX = shakeRect.x + (bruteForceDirection_ > 0 ? shakeRect.w - 10 : 10);
				int arrowY = shakeRect.y + shakeRect.h / 2;

				// 绘制箭头
				if (bruteForceDirection_ > 0) {
					// 向右箭头
					SDL_RenderDrawLine(r, arrowX, arrowY, arrowX + 8, arrowY);
					SDL_RenderDrawLine(r, arrowX + 6, arrowY - 2, arrowX + 8, arrowY);
					SDL_RenderDrawLine(r, arrowX + 6, arrowY + 2, arrowX + 8, arrowY);
				}
				else {
					// 向左箭头
					SDL_RenderDrawLine(r, arrowX, arrowY, arrowX - 8, arrowY);
					SDL_RenderDrawLine(r, arrowX - 6, arrowY - 2, arrowX - 8, arrowY);
					SDL_RenderDrawLine(r, arrowX - 6, arrowY + 2, arrowX - 8, arrowY);
				}
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
	bool hasAllDirection = false;

	// 检查所有印记
	for (const auto& mark : card.marks) {
		if (mark == u8"三向攻击") {
			hasTriple = true;
		}
		else if (mark == u8"双向攻击") {
			hasDouble = true;
		}
		else if (mark == u8"双重攻击") {
			hasTwice = true;
		}
		else if (mark == u8"全向打击") {
			hasAllDirection = true;
		}
	}

	// 按优先级返回攻击类型（组合攻击优先）
	if (hasAllDirection) {
		return BattleState::AttackType::AllDirection;
	}
	else if (hasTriple && hasTwice) {
		return BattleState::AttackType::TripleTwice;
	}
	else if (hasDouble && hasTwice) {
		return BattleState::AttackType::DoubleTwice;
	}
	else if (hasTriple) {
		return BattleState::AttackType::Triple;
	}
	else if (hasDouble) {
		return BattleState::AttackType::Double;
	}
	else if (hasTwice) {
		return BattleState::AttackType::Twice;
	}

	return BattleState::AttackType::Normal;
}

// 检查卡牌是否有特定印记
bool BattleState::hasMark(const Card& card, const std::string& mark) const {
	for (const auto& cardMark : card.marks) {
		if (cardMark == mark) {
			return true;
		}
	}
	return false;
}

// 检查多向攻击是否至少有一个方向可以攻击
bool BattleState::canMultiDirectionAttack(int attackerIndex, int targetCol, bool isPlayerAttacking) {
	const auto& attacker = battlefield_[attackerIndex];
	BattleState::AttackType attackType = getCardAttackType(attacker.card);
	
	// 检查各个攻击方向是否至少有一个可以攻击
	switch (attackType) {
	case BattleState::AttackType::Double: {
		// 双向攻击：检查左右斜对位
		bool canAttackLeft = false, canAttackRight = false;
		
		// 左斜对位
		if (targetCol > 0) {
			int leftTargetIndex = isPlayerAttacking ?
				(1 * BATTLEFIELD_COLS + (targetCol - 1)) :
				(2 * BATTLEFIELD_COLS + (targetCol - 1));
			if (leftTargetIndex >= 0 && leftTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
				// 空位也视为可攻击（攻击本体），仅当有存活且带厌恶情绪时视为该方向不可攻
				if (!battlefield_[leftTargetIndex].isAlive || !hasMark(battlefield_[leftTargetIndex].card, std::string(u8"厌恶情绪"))) {
					canAttackLeft = true;
				}
			}
		}
		
		// 右斜对位
		if (targetCol < BATTLEFIELD_COLS - 1) {
			int rightTargetIndex = isPlayerAttacking ?
				(1 * BATTLEFIELD_COLS + (targetCol + 1)) :
				(2 * BATTLEFIELD_COLS + (targetCol + 1));
			if (rightTargetIndex >= 0 && rightTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
				// 空位也视为可攻击（攻击本体），仅当有存活且带厌恶情绪时视为该方向不可攻
				if (!battlefield_[rightTargetIndex].isAlive || !hasMark(battlefield_[rightTargetIndex].card, std::string(u8"厌恶情绪"))) {
					canAttackRight = true;
				}
			}
		}
		
		return canAttackLeft || canAttackRight;
	}
	case BattleState::AttackType::Triple: {
		// 三向攻击：检查对位+左右斜对位
		bool canAttackLeft = false, canAttackFront = false, canAttackRight = false;
		
		// 左斜对位
		if (targetCol > 0) {
			int leftTargetIndex = isPlayerAttacking ?
				(1 * BATTLEFIELD_COLS + (targetCol - 1)) :
				(2 * BATTLEFIELD_COLS + (targetCol - 1));
			if (leftTargetIndex >= 0 && leftTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
				if (!battlefield_[leftTargetIndex].isAlive || !hasMark(battlefield_[leftTargetIndex].card, std::string(u8"厌恶情绪"))) {
					canAttackLeft = true;
				}
			}
		}
		
		// 对位
		int frontTargetIndex = isPlayerAttacking ?
			(1 * BATTLEFIELD_COLS + targetCol) :
			(2 * BATTLEFIELD_COLS + targetCol);
		if (frontTargetIndex >= 0 && frontTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
			if (!battlefield_[frontTargetIndex].isAlive || !hasMark(battlefield_[frontTargetIndex].card, std::string(u8"厌恶情绪"))) {
				canAttackFront = true;
			}
		}
		
		// 右斜对位
		if (targetCol < BATTLEFIELD_COLS - 1) {
			int rightTargetIndex = isPlayerAttacking ?
				(1 * BATTLEFIELD_COLS + (targetCol + 1)) :
				(2 * BATTLEFIELD_COLS + (targetCol + 1));
			if (rightTargetIndex >= 0 && rightTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
				if (!battlefield_[rightTargetIndex].isAlive || !hasMark(battlefield_[rightTargetIndex].card, std::string(u8"厌恶情绪"))) {
					canAttackRight = true;
				}
			}
		}
		
		return canAttackLeft || canAttackFront || canAttackRight;
	}
	case BattleState::AttackType::AllDirection: {
		// 全向打击：检查对面所有位置
		if (isPlayerAttacking) {
			// 玩家攻击：检查敌方前排的四个位置
			for (int col = 0; col < 4; ++col) {
				int enemyFrontIndex = 1 * BATTLEFIELD_COLS + col;
				if (enemyFrontIndex >= 0 && enemyFrontIndex < TOTAL_BATTLEFIELD_SLOTS) {
					if (!battlefield_[enemyFrontIndex].isAlive || !hasMark(battlefield_[enemyFrontIndex].card, std::string(u8"厌恶情绪"))) {
						return true; // 至少有一个方向可以攻击（空位也允许，视为打本体）
					}
				}
			}
		} else {
			// 敌方攻击：检查玩家前排的四个位置
			for (int col = 0; col < 4; ++col) {
				int playerIndex = 2 * BATTLEFIELD_COLS + col;
				if (playerIndex >= 0 && playerIndex < TOTAL_BATTLEFIELD_SLOTS) {
					if (!battlefield_[playerIndex].isAlive || !hasMark(battlefield_[playerIndex].card, std::string(u8"厌恶情绪"))) {
						return true; // 至少有一个方向可以攻击（空位也允许，视为打本体）
					}
				}
			}
		}
		return false;
	}
	case BattleState::AttackType::Twice:
	case BattleState::AttackType::DoubleTwice:
	case BattleState::AttackType::TripleTwice: {
		// 双重攻击类：检查对位即可（双重攻击只攻击对位）
		int frontTargetIndex = isPlayerAttacking ?
			(1 * BATTLEFIELD_COLS + targetCol) :
			(2 * BATTLEFIELD_COLS + targetCol);
		if (frontTargetIndex >= 0 && frontTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
			// 空位也允许（攻击本体），仅当有存活且带厌恶情绪时禁止
			if (!battlefield_[frontTargetIndex].isAlive) return true;
			return !hasMark(battlefield_[frontTargetIndex].card, std::string(u8"厌恶情绪"));
		}
		return false;
	}
	default:
		return false;
	}
}

// 随机印记效果：删去随机印记并添加任意一个印记
void BattleState::applyRandomMarkEffect(Card& card) {
	// 检查是否有随机印记
	if (!hasMark(card, std::string(u8"随机"))) {
		return;
	}
	
	// 移除随机印记
	auto it = std::find(card.marks.begin(), card.marks.end(), std::string(u8"随机"));
	if (it != card.marks.end()) {
		card.marks.erase(it);
	}
	
	// 定义可选的印记列表
	std::vector<std::string> availableMarks = {
		u8"空袭", u8"水袭", u8"高跳", u8"护主", u8"领袖力量", u8"掘墓人",
		u8"双重攻击", u8"双向攻击", u8"三向攻击", u8"冲刺能手", u8"蛮力冲撞",
		u8"生生不息", u8"形态转换", u8"不死印记", u8"消耗骨头", u8"优质祭品",
		u8"内心之蜂", u8"滋生寄生虫", u8"断尾求生", u8"反伤", u8"死神之触",
		u8"令人生厌", u8"臭臭", u8"蚂蚁", u8"蚁后", u8"一口之量", u8"坚硬之躯",
		u8"兔窝", u8"筑坝师", u8"堤坝附带印记", u8"继承印记", u8"检索", u8"磐石之身", u8"半根骨头", u8"献祭之血", u8"厌恶情绪", u8"铁兽夹", u8"全向打击", u8"道具商"
	};
	
	// 随机选择一个印记
	if (!availableMarks.empty()) {
		int randomIndex = rand() % availableMarks.size();
		card.marks.push_back(availableMarks[randomIndex]);
	}
}

// 计算用于展示的攻击力（受对位"臭臭/令人生厌"临时影响）
int BattleState::getDisplayAttackForIndex(int battlefieldIndex) const {
	if (battlefieldIndex < 0 || battlefieldIndex >= TOTAL_BATTLEFIELD_SLOTS) return 0;
	const auto& self = battlefield_[battlefieldIndex];
	if (!self.isAlive) return 0;
	int display = self.card.attack;
	// 查找对位目标
	int row = battlefieldIndex / BATTLEFIELD_COLS;
	int col = battlefieldIndex % BATTLEFIELD_COLS;
	int opposeIndex = -1;
	if (row == 2) opposeIndex = 1 * BATTLEFIELD_COLS + col; // 我方对位敌方
	else if (row == 1) opposeIndex = 2 * BATTLEFIELD_COLS + col; // 敌方对位我方
	else if (row == 0) opposeIndex = 1 * BATTLEFIELD_COLS + col; // 顶行对位第二行（通常为空）
	if (opposeIndex >= 0 && opposeIndex < TOTAL_BATTLEFIELD_SLOTS) {
		const auto& opp = battlefield_[opposeIndex];
		if (opp.isAlive) {
			// 磐石之身印记：免疫臭臭和令人生厌效果
			if (!hasMark(self.card, std::string(u8"磐石之身"))) {
			if (hasMark(opp.card, std::string(u8"臭臭"))) display -= 1;
			if (hasMark(opp.card, std::string(u8"令人生厌"))) display += 1;
			}
		}
	}

	// 领袖力量：同一行左右相邻的友方每有一张带该印记的卡牌，+1攻击
	int leftCol = col - 1;
	int rightCol = col + 1;
	if (leftCol >= 0) {
		int leftIndex = row * BATTLEFIELD_COLS + leftCol;
		if (leftIndex >= 0 && leftIndex < TOTAL_BATTLEFIELD_SLOTS) {
			const auto& leftCard = battlefield_[leftIndex];
			if (leftCard.isAlive && leftCard.isPlayer == self.isPlayer && hasMark(leftCard.card, std::string(u8"领袖力量"))) {
				display += 1;
			}
		}
	}
	if (rightCol < BATTLEFIELD_COLS) {
		int rightIndex = row * BATTLEFIELD_COLS + rightCol;
		if (rightIndex >= 0 && rightIndex < TOTAL_BATTLEFIELD_SLOTS) {
			const auto& rightCard = battlefield_[rightIndex];
			if (rightCard.isAlive && rightCard.isPlayer == self.isPlayer && hasMark(rightCard.card, std::string(u8"领袖力量"))) {
				display += 1;
			}
		}
	}

	// 手牌数印记：攻击力等于当前玩家的手牌数
	if (hasMark(self.card, std::string(u8"手牌数"))) {
		display = static_cast<int>(handCards_.size());
	}

	// 半根骨头印记：攻击力加上当前魂骨数的一半（向下取整）
	if (hasMark(self.card, std::string(u8"半根骨头"))) {
		display += boneCount_ / 2;
	}

	// 献祭之血印记：攻击力加上本回合献祭次数
	if (hasMark(self.card, std::string(u8"献祭之血"))) {
		display += sacrificeCountThisTurn_;
	}

	// 蚂蚁印记：攻击力等于同侧场上所有带"蚂蚁"印记的存活单位数量
	if (hasMark(self.card, std::string(u8"蚂蚁"))) {
		int antCount = 0;
		for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
			const auto& bf = battlefield_[i];
			if (!bf.isAlive) continue;
			if (bf.isPlayer != self.isPlayer) continue;
			if (hasMark(bf.card, std::string(u8"蚂蚁"))) antCount++;
		}
		display = antCount;
	}

	// 镜像印记：攻击力等于对位卡牌的攻击力
	if (hasMark(self.card, std::string(u8"镜像"))) {
		if (opposeIndex >= 0 && opposeIndex < TOTAL_BATTLEFIELD_SLOTS) {
			const auto& opp = battlefield_[opposeIndex];
			if (opp.isAlive) {
				display = getDisplayAttackForIndex(opposeIndex);
			} else {
				display = 0; // 对位为空时攻击力为0
			}
		} else {
			display = 0; // 没有对位时攻击力为0
		}
	}

	// 铃铛距离印记：根据位置从左到右攻击力为4、3、2、1
	if (hasMark(self.card, std::string(u8"铃铛距离"))) {
		// 计算在行内的列位置（0-3）
		int col = battlefieldIndex % BATTLEFIELD_COLS;
		// 从左到右：位置0=4攻击力，位置1=3攻击力，位置2=2攻击力，位置3=1攻击力
		display = 4 - col;
	}

	if (display < 0) display = 0;
	return display;
}

// 执行特殊攻击
void BattleState::executeSpecialAttack(int attackerIndex, int targetCol, bool isPlayerAttacking) {
	const auto& attacker = battlefield_[attackerIndex];
	BattleState::AttackType attackType = getCardAttackType(attacker.card);

	// 如果是普通攻击，直接执行（以临时攻击力为准）
	if (attackType == BattleState::AttackType::Normal) {
		int tempAtk = getDisplayAttackForIndex(attackerIndex);
		if (tempAtk > 0) {
			executeNormalAttack(attackerIndex, targetCol, isPlayerAttacking, tempAtk);
		}
		return;
	}

	// 设置特殊攻击动画
	currentAttackType_ = attackType;
	isSpecialAttackAnimating_ = true;
	currentTargetIndex_ = 0;
	attackAnimTime_ = 0.0f;

	// 清空目标数组
	for (int i = 0; i < 8; ++i) {
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
	case BattleState::AttackType::AllDirection: {
		// 全向打击：设置对面全部卡牌目标
		setupAllDirectionTargets(attackerIndex, targetCol, isPlayerAttacking);
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
		
		if (leftTargetIndex >= 0 && leftTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
		specialAttackTargets_[targetCount++] = leftTargetIndex;
		}
	}

	// 右斜对位
	if (targetCol < BATTLEFIELD_COLS - 1) {
		int rightTargetIndex = isPlayerAttacking ?
			(1 * BATTLEFIELD_COLS + (targetCol + 1)) :
			(2 * BATTLEFIELD_COLS + (targetCol + 1));
		
		if (rightTargetIndex >= 0 && rightTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
		specialAttackTargets_[targetCount++] = rightTargetIndex;
		}
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
		
		if (leftTargetIndex >= 0 && leftTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
		specialAttackTargets_[targetCount++] = leftTargetIndex;
		}
	}

	// 2. 对位
	int frontTargetIndex = isPlayerAttacking ?
		(1 * BATTLEFIELD_COLS + targetCol) :
		(2 * BATTLEFIELD_COLS + targetCol);
	
	if (frontTargetIndex >= 0 && frontTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
	specialAttackTargets_[targetCount++] = frontTargetIndex;
	}

	// 3. 右斜对位
	if (targetCol < BATTLEFIELD_COLS - 1) {
		int rightTargetIndex = isPlayerAttacking ?
			(1 * BATTLEFIELD_COLS + (targetCol + 1)) :
			(2 * BATTLEFIELD_COLS + (targetCol + 1));
		
		if (rightTargetIndex >= 0 && rightTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
		specialAttackTargets_[targetCount++] = rightTargetIndex;
		}
	}
}

// 设置双重攻击目标
void BattleState::setupTwiceTargets(int attackerIndex, int targetCol, bool isPlayerAttacking) {
	int targetIndex = isPlayerAttacking ?
		(1 * BATTLEFIELD_COLS + targetCol) :
		(2 * BATTLEFIELD_COLS + targetCol);

	if (targetIndex >= 0 && targetIndex < TOTAL_BATTLEFIELD_SLOTS) {
	// 同一个目标攻击两次
	specialAttackTargets_[0] = targetIndex;
	specialAttackTargets_[1] = targetIndex;
	}
}

// 设置双向双重攻击目标
void BattleState::setupDoubleTwiceTargets(int attackerIndex, int targetCol, bool isPlayerAttacking) {
	int targetCount = 0;

	// 左斜对位攻击一次
	if (targetCol > 0) {
		int leftTargetIndex = isPlayerAttacking ?
			(1 * BATTLEFIELD_COLS + (targetCol - 1)) :
			(2 * BATTLEFIELD_COLS + (targetCol - 1));
		
		if (leftTargetIndex >= 0 && leftTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
		specialAttackTargets_[targetCount++] = leftTargetIndex;
		}
	}

	// 右斜对位攻击一次
	if (targetCol < BATTLEFIELD_COLS - 1) {
		int rightTargetIndex = isPlayerAttacking ?
			(1 * BATTLEFIELD_COLS + (targetCol + 1)) :
			(2 * BATTLEFIELD_COLS + (targetCol + 1));
		
		if (rightTargetIndex >= 0 && rightTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
		specialAttackTargets_[targetCount++] = rightTargetIndex;
	}
	}

	// 对位攻击两次
	int frontTargetIndex = isPlayerAttacking ?
		(1 * BATTLEFIELD_COLS + targetCol) :
		(2 * BATTLEFIELD_COLS + targetCol);
	
	if (frontTargetIndex >= 0 && frontTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
		specialAttackTargets_[targetCount++] = frontTargetIndex;
		specialAttackTargets_[targetCount++] = frontTargetIndex;
	}

}

// 设置三向双重攻击目标
void BattleState::setupTripleTwiceTargets(int attackerIndex, int targetCol, bool isPlayerAttacking) {
	int targetCount = 0;

	// 三向双重攻击顺序：左斜对位 → 对位 → 右斜对位（每个目标攻击两次）

	// 1. 左斜对位攻击一次
	if (targetCol > 0) {
		int leftTargetIndex = isPlayerAttacking ?
			(1 * BATTLEFIELD_COLS + (targetCol - 1)) :
			(2 * BATTLEFIELD_COLS + (targetCol - 1));
		
		if (leftTargetIndex >= 0 && leftTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
		specialAttackTargets_[targetCount++] = leftTargetIndex;
		}
	}

	// 2. 对位攻击两次
	int frontTargetIndex = isPlayerAttacking ?
		(1 * BATTLEFIELD_COLS + targetCol) :
		(2 * BATTLEFIELD_COLS + targetCol);
	
	if (frontTargetIndex >= 0 && frontTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
	specialAttackTargets_[targetCount++] = frontTargetIndex;
	specialAttackTargets_[targetCount++] = frontTargetIndex;
	}

	// 3. 右斜对位攻击一次
	if (targetCol < BATTLEFIELD_COLS - 1) {
		int rightTargetIndex = isPlayerAttacking ?
			(1 * BATTLEFIELD_COLS + (targetCol + 1)) :
			(2 * BATTLEFIELD_COLS + (targetCol + 1));
		
		if (rightTargetIndex >= 0 && rightTargetIndex < TOTAL_BATTLEFIELD_SLOTS) {
		specialAttackTargets_[targetCount++] = rightTargetIndex;
		}
	}
}

// 设置全向打击目标
void BattleState::setupAllDirectionTargets(int attackerIndex, int targetCol, bool isPlayerAttacking) {
	int targetCount = 0;
	
	// 全向打击：攻击对面四个位置
	if (isPlayerAttacking) {
		// 玩家攻击：攻击敌方前排（第二行）的四个位置
		for (int col = 0; col < 4; ++col) {
			int enemyFrontIndex = 1 * BATTLEFIELD_COLS + col;
			if (enemyFrontIndex >= 0 && enemyFrontIndex < TOTAL_BATTLEFIELD_SLOTS) {
				specialAttackTargets_[targetCount++] = enemyFrontIndex;
			}
		}
	} else {
		// 敌方攻击：攻击玩家前排（第三行）的四个位置
		for (int col = 0; col < 4; ++col) {
			int playerIndex = 2 * BATTLEFIELD_COLS + col;
			if (playerIndex >= 0 && playerIndex < TOTAL_BATTLEFIELD_SLOTS) {
				specialAttackTargets_[targetCount++] = playerIndex;
			}
		}
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
    // 注意：target 在断尾求生后会变化，延后获取

	// 检查攻击者是否有空袭印记
	bool hasAirStrike = hasMark(attacker.card, u8"空袭");

	// 断尾求生：被攻击前检查是否可以逃跑，让尾巴承伤
	// 空袭单位不会触发断尾求生，除非目标自己有高跳
	if (battlefield_[targetIndex].isAlive && hasMark(battlefield_[targetIndex].card, std::string(u8"断尾求生"))) {
		// 检查攻击者是否有空袭印记
		bool attackerHasAirStrike = hasMark(attacker.card, std::string(u8"空袭"));
		// 检查目标是否有高跳印记
		bool targetHasHighJump = hasMark(battlefield_[targetIndex].card, std::string(u8"高跳"));
		
		// 如果攻击者有空袭且目标没有高跳，不触发断尾求生
		if (attackerHasAirStrike && !targetHasHighJump) {
			// 不触发断尾求生，继续正常攻击流程
		} else {
		// 寻找同行空位
		int targetRow = targetIndex / BATTLEFIELD_COLS;
		int targetCol = targetIndex % BATTLEFIELD_COLS;
		int newIndex = -1;
		
		// 检查同行相邻空位（左右各一个）
		// 检查左边
		if (targetCol > 0) {
			int leftIndex = targetRow * BATTLEFIELD_COLS + (targetCol - 1);
			if (leftIndex >= 0 && leftIndex < TOTAL_BATTLEFIELD_SLOTS && !battlefield_[leftIndex].isAlive) {
				newIndex = leftIndex;
			}
		}
		// 如果左边没有空位，检查右边
		if (newIndex == -1 && targetCol < BATTLEFIELD_COLS - 1) {
			int rightIndex = targetRow * BATTLEFIELD_COLS + (targetCol + 1);
			if (rightIndex >= 0 && rightIndex < TOTAL_BATTLEFIELD_SLOTS && !battlefield_[rightIndex].isAlive) {
				newIndex = rightIndex;
			}
		}
		
		// 如果找到空位，执行断尾求生
		if (newIndex != -1) {
			// 移动本体到新位置
			SDL_Rect targetRect = battlefield_[newIndex].rect;
			battlefield_[newIndex] = battlefield_[targetIndex];
			battlefield_[newIndex].rect = targetRect;
			
			// 删除断尾求生印记
			auto& movedCard = battlefield_[newIndex].card;
			auto it = std::find(movedCard.marks.begin(), movedCard.marks.end(), std::string(u8"断尾求生"));
			if (it != movedCard.marks.end()) {
				movedCard.marks.erase(it);
			}
			
			// 在原位置生成断尾
			Card tailCard = CardDB::instance().make("tail_segment");
			if (!tailCard.id.empty()) {
				battlefield_[targetIndex].card = tailCard;
				battlefield_[targetIndex].isAlive = true;
				battlefield_[targetIndex].health = tailCard.health;
				battlefield_[targetIndex].isPlayer = battlefield_[newIndex].isPlayer;
				battlefield_[targetIndex].placedTurn = currentTurn_;
				battlefield_[targetIndex].oneTurnGrowthApplied = false;
			}
			
			statusMessage_ = "断尾求生！本体逃跑，尾巴承伤！";
		}
		}
	}

	// 断尾后再获取最新的 target 引用
	const auto& target = battlefield_[targetIndex];

	// 检查目标是否有效
	if (!target.isAlive) {
		// 守护者印记：空位被攻击时，守护者移动到该位置承伤
		bool guardianMoved = false;
		if (attacker.isPlayer) {
			// 玩家攻击敌方空位，寻找敌方守护者
			for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
				if (battlefield_[i].isAlive && !battlefield_[i].isPlayer && hasMark(battlefield_[i].card, std::string(u8"守护者"))) {
					// 找到敌方守护者，移动到空位承伤
					SDL_Rect targetRect = battlefield_[targetIndex].rect;
					battlefield_[targetIndex] = battlefield_[i];
					battlefield_[targetIndex].rect = targetRect;
					battlefield_[i].isAlive = false;
					guardianMoved = true;
					statusMessage_ = std::string("守护者：") + battlefield_[targetIndex].card.name + " 移动到空位承伤！";
					break;
				}
			}
		} else {
			// 敌方攻击玩家空位，寻找玩家守护者
			for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
				if (battlefield_[i].isAlive && battlefield_[i].isPlayer && hasMark(battlefield_[i].card, std::string(u8"守护者"))) {
					// 找到玩家守护者，移动到空位承伤
					SDL_Rect targetRect = battlefield_[targetIndex].rect;
					battlefield_[targetIndex] = battlefield_[i];
					battlefield_[targetIndex].rect = targetRect;
					battlefield_[i].isAlive = false;
					guardianMoved = true;
					statusMessage_ = std::string("守护者：") + battlefield_[targetIndex].card.name + " 移动到空位承伤！";
					break;
				}
			}
		}
		
		// 如果守护者移动了，继续攻击流程；否则攻击本体
		if (!guardianMoved) {
		// 目标已死亡，攻击敌方本体
		if (attacker.isPlayer) {
                // 扣除敌方血量，并按溢出计入文脉
                int before = enemyHealth_;
                enemyHealth_ -= damage;
				if (enemyHealth_ < 0) enemyHealth_ = 0;
				
				// 更新墨尺：直接增加伤害值
				meterActualPos_ += damage;
				
				// 显示值锁定在5，但实际值继续累加
				meterTargetPos_ = std::min(5, meterActualPos_);
				meterStartPos_ = meterDisplayPos_;
				isMeterAnimating_ = true;
				meterAnimTime_ = 0.0f;
				
				std::cout << "[METER] afterHit actualPos=" << meterActualPos_ << " displayPos=" << meterTargetPos_ << std::endl;
		}
		else {
            if (!lockPlayerHealth_) {
                playerHealth_ -= damage;
                if (playerHealth_ < 0) playerHealth_ = 0;
            }
		}
		return;
		}
		// 守护者移动了，继续执行攻击流程，让攻击对守护者造成伤害
	}

	

    if (attacker.isPlayer == target.isPlayer) return; // 不能攻击友军

	// 水袭印记检查：如果目标潜水，攻击者无法攻击到，直接攻击本体
	if (target.isDiving) {
		// 目标潜水，攻击直接打到本体
		if (attacker.isPlayer) {
            {
                int before = enemyHealth_;
                enemyHealth_ -= damage;
            }
			if (enemyHealth_ < 0) enemyHealth_ = 0;
			
			// 更新墨尺：直接增加伤害值
			meterActualPos_ += damage;
			
			// 显示值锁定在5，但实际值继续累加
			meterTargetPos_ = std::min(5, meterActualPos_);
			meterStartPos_ = meterDisplayPos_;
			isMeterAnimating_ = true;
			meterAnimTime_ = 0.0f;
			
			std::cout << "[METER] afterHit actualPos=" << meterActualPos_ << " displayPos=" << meterTargetPos_ << std::endl;
		}
		else {
            if (!lockPlayerHealth_) {
                playerHealth_ -= damage;
                if (playerHealth_ < 0) playerHealth_ = 0;
            }
		}
		return;
	}

	// 空袭规则：空袭只能对高跳的敌人造成伤害，若对位不是高跳的敌人则直接对本体造成伤害
	if (hasAirStrike) {
		bool targetHasHighJump = hasMark(target.card, u8"高跳");
		if (!targetHasHighJump) {
		// 空袭攻击非高跳目标，直接攻击本体
		if (attacker.isPlayer) {
                {
                    int before = enemyHealth_;
                    enemyHealth_ -= damage;
                }
				if (enemyHealth_ < 0) enemyHealth_ = 0;
				
				// 更新墨尺：直接增加伤害值
				meterActualPos_ += damage;
				
				// 显示值锁定在5，但实际值继续累加
				meterTargetPos_ = std::min(5, meterActualPos_);
				meterStartPos_ = meterDisplayPos_;
				isMeterAnimating_ = true;
				meterAnimTime_ = 0.0f;
				
				std::cout << "[METER] afterHit actualPos=" << meterActualPos_ << " displayPos=" << meterTargetPos_ << std::endl;
			}
			else {
                if (!lockPlayerHealth_) {
                    playerHealth_ -= damage;
                    if (playerHealth_ < 0) playerHealth_ = 0;
                }
			}
			return;
		}
		// 空袭攻击高跳目标，正常攻击卡牌
	}

	// 坚硬之躯印记：免疫第一次攻击（包括死神之触），受到攻击时印记消失
	// 只有在真正攻击卡牌时才生效，空袭攻击本体、水袭潜水等情况不触发
	if (hasMark(target.card, std::string(u8"坚硬之躯"))) {
		// 移除坚硬之躯印记
		auto& targetCard = battlefield_[targetIndex].card;
		auto it = std::find(targetCard.marks.begin(), targetCard.marks.end(), std::string(u8"坚硬之躯"));
		if (it != targetCard.marks.end()) {
			targetCard.marks.erase(it);
		}
		
		statusMessage_ = std::string("坚硬之躯：") + target.card.name + " 免疫了这次攻击！印记消失！";
		return;
	}

	// 穿透规则：
	// - 穿透对所有卡牌生效（溢出伤害继续结算到后排或本体）
	// - 例外：若攻击者有"空袭"，则仅当对位目标有"高跳"时才允许穿透；否则不允许穿透
	bool targetHasHighJump = hasMark(target.card, u8"高跳");
	bool allowPenetration = true;
	if (hasAirStrike && !targetHasHighJump) {
		allowPenetration = false;
	}

	// 厌恶情绪印记：攻击者攻击带有该印记的目标时，攻击力变为0
	if (hasMark(battlefield_[targetIndex].card, std::string(u8"厌恶情绪"))) {
		damage = 0; // 攻击者压下攻击念头，攻击力变为0
	}

	// 执行对位伤害
	int remainingDamage = damage - target.health;
	battlefield_[targetIndex].health -= damage;

	// 死神之触：只要攻击到对面的卡牌（非本体），目标必定死亡
	// 磐石之身印记：免疫死神之触效果
	if (battlefield_[targetIndex].isAlive && hasMark(attacker.card, std::string(u8"死神之触"))) {
		if (!hasMark(battlefield_[targetIndex].card, std::string(u8"磐石之身"))) {
		battlefield_[targetIndex].health = 0;
		}
	}

	// 检查目标是否死亡
	if (battlefield_[targetIndex].health <= 0) {
		battlefield_[targetIndex].isAlive = false;
		cardsToDestroy_.push_back(targetIndex);
		
		// 只有在没有正在播放死亡动画时才设置新的死亡动画
		if (!isDestroyAnimating_) {
		isDestroyAnimating_ = true;
		destroyAnimTime_ = 0.0f;
		}

		// 嗜血狂热印记：攻击者每杀死一个造物，攻击力+1
		if (attackerIndex >= 0 && attackerIndex < TOTAL_BATTLEFIELD_SLOTS && battlefield_[attackerIndex].isAlive) {
			if (hasMark(battlefield_[attackerIndex].card, std::string(u8"嗜血狂热"))) {
				battlefield_[attackerIndex].card.attack += 1;
				statusMessage_ = std::string("嗜血狂热：") + battlefield_[attackerIndex].card.name + " 攻击力+1！";
			}
		}

		// 内心之蜂：即使被击杀也会生成手牌（仅我方单位）
		if (battlefield_[targetIndex].isPlayer && hasMark(battlefield_[targetIndex].card, std::string(u8"内心之蜂"))) {
			Card bee = CardDB::instance().make("yunchuang_fengshi");
			if (!bee.id.empty()) {
				EngraveStore::instance().applyToCard(bee);
				handCards_.push_back(bee);
				layoutHandCards();
				statusMessage_ = std::string("内心之蜂：获得手牌 ") + bee.name;
			}
		}

		// 铁兽夹印记：死亡时杀死对位卡牌，若对位因此死亡则获得狼皮
		if (hasMark(battlefield_[targetIndex].card, std::string(u8"铁兽夹"))) {
			// 计算对位索引
			int row = targetIndex / BATTLEFIELD_COLS;
			int col = targetIndex % BATTLEFIELD_COLS;
			int opposeIndex = -1;
			if (row == 2) opposeIndex = 1 * BATTLEFIELD_COLS + col; // 我方对位敌方
			else if (row == 1) opposeIndex = 2 * BATTLEFIELD_COLS + col; // 敌方对位我方
			else if (row == 0) opposeIndex = 1 * BATTLEFIELD_COLS + col; // 顶行对位第二行
			
			if (opposeIndex >= 0 && opposeIndex < TOTAL_BATTLEFIELD_SLOTS && battlefield_[opposeIndex].isAlive) {
				// 检查对位卡牌是否因此死亡
				bool opposeWillDie = battlefield_[opposeIndex].health <= 1;
				
				// 杀死对位卡牌
				battlefield_[opposeIndex].isAlive = false;
				battlefield_[opposeIndex].health = 0;
				cardsToDestroy_.push_back(opposeIndex);
				
				// 如果对位因此死亡，玩家获得狼皮
				if (opposeWillDie) {
					Card wolfSkin = CardDB::instance().make("langpi");
					if (!wolfSkin.id.empty()) {
						EngraveStore::instance().applyToCard(wolfSkin);
						handCards_.push_back(wolfSkin);
						layoutHandCards();
						statusMessage_ = std::string("铁兽夹：获得狼皮！");
					}
				}
			}
		}

		// 不死印记：回手逻辑改在摧毁动画结束时统一处理
	}
    else {
        // 目标存活：若目标带有反伤，以一次1点的攻击处理，允许连锁（有限深度）
        if (hasMark(battlefield_[targetIndex].card, std::string(u8"反伤"))) {
            if (attackerIndex >= 0 && attackerIndex < TOTAL_BATTLEFIELD_SLOTS && battlefield_[attackerIndex].isAlive) {
                if (thornsChainDepth_ < thornsChainMax_) {
                    thornsChainDepth_++;
                    attackTarget(targetIndex, attackerIndex, 1);
                    thornsChainDepth_--;
                }
            }
        }
        // 被击中后触发的被动（例如：内心之蜂）
        if (targetIndex >= 0 && targetIndex < TOTAL_BATTLEFIELD_SLOTS) {
            // 不论存活与否，均可触发
            if (hasMark(battlefield_[targetIndex].card, std::string(u8"内心之蜂"))) {
                Card bee = CardDB::instance().make("yunchuang_fengshi");
                if (!bee.id.empty()) {
                    EngraveStore::instance().applyToCard(bee);
                    handCards_.push_back(bee);
                    layoutHandCards();
                    statusMessage_ = std::string("内心之蜂：获得手牌 ") + bee.name;
                }
            }
        }
    }

	// 处理穿透伤害（仅当允许且有溢出）
	if (remainingDamage > 0 && allowPenetration) {
		if (attacker.isPlayer) {
			// 玩家攻击：尝试穿透到敌方后排同列（row - 1）。没有后排或为空则不再穿透至本体。
			int behindIndex = targetIndex - BATTLEFIELD_COLS;
			if (behindIndex >= 0 && behindIndex < TOTAL_BATTLEFIELD_SLOTS) {
				if (battlefield_[behindIndex].isAlive && !battlefield_[behindIndex].isPlayer) {
					// 对后排同列单位造成剩余伤害
					battlefield_[behindIndex].health -= remainingDamage;
					if (battlefield_[behindIndex].health <= 0) {
						battlefield_[behindIndex].isAlive = false;
						cardsToDestroy_.push_back(behindIndex);
						isDestroyAnimating_ = true;
						destroyAnimTime_ = 0.0f;
					}
				}
				// 如果后排为空或不是敌方单位，溢出伤害丢弃（不穿透到本体）
			}
			// 如果没有更后排，溢出伤害丢弃（不穿透到本体）
		}
		else {
			// 敌方攻击：玩家在最下行，无更后排。溢出伤害不再打玩家本体，直接丢弃。
		}
	}
}

//

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
			SDL_Rect innerRect = { scaledRect.x + 2, scaledRect.y + 2, scaledRect.w - 4, scaledRect.h - 4 };
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
		SDL_Color textColor{ 200, 200, 200, 255 }; // 水墨风格文字颜色
		for (int i = 0; i <= maxInk_; ++i) {
			int x = inkRulerRect_.x + (inkRulerRect_.w * i / maxInk_);
			std::string numText;
			if (i <= maxInk_ / 2) {
				numText = std::to_string(maxInk_ / 2 - i);
			}
			else {
				numText = std::to_string(i - maxInk_ / 2);
			}
			SDL_Surface* s = TTF_RenderUTF8_Blended(infoFont_, numText.c_str(), textColor);
			if (s) {
				SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
				if (t) {
					SDL_Rect dst{ x - s->w / 2, inkRulerRect_.y + 45, s->w, s->h };
					SDL_RenderCopy(r, t, nullptr, &dst);
					SDL_DestroyTexture(t);
				}
				SDL_FreeSurface(s);
			}
		}
	}


	// 绘制墨尺指针（在中间位置，水墨风格），根据meterPosition_偏移
	int stepPerTick = (inkRulerRect_.w / std::max(1, maxInk_));
	int pointerX = inkRulerRect_.x + inkRulerRect_.w / 2 + static_cast<int>(std::round(meterDisplayPos_ * stepPerTick));
	SDL_SetRenderDrawColor(r, 255, 255, 255, 255); // 水墨风格白色指针
	// 主指针线（更粗更长）
	SDL_RenderDrawLine(r, pointerX, inkRulerRect_.y + 50, pointerX, inkRulerRect_.y + 80);
	SDL_RenderDrawLine(r, pointerX - 1, inkRulerRect_.y + 50, pointerX - 1, inkRulerRect_.y + 80);
	SDL_RenderDrawLine(r, pointerX + 1, inkRulerRect_.y + 50, pointerX + 1, inkRulerRect_.y + 80);
	// 指针箭头（更大更明显）
	SDL_RenderDrawLine(r, pointerX - 5, inkRulerRect_.y + 60, pointerX + 5, inkRulerRect_.y + 60);
	SDL_RenderDrawLine(r, pointerX - 4, inkRulerRect_.y + 65, pointerX + 4, inkRulerRect_.y + 65);
	SDL_RenderDrawLine(r, pointerX - 3, inkRulerRect_.y + 70, pointerX + 3, inkRulerRect_.y + 70);

	// 绘制文脉圆形图案（墨尺上方）
	int wenmaiOverflow = std::max(0, meterActualPos_ - 5);
	if (wenmaiOverflow > 0) {
		SDL_SetRenderDrawColor(r, 255, 215, 0, 255); // 金色圆形
		int circleRadius = 8;
		int circleSpacing = 20;
		int startX = inkRulerRect_.x + 20;
		int circleY = inkRulerRect_.y - 30;
		
		for (int i = 0; i < wenmaiOverflow; ++i) {
			int circleX = startX + i * circleSpacing;
			// 绘制实心圆形
			for (int dy = -circleRadius; dy <= circleRadius; ++dy) {
				for (int dx = -circleRadius; dx <= circleRadius; ++dx) {
					if (dx * dx + dy * dy <= circleRadius * circleRadius) {
						SDL_RenderDrawPoint(r, circleX + dx, circleY + dy);
					}
				}
			}
			// 绘制圆形边框
			SDL_SetRenderDrawColor(r, 255, 255, 255, 255); // 白色边框
			for (int angle = 0; angle < 360; angle += 2) {
				float rad = angle * 3.14159f / 180.0f;
				int px = circleX + static_cast<int>(circleRadius * cosf(rad));
				int py = circleY + static_cast<int>(circleRadius * sinf(rad));
				SDL_RenderDrawPoint(r, px, py);
			}
			SDL_SetRenderDrawColor(r, 255, 215, 0, 255); // 恢复金色
		}
	}

	// 绘制砚台（左侧，横着放，在墨尺下面）
	SDL_SetRenderDrawColor(r, 35, 35, 35, 200);
	SDL_RenderFillRect(r, &inkStoneRect_);
	SDL_SetRenderDrawColor(r, 70, 70, 70, 255);
	SDL_RenderDrawRect(r, &inkStoneRect_);

    // 砚台悬停高亮
    if (hoveredBattlefieldIndex_ == -1 && hoveredItemIndex_ == -1) {
        int mx, my; SDL_GetMouseState(&mx, &my);
        if (mx >= inkStoneRect_.x && mx <= inkStoneRect_.x + inkStoneRect_.w && my >= inkStoneRect_.y && my <= inkStoneRect_.y + inkStoneRect_.h) {
            SDL_SetRenderDrawColor(r, 255, 240, 180, 25);
            SDL_RenderFillRect(r, &inkStoneRect_);
            SDL_SetRenderDrawColor(r, 255, 240, 180, 70);
            SDL_RenderDrawRect(r, &inkStoneRect_);
            SDL_Rect outer = inkStoneRect_; outer.x -= 3; outer.y -= 3; outer.w += 6; outer.h += 6;
            SDL_SetRenderDrawColor(r, 255, 220, 120, 35);
            SDL_RenderDrawRect(r, &outer);
        }
    }

	// 绘制砚台内部墨池（横着，更大）
	SDL_SetRenderDrawColor(r, 5, 5, 5, 200);
	SDL_Rect inkPool{ inkStoneRect_.x + 20, inkStoneRect_.y + 20, inkStoneRect_.w - 40, inkStoneRect_.h - 40 };
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
		SDL_Color textColor{ 220, 220, 220, 255 }; // 水墨风格文字颜色

		// 敌人区域标签
		SDL_Surface* s1 = TTF_RenderUTF8_Blended(infoFont_, "敌人", textColor);
		if (s1) {
			SDL_Texture* t1 = SDL_CreateTextureFromSurface(r, s1);
			if (t1) {
				SDL_Rect dst1{ enemyAreaRect_.x + 10, enemyAreaRect_.y + 10, s1->w, s1->h };
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
				SDL_Rect dst2{ inkRulerRect_.x + (inkRulerRect_.w - s2->w) / 2, inkRulerRect_.y + 10, s2->w, s2->h };
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
				SDL_Rect dst3{ inkStoneRect_.x + (inkStoneRect_.w - s3->w) / 2, inkStoneRect_.y + 10, s3->w, s3->h };
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
				SDL_Rect dst4{ cardInfoRect_.x + 10, cardInfoRect_.y + 10, s4->w, s4->h };
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
				SDL_Rect dst5{ itemAreaRect_.x + 10, itemAreaRect_.y + 10, s5->w, s5->h };
				SDL_RenderCopy(r, t5, nullptr, &dst5);
				SDL_DestroyTexture(t5);
			}
			SDL_FreeSurface(s5);
		}
	}

	// 绘制道具槽位和道具
	int itemSlotSize = 60;  // 道具槽尺寸
	int itemSpacing = 15;   // 道具间距
	int itemsPerRow = 3;    // 每行3个道具
	int itemStartX = itemAreaRect_.x + 20;
	int itemStartY = itemAreaRect_.y + 40;
	
	for (int i = 0; i < MAX_ITEMS; ++i) {
		// 计算道具槽位置（分两行显示）
		int row = i / itemsPerRow;
		int col = i % itemsPerRow;
		
		itemSlots_[i] = {
			itemStartX + col * (itemSlotSize + itemSpacing),
			itemStartY + row * (itemSlotSize + itemSpacing),
			itemSlotSize,
			itemSlotSize
		};
		
		// 绘制道具槽背景
		if (isItemHovered_[i]) {
			SDL_SetRenderDrawColor(r, 80, 80, 80, 255);
		} else {
			SDL_SetRenderDrawColor(r, 50, 50, 50, 200);
		}
		SDL_RenderFillRect(r, &itemSlots_[i]);
		SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
		SDL_RenderDrawRect(r, &itemSlots_[i]);
		
		// 如果有道具，绘制道具
		if (i < playerItems_.size()) {
			const Item& item = playerItems_[i];
			
			// 绘制道具图标（简单的圆形）
			SDL_SetRenderDrawColor(r, 200, 150, 50, 255);
			int centerX = itemSlots_[i].x + itemSlots_[i].w / 2;
			int centerY = itemSlots_[i].y + itemSlots_[i].h / 2;
			int radius = itemSlotSize / 3;
			
			// 绘制圆形（简单版本）
			for (int w = 0; w < radius * 2; w++) {
				for (int h = 0; h < radius * 2; h++) {
					int dx = radius - w;
					int dy = radius - h;
					if ((dx * dx + dy * dy) <= (radius * radius)) {
						SDL_RenderDrawPoint(r, centerX + dx, centerY + dy);
					}
				}
			}
			
			// 绘制道具数量（如果大于1）
			if (item.count > 1 && infoFont_) {
				SDL_Color countColor{ 255, 255, 255, 255 };
				std::string countText = std::to_string(item.count);
				SDL_Surface* countSurface = TTF_RenderUTF8_Blended(infoFont_, countText.c_str(), countColor);
				if (countSurface) {
					SDL_Texture* countTexture = SDL_CreateTextureFromSurface(r, countSurface);
					if (countTexture) {
						SDL_Rect countRect{ itemSlots_[i].x + itemSlots_[i].w - 15, itemSlots_[i].y + 2, countSurface->w, countSurface->h };
						SDL_RenderCopy(r, countTexture, nullptr, &countRect);
						SDL_DestroyTexture(countTexture);
					}
					SDL_FreeSurface(countSurface);
				}
			}
			
			// 绘制道具名称（在道具下方）
			if (infoFont_) {
				SDL_Color nameColor{ 180, 180, 180, 255 };
				SDL_Surface* nameSurface = TTF_RenderUTF8_Blended(infoFont_, item.name.c_str(), nameColor);
				if (nameSurface) {
					SDL_Texture* nameTexture = SDL_CreateTextureFromSurface(r, nameSurface);
					if (nameTexture) {
						SDL_Rect nameRect{ 
							itemSlots_[i].x + (itemSlots_[i].w - nameSurface->w) / 2, 
							itemSlots_[i].y + itemSlots_[i].h + 5, 
							nameSurface->w, 
							nameSurface->h 
						};
						SDL_RenderCopy(r, nameTexture, nullptr, &nameRect);
						SDL_DestroyTexture(nameTexture);
					}
					SDL_FreeSurface(nameSurface);
				}
			}
		}
	}

	// 绘制墨锭牌堆（右下角）
	SDL_SetRenderDrawColor(r, 35, 35, 35, 200);
	SDL_RenderFillRect(r, &inkPileRect_);
	SDL_SetRenderDrawColor(r, 70, 70, 70, 255);
	SDL_RenderDrawRect(r, &inkPileRect_);

    // 敌方牌堆悬停高亮
    if (hoveredBattlefieldIndex_ == -1 && hoveredItemIndex_ == -1) {
        int mx, my; SDL_GetMouseState(&mx, &my);
        if (mx >= inkPileRect_.x && mx <= inkPileRect_.x + inkPileRect_.w && my >= inkPileRect_.y && my <= inkPileRect_.y + inkPileRect_.h) {
            SDL_SetRenderDrawColor(r, 255, 240, 180, 25);
            SDL_RenderFillRect(r, &inkPileRect_);
            SDL_SetRenderDrawColor(r, 255, 240, 180, 70);
            SDL_RenderDrawRect(r, &inkPileRect_);
            SDL_Rect outer = inkPileRect_; outer.x -= 3; outer.y -= 3; outer.w += 6; outer.h += 6;
            SDL_SetRenderDrawColor(r, 255, 220, 120, 35);
            SDL_RenderDrawRect(r, &outer);
        }
    }

	// 绘制玩家牌堆（右下角）
	SDL_SetRenderDrawColor(r, 40, 40, 40, 200);
	SDL_RenderFillRect(r, &playerPileRect_);
	SDL_SetRenderDrawColor(r, 75, 75, 75, 255);
	SDL_RenderDrawRect(r, &playerPileRect_);

    // 玩家牌堆悬停高亮
    if (hoveredBattlefieldIndex_ == -1 && hoveredItemIndex_ == -1) {
        int mx, my; SDL_GetMouseState(&mx, &my);
        if (mx >= playerPileRect_.x && mx <= playerPileRect_.x + playerPileRect_.w && my >= playerPileRect_.y && my <= playerPileRect_.y + playerPileRect_.h) {
            SDL_SetRenderDrawColor(r, 255, 240, 180, 25);
            SDL_RenderFillRect(r, &playerPileRect_);
            SDL_SetRenderDrawColor(r, 255, 240, 180, 70);
            SDL_RenderDrawRect(r, &playerPileRect_);
            SDL_Rect outer = playerPileRect_; outer.x -= 3; outer.y -= 3; outer.w += 6; outer.h += 6;
            SDL_SetRenderDrawColor(r, 255, 220, 120, 35);
            SDL_RenderDrawRect(r, &outer);
        }
    }

	// 绘制牌堆文字
	if (infoFont_) {
		SDL_Color textColor{ 220, 220, 220, 255 }; // 水墨风格文字颜色

		// 墨锭牌堆文字
		SDL_Surface* s1 = TTF_RenderUTF8_Blended(infoFont_, "墨锭牌堆", textColor);
		if (s1) {
			SDL_Texture* t1 = SDL_CreateTextureFromSurface(r, s1);
			if (t1) {
				SDL_Rect dst1{ inkPileRect_.x + 5, inkPileRect_.y + 5, s1->w, s1->h };
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
				SDL_Rect dst2{ playerPileRect_.x + 5, playerPileRect_.y + 5, s2->w, s2->h };
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
				SDL_Rect dst3{ inkPileRect_.x + 5, inkPileRect_.y + 25, s3->w, s3->h };
				SDL_RenderCopy(r, t3, nullptr, &dst3);
				SDL_DestroyTexture(t3);
			}
			SDL_FreeSurface(s3);
		}

		std::string playerCountText = "x" + std::to_string(battleDeck_.size());
		SDL_Surface* s4 = TTF_RenderUTF8_Blended(infoFont_, playerCountText.c_str(), textColor);
		if (s4) {
			SDL_Texture* t4 = SDL_CreateTextureFromSurface(r, s4);
			if (t4) {
				SDL_Rect dst4{ playerPileRect_.x + 5, playerPileRect_.y + 25, s4->w, s4->h };
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
		SDL_Rect dropBg{ progressX - 10, progressY - 10, dropSize + 20, currentSacrificeInk_ * dropSpacing + 20 };
		SDL_RenderFillRect(r, &dropBg);
		SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
		SDL_RenderDrawRect(r, &dropBg);

		// 绘制墨滴（献祭多少就显示多少个，竖着排列）
		for (int i = 0; i < currentSacrificeInk_; ++i) {
			int x = progressX;
			int y = progressY + i * dropSpacing;

			// 绘制墨滴（黑色圆形）
			SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
			SDL_Rect drop{ x, y, dropSize, dropSize };
			SDL_RenderFillRect(r, &drop);
			SDL_SetRenderDrawColor(r, 50, 50, 50, 255);
			SDL_RenderDrawRect(r, &drop);

			// 绘制墨滴尾巴（向下）
			SDL_RenderDrawLine(r, x + dropSize / 2, y + dropSize, x + dropSize / 2, y + dropSize + dropSize / 2);
		}

		// 显示进度文字（在墨滴下方）
		if (infoFont_) {
			SDL_Color progressCol{ 220, 220, 220, 255 };
			std::string progressText = std::to_string(currentSacrificeInk_) + "/" + std::to_string(sacrificeTargetCost_);
			SDL_Surface* s = TTF_RenderUTF8_Blended(infoFont_, progressText.c_str(), progressCol);
			if (s) {
				SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
				SDL_Rect dst{ progressX - s->w / 2, progressY + currentSacrificeInk_ * dropSpacing + 10, s->w, s->h };
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
		}
		else {
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
			SDL_Rect boneBody{ x + 2, y + 4, boneSize - 4, boneSize - 8 };
			SDL_RenderFillRect(r, &boneBody);

			// 骨头两端（圆形）
			SDL_Rect boneEnd1{ x, y + 2, 6, 6 };
			SDL_Rect boneEnd2{ x + boneSize - 6, y + 2, 6, 6 };
			SDL_Rect boneEnd3{ x, y + boneSize - 8, 6, 6 };
			SDL_Rect boneEnd4{ x + boneSize - 6, y + boneSize - 8, 6, 6 };

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


	// 左上角玩家/敌人血量UI已移除

	// 显示伤害数值（在敌人区域中央）
	if (showDamage_ && totalDamageDealt_ > 0 && enemyFont_) {
		SDL_Color damageColor{ 255, 255, 0, 255 }; // 黄色
		std::string damageText = "-" + std::to_string(totalDamageDealt_);
		SDL_Surface* damageSurface = TTF_RenderUTF8_Blended(enemyFont_, damageText.c_str(), damageColor);
		if (damageSurface) {
			SDL_Texture* damageTexture = SDL_CreateTextureFromSurface(r, damageSurface);
			if (damageTexture) {
				// 在敌人区域中央显示伤害
				SDL_Rect damageRect{ enemyAreaRect_.x + enemyAreaRect_.w / 2 - damageSurface->w / 2,
									enemyAreaRect_.y + enemyAreaRect_.h / 2 - damageSurface->h / 2,
									damageSurface->w, damageSurface->h };

				// 绘制半透明背景
				SDL_SetRenderDrawColor(r, 0, 0, 0, 150);
				SDL_Rect damageBg{ damageRect.x - 10, damageRect.y - 5, damageRect.w + 20, damageRect.h + 10 };
				SDL_RenderFillRect(r, &damageBg);

				// 绘制伤害数值
				SDL_RenderCopy(r, damageTexture, nullptr, &damageRect);
				SDL_DestroyTexture(damageTexture);
			}
			SDL_FreeSurface(damageSurface);
		}
	}

	// 上帝模式帮助：绘制到手牌区左侧（使用更小字体）
	if (godMode_) {
		TTF_Font* helpFont = cardStatFont_ ? cardStatFont_ : (infoFont_ ? infoFont_ : enemyFont_);
		if (helpFont) {
		SDL_Color godModeColor{ 255, 255, 0, 255 }; // 黄色
			const char* lines[] = {
				u8"上帝模式：",
				u8"H：锁血/锁墨尺",
				u8"J：魂骨+1",
				u8"A：在敌方悬停格生成 巴蛇",
				u8"S：在敌方悬停格生成 厌恶情绪",
				u8"D：在敌方悬停格生成 铁兽夹",
				u8"F：在敌方悬停格生成 全向打击",
				u8"B：获得随机道具（道具未满）",
				u8"V：检索玩家牌堆",
				u8"N：检索全卡库",
				u8"G：在第一/二行空格生成(全卡库选择)"
			};
			int lineCount = sizeof(lines) / sizeof(lines[0]);
			int x = handAreaRect_.x + 10;
			int y = handAreaRect_.y + 10;
			for (int i = 0; i < lineCount; ++i) {
				SDL_Surface* s = TTF_RenderUTF8_Blended(helpFont, lines[i], godModeColor);
				if (!s) continue;
				SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
				if (t) {
					SDL_Rect dst{ x, y, s->w, s->h };
					SDL_RenderCopy(r, t, nullptr, &dst);
					SDL_DestroyTexture(t);
				}
				int h = s->h; SDL_FreeSurface(s);
				y += 4 + h; // 行距
			}
		}
	}

	// 显示状态消息（在敌人区域）
	if (!statusMessage_.empty() && enemyFont_) {
		SDL_Color statusColor{ 255, 255, 255, 255 }; // 白色文字
		SDL_Surface* statusSurface = TTF_RenderUTF8_Blended(enemyFont_, statusMessage_.c_str(), statusColor);
		if (statusSurface) {
			SDL_Texture* statusTexture = SDL_CreateTextureFromSurface(r, statusSurface);
			if (statusTexture) {
				// 在敌人区域下方显示状态消息
				SDL_Rect statusRect{ enemyAreaRect_.x + enemyAreaRect_.w / 2 - statusSurface->w / 2,
								   enemyAreaRect_.y + enemyAreaRect_.h - 100,
								   statusSurface->w, statusSurface->h };

				// 绘制半透明背景
				SDL_SetRenderDrawColor(r, 0, 0, 0, 150);
				SDL_Rect statusBg{ statusRect.x - 10, statusRect.y - 5, statusRect.w + 20, statusRect.h + 10 };
				SDL_RenderFillRect(r, &statusBg);

				// 绘制状态消息
				SDL_RenderCopy(r, statusTexture, nullptr, &statusRect);
				SDL_DestroyTexture(statusTexture);
			}
			SDL_FreeSurface(statusSurface);
		}
	}

	// 道具悬停提示：在道具栏下方显示名称与描述（18字换行，限制不超出屏幕）
	if (hoveredItemIndex_ >= 0 && hoveredItemIndex_ < static_cast<int>(playerItems_.size()) && enemyFont_) {
		const Item& it = playerItems_[hoveredItemIndex_];
		std::string tip = it.name + "：" + it.description;
		// 将UTF-8字符串每18个"字符"分行（按UTF-8首字节计数）
		auto wrapUtf8 = [](const std::string& s, int maxChars) {
			std::vector<std::string> lines; lines.reserve((int)s.size() / (maxChars*2) + 1);
			int count = 0; size_t start = 0; size_t i = 0; 
			while (i < s.size()) {
				unsigned char c = (unsigned char)s[i];
				bool isLead = (c & 0xC0) != 0x80; // 非续字节视为一个字符（近似）
				if (isLead) {
					if (count == maxChars) {
						lines.push_back(s.substr(start, i - start));
						start = i; count = 0;
					}
					count++;
				}
				i++;
			}
			if (start < s.size()) lines.push_back(s.substr(start));
			return lines;
		};

		std::vector<std::string> lines = wrapUtf8(tip, 16);
		TTF_Font* tipFont = cardNameFont_ ? cardNameFont_ : enemyFont_;
		SDL_Color tipColor{ 255, 255, 0, 255 };

		// 计算最大宽与总高
		int maxW = 0; int totalH = 0; int lineH = 0; std::vector<SDL_Surface*> surfaces; surfaces.reserve(lines.size());
		for (const auto& ln : lines) {
			SDL_Surface* s = TTF_RenderUTF8_Blended(tipFont, ln.c_str(), tipColor);
			surfaces.push_back(s);
			if (s) { if (s->w > maxW) maxW = s->w; totalH += s->h; lineH = s->h; }
		}
		int lineSpacing = 4; totalH += (int(lines.size()) - 1) * lineSpacing;

		// 放置位置：以第一个道具槽为基准，限制在屏幕内
		int baseX = itemSlots_[0].x;
		int baseY = itemSlots_[0].y + itemSlots_[0].h + 28;
		if (baseX + maxW + 16 > screenW_) baseX = std::max(10, screenW_ - (maxW + 16));
		if (baseY + totalH + 8 > screenH_) baseY = std::max(10, screenH_ - (totalH + 8));

		// 背景
		SDL_SetRenderDrawColor(r, 0, 0, 0, 160);
		SDL_Rect bg{ baseX - 8, baseY - 4, maxW + 16, totalH + 8 };
		SDL_RenderFillRect(r, &bg);

		// 绘制各行
		int y = baseY;
		for (SDL_Surface* s : surfaces) {
			if (!s) continue;
			SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
			if (t) {
				SDL_Rect dst{ baseX, y, s->w, s->h };
				SDL_RenderCopy(r, t, nullptr, &dst);
				SDL_DestroyTexture(t);
			}
			y += s->h + lineSpacing;
		}
		for (SDL_Surface* s : surfaces) if (s) SDL_FreeSurface(s);
	}
}
// 冲刺能手相关方法实现
void BattleState::startRushing(int cardIndex) {
	if (cardIndex < 0 || cardIndex >= TOTAL_BATTLEFIELD_SLOTS) {
		return;
	}

	const auto& card = battlefield_[cardIndex];
	if (!card.isAlive) {
		return;
	}

	// 计算当前位置
	int currentRow = cardIndex / BATTLEFIELD_COLS;
	int currentCol = cardIndex % BATTLEFIELD_COLS;

	// 检查是否在边界，如果在边界，设置初始方向为另一方向
	if (!hasSetInitialRushDirection_) {
		if (currentCol == 0) {
			rushingDirection_ = 1; // 在左边界，初始方向向右
		}
		else if (currentCol == BATTLEFIELD_COLS - 1) {
			rushingDirection_ = -1; // 在右边界，初始方向向左
		}
		hasSetInitialRushDirection_ = true; // 标记已设置初始方向
	}

	// 更新卡牌的方向
	battlefield_[cardIndex].moveDirection = rushingDirection_;

	// 检测当前方向是否可以移动
	bool canMove = checkRushingCanMove(currentRow, currentCol, rushingDirection_);

	if (canMove) {
		// 可以移动，设置状态为横冲直撞
		isRushing_ = true;
		rushingCardIndex_ = cardIndex;
		isRushingShaking_ = false;
		rushingAnimTime_ = 0.0f;
		statusMessage_ = "冲刺能手开始！";
	}
	else {
		// 原方向不可行：自动切换方向并再次检测
		int flipped = -rushingDirection_;
		bool canMoveFlipped = checkRushingCanMove(currentRow, currentCol, flipped);
		if (canMoveFlipped) {
			// 直接采用相反方向移动
			rushingDirection_ = flipped;
			battlefield_[cardIndex].moveDirection = rushingDirection_;
			isRushing_ = true;
			rushingCardIndex_ = cardIndex;
			isRushingShaking_ = false;
			rushingAnimTime_ = 0.0f;
			statusMessage_ = "冲刺能手自动转向并开始！";
		}
		else {
			// 相反方向也不可行：播放摇晃动画
			rushingDirection_ = flipped; // 预先转向，供下次尝试
			battlefield_[cardIndex].moveDirection = rushingDirection_;
			isRushing_ = true;
			rushingCardIndex_ = cardIndex;
			isRushingShaking_ = true;
			rushingAnimTime_ = 0.0f;
			statusMessage_ = "冲刺能手被阻挡，自动转向并播放摇晃动画！";
		}
	}
}

// 添加 checkRushingCanMove 函数实现
bool BattleState::checkRushingCanMove(int currentRow, int currentCol, int direction) {
	int targetCol = currentCol + direction;
	if (targetCol >= 0 && targetCol < BATTLEFIELD_COLS) {
		int targetIndex = currentRow * BATTLEFIELD_COLS + targetCol;
		return !battlefield_[targetIndex].isAlive;
	}
	return false;
}

void BattleState::updateRushing(float dt) {
	if (!isRushing_) return;

	rushingAnimTime_ += dt;

	// 动画持续时间
	if (rushingAnimTime_ >= rushingAnimDuration_) {
		// 动画结束，执行移动
		executeRushing();
	}
}

void BattleState::executeRushing() {
	if (rushingCardIndex_ < 0 || rushingCardIndex_ >= TOTAL_BATTLEFIELD_SLOTS) {
		// 重置状态
		isRushing_ = false;
		isRushingShaking_ = false;
		rushingCardIndex_ = -1;
		rushingAnimTime_ = 0.0f;
		return;
	}

	const auto& card = battlefield_[rushingCardIndex_];
	if (!card.isAlive ) {
		// 卡牌已死亡，重置状态
		isRushing_ = false;
		isRushingShaking_ = false;
		rushingCardIndex_ = -1;
		rushingAnimTime_ = 0.0f;
		return;
	}
	int currentRow = rushingCardIndex_ / BATTLEFIELD_COLS;
	int currentCol = rushingCardIndex_ % BATTLEFIELD_COLS;
	int targetCol = currentCol + rushingDirection_;
	// 检查是否是摇晃动画
	if (isRushingShaking_) {
		// 摇晃动画，不执行移动
		statusMessage_ = "冲刺能手摇晃完成，下回合将向新方向移动！";
	}
	else {
		// 正常移动动画，执行移动
		// 计算当前位置

		// 移动到目标位置
		
		if (targetCol >= 0 && targetCol < BATTLEFIELD_COLS) {
			int targetIndex = currentRow * BATTLEFIELD_COLS + targetCol;

			if (!battlefield_[targetIndex].isAlive) {
				// 找到空位，移动到这里
				SDL_Rect targetRect = battlefield_[targetIndex].rect;

				// 移动卡牌数据
				battlefield_[targetIndex] = battlefield_[rushingCardIndex_];
				// 保持目标位置的rect
				battlefield_[targetIndex].rect = targetRect;

				// 清空原位置，标记为因移动死亡（不获得魂骨）
				battlefield_[rushingCardIndex_].isAlive = false;
				battlefield_[rushingCardIndex_].health = 0;
				battlefield_[rushingCardIndex_].isMovedToDeath = true;
				suppressBoneGainThisFrame_ = true;
			}
		}
	}
	// 检查移动后是否在边界
	int newCol = currentCol;
	if (targetCol >= 0 && targetCol < BATTLEFIELD_COLS) {
		newCol = targetCol; // 使用移动后的目标列
	}
	if (newCol == 0 || newCol == BATTLEFIELD_COLS - 1) {
		if (currentCol != targetCol) {
			// 在边界，改变方向
			rushingDirection_ = -rushingDirection_;
			statusMessage_ = "冲刺能手到达边界，改变方向！";
		}
	}
	else {
		statusMessage_ = "冲刺能手移动成功！";
	}

	// 重置状态
	isRushing_ = false;
	isRushingShaking_ = false;
	rushingCardIndex_ = -1;
	rushingAnimTime_ = 0.0f;

	// 冲刺能手移动完成，检查是否还有更多移动卡牌
	onMovementComplete();
}

// 推动检测辅助方法实现
BattleState::PushResult BattleState::checkCanPush(int currentRow, int currentCol, int direction) {
	PushResult result;
	result.canPush = false;
	result.cardsToPush.clear();

	// 沿路径检测所有格子，直到找到空位或到达边界
	int checkCol = currentCol + direction;

	while (checkCol >= 0 && checkCol < BATTLEFIELD_COLS) {
		int checkIndex = currentRow * BATTLEFIELD_COLS + checkCol;

		if (!battlefield_[checkIndex].isAlive) {
			// 检测到空位，可以移动
			result.canPush = true;
			result.cardsToPush.push_back(checkIndex);
			break;
		}

		// 继续检查下一个格子
		checkCol += direction;
	}

	return result;
}

// 蛮力冲撞相关方法实现
void BattleState::startBruteForce(int cardIndex) {
	if (cardIndex < 0 || cardIndex >= TOTAL_BATTLEFIELD_SLOTS) {
		return;
	}

	const auto& card = battlefield_[cardIndex];
	if (!card.isAlive) {
		return;
	}

	// 计算当前位置
	int currentRow = cardIndex / BATTLEFIELD_COLS;
	int currentCol = cardIndex % BATTLEFIELD_COLS;

	// 检查是否在边界，如果在边界，设置初始方向为另一方向
	if (!hasSetInitialBruteForceDirection_) {
		if (currentCol == 0) {
			bruteForceDirection_ = 1; // 在左边界，初始方向向右
		}
		else if (currentCol == BATTLEFIELD_COLS - 1) {
			bruteForceDirection_ = -1; // 在右边界，初始方向向左
		}
		hasSetInitialBruteForceDirection_ = true; // 标记已设置初始方向
	}

	// 更新卡牌的方向
	battlefield_[cardIndex].moveDirection = bruteForceDirection_;

	// 检测当前方向是否可以推动
	PushResult pushResult = checkCanPush(currentRow, currentCol, bruteForceDirection_);

	if (pushResult.canPush) {
		// 可以推动，设置状态为蛮力冲撞
		isBruteForcing_ = true;
		bruteForceCardIndex_ = cardIndex;
		isBruteForceShaking_ = false;
		bruteForceAnimTime_ = 0.0f;
		statusMessage_ = "蛮力冲撞开始！";
	}
	else {
		// 原方向不可行：自动切换方向并再次检测
		int flipped = -bruteForceDirection_;
		PushResult flippedResult = checkCanPush(currentRow, currentCol, flipped);
		if (flippedResult.canPush) {
			// 直接采用相反方向推动
			bruteForceDirection_ = flipped;
			battlefield_[cardIndex].moveDirection = bruteForceDirection_;
			isBruteForcing_ = true;
			bruteForceCardIndex_ = cardIndex;
			isBruteForceShaking_ = false;
			bruteForceAnimTime_ = 0.0f;
			statusMessage_ = "蛮力冲撞自动转向并开始！";
		}
		else {
			// 相反方向也不可行：播放摇晃动画
			bruteForceDirection_ = flipped; // 预先转向，供下次尝试
			battlefield_[cardIndex].moveDirection = bruteForceDirection_;
			isBruteForcing_ = true;
			bruteForceCardIndex_ = cardIndex;
			isBruteForceShaking_ = true;
			bruteForceAnimTime_ = 0.0f;
			statusMessage_ = "蛮力冲撞被阻挡，自动转向并播放摇晃动画！";
		}
	}
}

void BattleState::updateBruteForce(float dt) {
	if (!isBruteForcing_) return;

	bruteForceAnimTime_ += dt;

	if (bruteForceAnimTime_ >= bruteForceAnimDuration_) {
		// 蛮力冲撞动画完成，执行移动
		executeBruteForce();
		// 立即触发渲染更新
		// 注意：这里假设有一个方法可以强制渲染更新，如果没有，需要在游戏循环中处理
		statusMessage_ = "蛮力冲撞移动已完成，位置已更新！";
	}
}

void BattleState::executeBruteForce() {
	if (bruteForceCardIndex_ < 0 || bruteForceCardIndex_ >= TOTAL_BATTLEFIELD_SLOTS) {
		// 重置状态
		isBruteForcing_ = false;
		bruteForceCardIndex_ = -1;
		bruteForceAnimTime_ = 0.0f;
		return;
	}

	const auto& card = battlefield_[bruteForceCardIndex_];
	if (!card.isAlive) {
		// 卡牌已死亡，重置状态
		isBruteForcing_ = false;
		bruteForceCardIndex_ = -1;
		bruteForceAnimTime_ = 0.0f;
		return;
	}

	// 计算当前位置
	int currentRow = bruteForceCardIndex_ / BATTLEFIELD_COLS;
	int currentCol = bruteForceCardIndex_ % BATTLEFIELD_COLS;
	PushResult pushResult = checkCanPush(currentRow, currentCol, bruteForceDirection_);
	// 检查是否是摇晃动画
	if (isBruteForceShaking_) {
		// 摇晃动画，不执行移动
		statusMessage_ = "蛮力冲撞摇晃完成，下回合将向新方向移动！";
	}
	else {
		// 正常推动动画，执行移动
		// 重新检测推动路径

		if (pushResult.canPush) {
			// 执行推动：先移动路径上记录的可推动卡牌，再移动蛮力冲撞自己
			
			// 收集需要移动的卡牌索引
			std::vector<int> cardsToMove;
			int checkCol = currentCol + bruteForceDirection_;
			while (checkCol >= 0 && checkCol < BATTLEFIELD_COLS) {
				int checkIndex = currentRow * BATTLEFIELD_COLS + checkCol;
				if (battlefield_[checkIndex].isAlive) {
					cardsToMove.push_back(checkIndex);
				} else {
					break; // 遇到空位，停止
				}
				checkCol += bruteForceDirection_;
			}

			// 从最远端的卡牌开始移动，确保连锁反应
			for (size_t i = cardsToMove.size(); i > 0; --i) {
				int currentIndex = cardsToMove[i - 1];
				int targetCol = (currentIndex % BATTLEFIELD_COLS) + bruteForceDirection_;
				if (targetCol >= 0 && targetCol < BATTLEFIELD_COLS) {
					int targetIndex = currentRow * BATTLEFIELD_COLS + targetCol;
					if (!battlefield_[targetIndex].isAlive) {
						// 目标位置为空，移动卡牌
						SDL_Rect targetRect = battlefield_[targetIndex].rect;
						battlefield_[targetIndex] = battlefield_[currentIndex];
						battlefield_[targetIndex].rect = targetRect;

						// 清空原位置，标记为因移动死亡（不获得魂骨）
						battlefield_[currentIndex].isAlive = false;
						battlefield_[currentIndex].health = 0;
						battlefield_[currentIndex].isMovedToDeath = true;
						suppressBoneGainThisFrame_ = true;
					}
				}
			}

			// 移动蛮力冲撞卡牌到相邻位置
			int targetCol = currentCol + bruteForceDirection_;
			if (targetCol >= 0 && targetCol < BATTLEFIELD_COLS) {
				int targetIndex = currentRow * BATTLEFIELD_COLS + targetCol;
				if (!battlefield_[targetIndex].isAlive) {
					// 目标位置是空的，直接移动蛮力冲撞卡牌
					SDL_Rect targetRect = battlefield_[targetIndex].rect;
					battlefield_[targetIndex] = battlefield_[bruteForceCardIndex_];
					battlefield_[targetIndex].rect = targetRect;

					// 清空原位置，标记为因移动死亡（不获得魂骨）
					battlefield_[bruteForceCardIndex_].isAlive = false;
					battlefield_[bruteForceCardIndex_].health = 0;
					battlefield_[bruteForceCardIndex_].isMovedToDeath = true;
					suppressBoneGainThisFrame_ = true;

					// 更新bruteForceCardIndex_为新位置
					bruteForceCardIndex_ = targetIndex;
				}
			}
		}
	}

	// 检查移动后是否在边界
	int newCol = currentCol + (pushResult.canPush ? bruteForceDirection_ : 0);
	if ((newCol == 0 || newCol == BATTLEFIELD_COLS - 1) && pushResult.canPush) {
		// 在边界，改变方向
		bruteForceDirection_ = -bruteForceDirection_;
		battlefield_[bruteForceCardIndex_].moveDirection = bruteForceDirection_;
		statusMessage_ = "蛮力冲撞到达边界，改变方向！";
	}
	else {
		statusMessage_ = "蛮力冲撞推动成功！";
	}

	// 检查是否是摇晃动画
	if (isBruteForceShaking_) {
		// 摇晃动画完成，不移动，直接结束
		isBruteForceShaking_ = false;
		statusMessage_ = "蛮力冲撞摇晃完成，下回合将向新方向移动！";
	}
	else {
		// 正常推动动画完成
		statusMessage_ = "蛮力冲撞推动成功！";
	}

	// 重置状态
	isBruteForcing_ = false;
	bruteForceCardIndex_ = -1;
	bruteForceAnimTime_ = 0.0f;

	// 蛮力冲撞移动完成，检查是否还有更多移动卡牌
	// 确保动画完成后才调用 onMovementComplete()
	onMovementComplete();
}


// 被推动卡牌动画方法实现
void BattleState::startPushedAnimation(const std::vector<int>& cardIndices, const std::vector<int>& directions) {
	if (cardIndices.empty()) return;

	// 初始化被推动动画状态
	isPushedAnimating_ = true;
	pushedCardIndices_ = cardIndices;
	pushedDirections_ = directions;
	pushedAnimTime_ = 0.0f;

	statusMessage_ = "卡牌被推动！";
}

void BattleState::updatePushedAnimation(float dt) {
	if (!isPushedAnimating_) return;

	pushedAnimTime_ += dt;

	// 被推动动画完成
	if (pushedAnimTime_ >= pushedAnimDuration_) {
		// 被推动动画完成，执行移动
		executePushedAnimation();
	}
}

void BattleState::executePushedAnimation() {
	if (pushedCardIndices_.empty()) {
		// 重置状态
		isPushedAnimating_ = false;
		pushedCardIndices_.clear();
		pushedDirections_.clear();
		pushedAnimTime_ = 0.0f;
		return;
	}

	// 执行所有被推动卡牌的移动
	for (size_t i = 0; i < pushedCardIndices_.size(); ++i) {
		int cardIndex = pushedCardIndices_[i];
		int direction = pushedDirections_[i];

		if (cardIndex < 0 || cardIndex >= TOTAL_BATTLEFIELD_SLOTS) continue;

		const auto& card = battlefield_[cardIndex];
		if (!card.isAlive) continue;

		// 重新计算当前位置和目标位置（因为卡牌可能已经移动过）
		int currentRow = cardIndex / BATTLEFIELD_COLS;
		int currentCol = cardIndex % BATTLEFIELD_COLS;
		int targetCol = currentCol + direction;

		if (targetCol >= 0 && targetCol < BATTLEFIELD_COLS) {
			int targetIndex = currentRow * BATTLEFIELD_COLS + targetCol;

			// 检查目标位置是否为空
			if (!battlefield_[targetIndex].isAlive) {
				// 保存目标位置的rect
				SDL_Rect targetRect = battlefield_[targetIndex].rect;

				// 移动卡牌数据
				battlefield_[targetIndex] = battlefield_[cardIndex];
				// 保持目标位置的rect
				battlefield_[targetIndex].rect = targetRect;

				// 清空原位置，标记为因移动死亡（不获得魂骨）
				battlefield_[cardIndex].isAlive = false;
				battlefield_[cardIndex].health = 0;
				battlefield_[cardIndex].isMovedToDeath = true;
				suppressBoneGainThisFrame_ = true;
			}
		}
	}

	// 重置状态
	isPushedAnimating_ = false;
	pushedCardIndices_.clear();
	pushedDirections_.clear();
	pushedAnimTime_ = 0.0f;

	statusMessage_ = "推动完成！";

	// 被推动动画完成，检查是否还有更多移动卡牌
	onMovementComplete();
}

// 移动卡牌队列处理方法实现
void BattleState::processNextMovement() {
	// 按照位置顺序处理，不区分类型
	if (!pendingMovementCards_.empty()) {
		int cardIndex = pendingMovementCards_[0];
		pendingMovementCards_.erase(pendingMovementCards_.begin());

		// 根据卡牌类型启动相应的移动
		if (hasMark(battlefield_[cardIndex].card, u8"冲刺能手")) {
			startRushing(cardIndex);
		}
		else if (hasMark(battlefield_[cardIndex].card, u8"蛮力冲撞")) {
			startBruteForce(cardIndex);
		}

	// 已移除：误插入到移动流程中的UI高亮代码（统一在 renderUI 中绘制）

	}
	else {
		// 没有更多移动卡牌，结束移动队列处理
		isProcessingMovementQueue_ = false;
		onMovementComplete();
	}
}

void BattleState::onMovementComplete() {
	// 检查是否还有更多移动卡牌需要处理
	if (!pendingMovementCards_.empty()) {
		// 还有更多移动卡牌，处理下一个
		processNextMovement();
	}
	else {
		// 没有更多移动卡牌，结束移动队列处理
		isProcessingMovementQueue_ = false;
		// 如果正在播放成长动画，暂不进行相位切换，等待成长完成后由成长流程继续推进
		if (isGrowthAnimating_) {
			return;
		}
        if (currentPhase_ == GamePhase::EnemyTurn) {
            // 敌方回合结束时：结算掘墓人（统计敌方单位）
            grantGravediggerBones(true);
            
            // 清理风雅扇的空袭效果（一回合后失效）
            for (int slot : fengyaShanAirstrikeSlots_) {
                if (slot >= 0 && slot < TOTAL_BATTLEFIELD_SLOTS) {
                    auto& bf = battlefield_[slot];
                    if (bf.isAlive && bf.isPlayer) {
                        // 移除空袭印记
                        auto it = std::find(bf.card.marks.begin(), bf.card.marks.end(), std::string(u8"空袭"));
                        if (it != bf.card.marks.end()) {
                            bf.card.marks.erase(it);
                        }
                    }
                }
            }
            fengyaShanAirstrikeSlots_.clear();
            
            // 敌方回合结束，进行胜负判定
            checkGameOver();
            if (currentPhase_ == GamePhase::GameOver) return; // 游戏结束，不再继续
            
			currentPhase_ = GamePhase::PlayerTurn;
			// 重置本回合献祭次数
			sacrificeCountThisTurn_ = 0;
			if (mustDrawThisTurn_) {
				statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始 - 必须先抽牌！";
			}
			else {
				statusMessage_ = "第 " + std::to_string(currentTurn_) + " 回合开始";
			}
		}
		else if (currentPhase_ == GamePhase::PlayerTurn) {
			// 玩家回合结束，进行胜负判定
			checkGameOver();
			if (currentPhase_ == GamePhase::GameOver) return; // 游戏结束，不再继续
			
			currentPhase_ = GamePhase::EnemyTurn;
			currentTurn_++; // 有移动时，从玩家回合切到敌方回合也需要递增回合数
			hasDrawnThisTurn_ = false; // 重置抽牌状态
			mustDrawThisTurn_ = (currentTurn_ >= 2); // 第二回合开始必须抽牌
			std::cout << "1" << std::endl;
			// 延迟执行敌人回合
			enemyTurn();
		}
	}
}

bool BattleState::parseOneTurnGrowthTarget(const Card& card, std::string& outTargetName) {
	// 仅支持：
	//  - "一回合成长"（仅数值成长）
	//  - "一回合成长XXXX"（目标名直接拼接）
	for (const auto& mk : card.marks) {
		if (mk.rfind(u8"一回合成长", 0) == 0) {
			// 取前缀后的剩余部分作为目标名（可能为空）
			std::string tail = mk.substr(std::string(u8"一回合成长").size());
			// 去除两端空白
			while (!tail.empty() && (tail.front() == ' ' || tail.front() == '\t')) tail.erase(tail.begin());
			while (!tail.empty() && (tail.back() == ' ' || tail.back() == '\t')) tail.pop_back();
			outTargetName = tail; // 空字符串表示仅数值成长
			return true;
		}
	}
	return false;
}

std::string BattleState::findCardIdByName(const std::string& nameUtf8) {
	if (nameUtf8.empty()) return std::string();
	// 简单遍历 CardDB
	auto ids = CardDB::instance().allIds();
	for (const auto& id : ids) {
		const Card* proto = CardDB::instance().find(id);
		if (!proto) continue;
		if (proto->name == nameUtf8) return id;
	}
	return std::string();
}

bool BattleState::scheduleTurnStartGrowth() {
	pendingGrowth_.clear();
	for (int row = 0; row < BATTLEFIELD_ROWS; ++row) {
		for (int col = 0; col < BATTLEFIELD_COLS; ++col) {
			int idx = row * BATTLEFIELD_COLS + col;
			auto& bf = battlefield_[idx];
			if (!bf.isAlive || !bf.isPlayer) continue;
			if (bf.oneTurnGrowthApplied || bf.placedTurn <= 0 || currentTurn_ < bf.placedTurn + 1) continue;
			std::string targetName;
			if (!parseOneTurnGrowthTarget(bf.card, targetName)) continue;

			GrowthStep step;
			step.index = idx;
			if (!targetName.empty()) {
				std::string targetId = findCardIdByName(targetName);
				if (!targetId.empty()) {
					step.willTransform = true;
					step.targetCard = CardDB::instance().make(targetId);
					// 存储原始卡牌数据，用于计算差值
					Card originalCard = CardDB::instance().make(bf.card.id);
					step.addAttack = originalCard.attack;
					step.addHealth = originalCard.health;
					pendingGrowth_.push_back(step);
					continue;
				}
			}
			// 默认数值成长
			step.willTransform = false;
			step.addAttack = 1;
			step.addHealth = 2;
			pendingGrowth_.push_back(step);
		}
	}
	if (!pendingGrowth_.empty()) {
		isGrowthAnimating_ = true;
		growthAnimTime_ = 0.0f;
		growthAtTurnStart_ = true;
		statusMessage_ = "成长中...";
		return true;
	}
	return false;
}
bool BattleState::scheduleEnemyPreAttackGrowth() {
	pendingGrowth_.clear();
	for (int row = 1; row < 2; ++row) { // 仅第二行（敌方攻击行）
		for (int col = 0; col < BATTLEFIELD_COLS; ++col) {
			int idx = row * BATTLEFIELD_COLS + col;
			auto& bf = battlefield_[idx];
			if (!bf.isAlive || bf.isPlayer) continue;
			// 只有进入第二行后的"下一个回合"才能成长
			if (bf.oneTurnGrowthApplied || bf.placedTurn <= 0 || currentTurn_ < bf.placedTurn + 1) continue;
			std::string targetName;
			if (!parseOneTurnGrowthTarget(bf.card, targetName)) continue;

			GrowthStep step;
			step.index = idx;
			if (!targetName.empty()) {
				std::string targetId = findCardIdByName(targetName);
				if (!targetId.empty()) {
					step.willTransform = true;
					step.targetCard = CardDB::instance().make(targetId);
					// 存储原始卡牌数据，用于计算差值
					Card originalCard = CardDB::instance().make(bf.card.id);
					step.addAttack = originalCard.attack;
					step.addHealth = originalCard.health;
					pendingGrowth_.push_back(step);
					continue;
				}
			}
			// 默认数值成长
			step.willTransform = false;
			step.addAttack = 1;
			step.addHealth = 2;
			pendingGrowth_.push_back(step);
		}
	}
	if (!pendingGrowth_.empty()) {
		isGrowthAnimating_ = true;
		growthAnimTime_ = 0.0f;
		growthAtTurnStart_ = false;
		growthForEnemyAttack_ = true;
		statusMessage_ = "敌方成长中...";
		return true;
	}
	return false;
}

// 水袭印记相关方法实现
void BattleState::updateWaterAttackMarks() {
	// 根据当前回合阶段更新水袭印记状态
	if (currentPhase_ == GamePhase::PlayerTurn) {
		// 玩家回合：我方水袭卡牌浮出，敌方水袭卡牌潜水
		applyWaterAttackSurfacing();
	}
	else if (currentPhase_ == GamePhase::EnemyTurn) {
		// 敌人回合：我方水袭卡牌潜水，敌方水袭卡牌浮出
		applyWaterAttackDiving();
	}
	
	// 水袭翻面逻辑：根据回合和卡牌归属控制翻面状态
	for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
		auto& bf = battlefield_[i];
		if (bf.isAlive && hasMark(bf.card, u8"水袭")) {
			if (currentPhase_ == GamePhase::PlayerTurn) {
				// 我方回合：我方水袭卡牌翻到正面，敌方水袭卡牌翻到反面
				waterAttackFlipped_[i] = !bf.isPlayer;
			}
			else if (currentPhase_ == GamePhase::EnemyTurn) {
				// 敌方回合：我方水袭卡牌翻到反面，敌方水袭卡牌翻到正面
				waterAttackFlipped_[i] = bf.isPlayer;
			}
		}
	}
}

void BattleState::applyWaterAttackDiving() {
	// 我方水袭卡牌在敌人回合潜水
	for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
		auto& bf = battlefield_[i];
		if (bf.isAlive && bf.isPlayer && hasMark(bf.card, u8"水袭")) {
			bf.isDiving = true;
		}
	}
	
	// 敌方水袭卡牌在敌人回合浮出
	for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
		auto& bf = battlefield_[i];
		if (bf.isAlive && !bf.isPlayer && hasMark(bf.card, u8"水袭")) {
			bf.isDiving = false;
		}
	}
}

void BattleState::applyWaterAttackSurfacing() {
	// 我方水袭卡牌在玩家回合浮出
	for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
		auto& bf = battlefield_[i];
		if (bf.isAlive && bf.isPlayer && hasMark(bf.card, u8"水袭")) {
			bf.isDiving = false;
		}
	}
	
	// 敌方水袭卡牌在玩家回合潜水
	for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
		auto& bf = battlefield_[i];
		if (bf.isAlive && !bf.isPlayer && hasMark(bf.card, u8"水袭")) {
			bf.isDiving = true;
		}
	}
}

void BattleState::renderDeckSelection(App& app) {
	SDL_Renderer* renderer = app.getRenderer();
	
	// 绘制半透明背景
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
	SDL_Rect overlayRect = { 0, 0, screenW_, screenH_ };
	SDL_RenderFillRect(renderer, &overlayRect);
	
	// 计算牌堆卡牌的显示区域 - 居中动态排布
	int cardWidth = searchAllCardsMode_ ? 90 : 120;   // 全卡库更小
	int cardHeight = searchAllCardsMode_ ? 135 : 180; // 全卡库更小
	int cardSpacing = searchAllCardsMode_ ? 10 : 15;  // 全卡库更紧凑
	int rowSpacing = searchAllCardsMode_ ? 14 : 20;   // 全卡库更紧凑
	
	// 动态计算每行卡牌数和布局
	auto& store = DeckStore::instance();
	int totalCards = static_cast<int>(searchAllCardsMode_ ? searchCandidates_.size() : store.library().size());
	if (totalCards == 0) return;
	
	// 根据屏幕宽度和卡牌数量动态计算每行卡牌数
	int maxCardsPerRow = (screenW_ - 100) / (cardWidth + cardSpacing);
	int cardsPerRow = std::min(maxCardsPerRow, totalCards);
	if (cardsPerRow < 1) cardsPerRow = 1;
	
	// 计算总行数
	int totalRows = (totalCards + cardsPerRow - 1) / cardsPerRow;
	
	// 计算每行实际卡牌数（最后一行可能不满）
	int cardsInLastRow = totalCards % cardsPerRow;
	if (cardsInLastRow == 0) cardsInLastRow = cardsPerRow;
	
	// 计算整体布局的起始位置（水平和垂直居中）
	int totalLayoutWidth = cardsPerRow * cardWidth + (cardsPerRow - 1) * cardSpacing;
	int totalLayoutHeight = totalRows * cardHeight + (totalRows - 1) * rowSpacing;
	
	int startX = (screenW_ - totalLayoutWidth) / 2;
	int startY = (screenH_ - totalLayoutHeight) / 2;
	
	// 渲染牌堆中的所有卡牌
	for (size_t i = 0; i < (searchAllCardsMode_ ? searchCandidates_.size() : store.library().size()); ++i) {
		int row = static_cast<int>(i) / cardsPerRow;
		int col = static_cast<int>(i) % cardsPerRow;
		
		// 计算当前行的卡牌数
		int cardsInCurrentRow = (row == totalRows - 1) ? cardsInLastRow : cardsPerRow;
		
		// 计算当前行的起始位置（居中）
		int currentRowWidth = cardsInCurrentRow * cardWidth + (cardsInCurrentRow - 1) * cardSpacing;
		int currentRowStartX = (screenW_ - currentRowWidth) / 2;
		
		int cardX = currentRowStartX + col * (cardWidth + cardSpacing);
		int cardY = startY + row * (cardHeight + rowSpacing);
		
		// 使用和战斗界面一样的卡牌渲染方式
		const std::string& cid = searchAllCardsMode_ ? searchCandidates_[i] : store.library()[i].id;
		Card card = CardDB::instance().make(cid);
		if (!card.id.empty()) {
			SDL_Rect cardRect = { cardX, cardY, cardWidth, cardHeight };
			// 使用CardRenderer渲染卡牌，和战斗界面保持一致
			CardRenderer::renderCard(app, card, cardRect, cardNameFont_, cardStatFont_, false);
		}
	}
	
	// 显示提示文字
	if (cardNameFont_) {
		SDL_Color textColor = { 255, 255, 0, 255 };
		SDL_Surface* textSurface = TTF_RenderUTF8_Blended(cardNameFont_, u8"请选择一张卡牌加入手牌", textColor);
		if (textSurface) {
			SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
			SDL_Rect textRect = { 50, 150, textSurface->w, textSurface->h };
			SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
			SDL_DestroyTexture(textTexture);
			SDL_FreeSurface(textSurface);
		}
	}
}

// 道具系统实现
std::string BattleState::getRandomItem() {
	if (AVAILABLE_ITEMS.empty()) {
		return "";
	}
	int randomIndex = rand() % AVAILABLE_ITEMS.size();
	return AVAILABLE_ITEMS[randomIndex];
}

void BattleState::addItem(const std::string& itemId, int count) {
	// 道具系统不支持叠加，每个道具占用独立槽位
	// 检查道具栏是否已满
	if (playerItems_.size() < MAX_ITEMS) {
		// 根据itemId创建对应的道具
		Item newItem;
		if (itemId == "yinyang_pei") {
			newItem = Item("yinyang_pei", u8"阴阳佩", u8"对敌人本体造成1点伤害", count);
		} else if (itemId == "mobao_ping") {
			newItem = Item("mobao_ping", u8"墨宝瓶", u8"使用后手牌出现一张墨锭", count);
		} else if (itemId == "wangchuan_shi") {
			newItem = Item("wangchuan_shi", u8"忘川石", u8"使用后手牌获得一张磐石", count);
		} else if (itemId == "fengya_shan") {
			newItem = Item("fengya_shan", u8"风雅扇", u8"使用后我方场上的牌获得一回合的空袭效果", count);
		} else if (itemId == "rigui") {
			newItem = Item("rigui", u8"日晷", u8"敌方跳过一回合，玩家结束回合后再次到玩家回合", count);
		} else if (itemId == "duanyinjian") {
			newItem = Item("duanyinjian", u8"断因剑", u8"使用后可以选择敌人第二行的任意一张牌直接摧毁", count);
		} else if (itemId == "tunmohao") {
			newItem = Item("tunmohao", u8"吞墨毫", u8"使用后摧毁敌人第二行任意一张牌，摧毁后手牌获得一张狼皮", count);
		} else if (itemId == "fuhunsuo") {
			newItem = Item("fuhunsuo", u8"缚魂锁", u8"使用后可以将敌人第二行的某一张卡牌移动到我方第三行空位", count);
		} else if (itemId == "wuzitianshu") {
			newItem = Item("wuzitianshu", u8"无字天书", u8"使用后删除对面场上所有卡牌的印记效果", count);
		} else if (itemId == "xuanmuping") {
			newItem = Item("xuanmuping", u8"玄牧瓶", u8"使用后手牌获得一张玄牧", count);
		} else if (itemId == "tianjiluopan") {
			newItem = Item("tianjiluopan", u8"天机罗盘", u8"使用后可以从牌组中检索一张牌加入手牌", count);
		} else if (itemId == "sanguiping") {
			newItem = Item("sanguiping", u8"散骨瓶", u8"使用后获得4个魂骨", count);
		} else {
			newItem = Item(itemId, "未知道具", "道具描述", count);
		}
		playerItems_.push_back(newItem);
		statusMessage_ = "获得新道具：" + newItem.name + " x" + std::to_string(count);
	} else {
		statusMessage_ = "道具栏已满，无法获得新道具";
	}
}

bool BattleState::removeItem(const std::string& itemId, int count) {
	for (auto it = playerItems_.begin(); it != playerItems_.end(); ++it) {
		if (it->id == itemId) {
			it->count -= count;
			if (it->count <= 0) {
				playerItems_.erase(it);
			}
			return true;
		}
	}
	return false;
}

bool BattleState::hasItem(const std::string& itemId) const {
	for (const auto& item : playerItems_) {
		if (item.id == itemId) {
			return true;
		}
	}
	return false;
}

int BattleState::getItemCount(const std::string& itemId) const {
	for (const auto& item : playerItems_) {
		if (item.id == itemId) {
			return item.count;
		}
	}
	return 0;
}

void BattleState::useItem(const std::string& itemId) {
	// 根据不同的道具ID实现不同的效果
	if (itemId == "yinyang_pei") {
		// 阴阳佩：对敌人本体造成1点伤害
		if (removeItem(itemId, 1)) {
			enemyHealth_ -= 1;
			if (enemyHealth_ < 0) enemyHealth_ = 0;
			
			// 更新墨尺：直接增加伤害值
			meterActualPos_ += 1;
			
			// 显示值锁定在5，但实际值继续累加
			meterTargetPos_ = std::min(5, meterActualPos_);
			
			// 启动墨尺指针动画
			isMeterAnimating_ = true;
			meterAnimTime_ = 0.0f;
			
			statusMessage_ = u8"使用阴阳佩，对敌人造成1点伤害！";
		} else {
			statusMessage_ = "没有该道具";
		}
	} else if (itemId == "mobao_ping") {
		// 墨宝瓶：手牌出现一张墨锭
		if (removeItem(itemId, 1)) {
			// 添加墨锭到手牌
			Card inkCard = CardDB::instance().make("moding");
			if (!inkCard.id.empty()) {
				handCards_.push_back(inkCard);
				layoutHandCards();  // 重新布局手牌
				statusMessage_ = u8"使用墨宝瓶，获得一张墨锭！";
			} else {
				statusMessage_ = "无法创建墨锭卡牌";
			}
		} else {
			statusMessage_ = "没有该道具";
		}
	} else if (itemId == "wangchuan_shi") {
		// 忘川石：手牌获得一张磐石
		if (removeItem(itemId, 1)) {
			// 添加磐石到手牌
			Card rockCard = CardDB::instance().make("panshi");
			if (!rockCard.id.empty()) {
				handCards_.push_back(rockCard);
				layoutHandCards();  // 重新布局手牌
				statusMessage_ = u8"使用忘川石，获得一张磐石！";
			} else {
				statusMessage_ = "无法创建磐石卡牌";
			}
		} else {
			statusMessage_ = "没有该道具";
		}
	} else if (itemId == "fengya_shan") {
		// 风雅扇：我方场上的牌获得一回合的空袭效果
		if (removeItem(itemId, 1)) {
			// 清理之前的风雅扇效果
			fengyaShanAirstrikeSlots_.clear();
			
			// 给我方场上的所有卡牌添加空袭效果
			int airstrikeCount = 0;
			for (int i = 0; i < TOTAL_BATTLEFIELD_SLOTS; ++i) {
				auto& bf = battlefield_[i];
				if (bf.isAlive && bf.isPlayer) {
					// 给卡牌添加空袭印记
					bf.card.marks.push_back(u8"空袭");
					// 记录这个位置，用于回合结束时清理
					fengyaShanAirstrikeSlots_.push_back(i);
					airstrikeCount++;
				}
			}
			
			if (airstrikeCount > 0) {
				statusMessage_ = u8"使用风雅扇，我方" + std::to_string(airstrikeCount) + u8"张卡牌获得空袭效果！";
			} else {
				statusMessage_ = u8"使用风雅扇，但我方场上没有卡牌！";
			}
		} else {
			statusMessage_ = "没有该道具";
		}
	} else if (itemId == "rigui") {
		// 日晷：敌方跳过一回合
		if (removeItem(itemId, 1)) {
			// 激活日晷效果
			riguiEffectActive_ = true;
			statusMessage_ = u8"使用日晷，敌方将跳过下一回合！";
		} else {
			statusMessage_ = "没有该道具";
		}
	} else if (itemId == "duanyinjian") {
		// 断因剑：选择敌人第二行任意一张牌直接摧毁
		// 检查敌人第二行是否有卡牌
		bool hasEnemyInSecondRow = false;
		for (int col = 0; col < BATTLEFIELD_COLS; ++col) {
			int index = 1 * BATTLEFIELD_COLS + col; // 第二行索引4-7
			if (battlefield_[index].isAlive && !battlefield_[index].isPlayer) {
				hasEnemyInSecondRow = true;
				break;
			}
		}
		
		if (!hasEnemyInSecondRow) {
			statusMessage_ = u8"断因剑：敌人第二行没有卡牌，无法使用！";
			return;
		}
		
		if (removeItem(itemId, 1)) {
			// 进入目标选择模式
			isSelectingTarget_ = true;
			selectedTargetIndex_ = -1;
			statusMessage_ = u8"断因剑激活！请点击敌人第二行的卡牌进行摧毁！";
		} else {
			statusMessage_ = "没有该道具";
		}
	} else if (itemId == "tunmohao") {
		// 吞墨毫：选择敌人第二行任意一张牌直接摧毁，摧毁后手牌获得一张狼皮
		// 检查敌人第二行是否有卡牌
		bool hasEnemyInSecondRow = false;
		for (int col = 0; col < BATTLEFIELD_COLS; ++col) {
			int index = 1 * BATTLEFIELD_COLS + col; // 第二行索引4-7
			if (battlefield_[index].isAlive && !battlefield_[index].isPlayer) {
				hasEnemyInSecondRow = true;
				break;
			}
		}
		
		if (!hasEnemyInSecondRow) {
			statusMessage_ = u8"吞墨毫：敌人第二行没有卡牌，无法使用！";
			return;
		}
		
		if (removeItem(itemId, 1)) {
			// 进入目标选择模式
			isSelectingTunmohaoTarget_ = true;
			selectedTunmohaoTargetIndex_ = -1;
			statusMessage_ = u8"吞墨毫激活！请点击敌人第二行的卡牌进行摧毁！";
		} else {
			statusMessage_ = "没有该道具";
		}
	} else if (itemId == "fuhunsuo") {
		// 缚魂锁：选择敌人第二行任意一张牌移动到我方第三行空位
		// 检查敌人第二行是否有卡牌
		bool hasEnemyInSecondRow = false;
		for (int col = 0; col < BATTLEFIELD_COLS; ++col) {
			int index = 1 * BATTLEFIELD_COLS + col; // 第二行索引4-7
			if (battlefield_[index].isAlive && !battlefield_[index].isPlayer) {
				hasEnemyInSecondRow = true;
				break;
			}
		}
		
		if (!hasEnemyInSecondRow) {
			statusMessage_ = u8"缚魂锁：敌人第二行没有卡牌，无法使用！";
			return;
		}
		
		if (removeItem(itemId, 1)) {
			// 进入目标选择模式
			isSelectingFuhunsuoTarget_ = true;
			selectedFuhunsuoTargetIndex_ = -1;
			statusMessage_ = u8"缚魂锁激活！请点击敌人第二行的卡牌进行移动！";
		} else {
			statusMessage_ = "没有该道具";
		}
	} else if (itemId == "wuzitianshu") {
		// 无字天书：删除对面场上所有卡牌的印记效果
		if (removeItem(itemId, 1)) {
			int removedMarksCount = 0;
			// 遍历敌方场上所有卡牌（第一行和第二行）
			for (int row = 0; row < 2; ++row) {
				for (int col = 0; col < BATTLEFIELD_COLS; ++col) {
					int index = row * BATTLEFIELD_COLS + col;
					if (battlefield_[index].isAlive && !battlefield_[index].isPlayer) {
						// 清空敌方卡牌的所有印记
						if (!battlefield_[index].card.marks.empty()) {
							removedMarksCount += static_cast<int>(battlefield_[index].card.marks.size());
							battlefield_[index].card.marks.clear();
						}
					}
				}
			}
			statusMessage_ = u8"无字天书生效！删除了敌方 " + std::to_string(removedMarksCount) + " 个印记效果！";
		} else {
			statusMessage_ = "没有该道具";
		}
	} else if (itemId == "xuanmuping") {
		// 玄牧瓶：手牌获得一张玄牧
		if (removeItem(itemId, 1)) {
			Card xuanmuCard = CardDB::instance().make("xuanmu");
			if (xuanmuCard.id != "unknown") {
				EngraveStore::instance().applyToCard(xuanmuCard);
				handCards_.push_back(xuanmuCard);
				layoutHandCards();
				statusMessage_ = u8"玄牧瓶生效！获得玄牧！";
			} else {
				statusMessage_ = u8"玄牧瓶生效，但无法获得玄牧卡牌！";
			}
		} else {
			statusMessage_ = "没有该道具";
		}
	} else if (itemId == "tianjiluopan") {
		// 天机罗盘：从牌组中检索一张牌加入手牌
		if (removeItem(itemId, 1)) {
			// 启动检索状态（使用检索印记的函数）
			isSearchingDeck_ = true;
			selectedDeckCardIndex_ = -1;
			statusMessage_ = u8"天机罗盘激活！请从牌组中选择一张卡牌！";
		} else {
			statusMessage_ = "没有该道具";
		}
	} else if (itemId == "sanguiping") {
		// 散骨瓶：获得4个魂骨
		if (removeItem(itemId, 1)) {
			boneCount_ += 4;
			statusMessage_ = u8"散骨瓶生效！获得4个魂骨！当前魂骨数量: " + std::to_string(boneCount_);
		} else {
			statusMessage_ = "没有该道具";
		}
	} else {
		// 其他道具暂时只是移除
		if (removeItem(itemId, 1)) {
			statusMessage_ = "使用了道具：" + itemId;
		} else {
			statusMessage_ = "没有该道具";
		}
	}
}

void BattleState::initializeItems() {
	// 初始化所有14种道具的定义
	// 暂时为空，道具会在获得时动态创建
}

void BattleState::renderEngravingHints(App& app) {
	SDL_Renderer* r = app.getRenderer();
	
	// 获取已组合的意境
	auto& store = EngraveStore::instance();
	const auto& bindings = store.bindings();
	
	if (bindings.empty()) return;
	
	// 在右侧显示已组合意境
	int startX = screenW_ - 200;
	int startY = 100;
	int lineHeight = 25;
	
	if (infoFont_) {
		SDL_Color titleColor{255, 255, 100, 255}; // 金黄色标题
		SDL_Surface* titleSurface = TTF_RenderUTF8_Blended(infoFont_, u8"已组合意境", titleColor);
		if (titleSurface) {
			SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(r, titleSurface);
			SDL_Rect titleRect{startX, startY, titleSurface->w, titleSurface->h};
			SDL_RenderCopy(r, titleTexture, nullptr, &titleRect);
			SDL_DestroyTexture(titleTexture);
			SDL_FreeSurface(titleSurface);
			startY += titleSurface->h + 10;
		}
	}
	
	// 显示每个组合
	SDL_Color textColor{200, 200, 200, 255};
	for (const auto& kv : bindings) {
		const std::string& category = kv.first;
		const auto& marks = kv.second;
		
		for (const auto& mark : marks) {
			std::string hintText = category + " + " + mark;
			SDL_Surface* s = TTF_RenderUTF8_Blended(infoFont_, hintText.c_str(), textColor);
			if (s) {
				SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
				SDL_Rect d{startX, startY, s->w, s->h};
				SDL_RenderCopy(r, t, nullptr, &d);
				SDL_DestroyTexture(t);
				SDL_FreeSurface(s);
				startY += lineHeight;
			}
		}
	}
}

// 缚魂锁移动动画更新
void BattleState::updateFuhunsuoAnimation(float dt) {
	if (!isFuhunsuoAnimating_) return;

	fuhunsuoAnimTime_ += dt;

	// 动画完成
	if (fuhunsuoAnimTime_ >= fuhunsuoAnimDuration_) {
		// 目标卡牌已经在动画开始时设置为存活状态，这里只需要重置动画状态
		isFuhunsuoAnimating_ = false;
		fuhunsuoAnimTime_ = 0.0f;
		fuhunsuoFromIndex_ = -1;
		fuhunsuoToIndex_ = -1;
	}
}