#pragma once

#include "Card.h"
#include <vector>
#include <string>
#include <unordered_map>

// 简单的牌组/玩家持有结构：牌库、手牌、弃牌
class DeckStore {
public:
	static DeckStore& instance();

	// 数据访问
	std::vector<Card>& hand();
	std::vector<Card>& library();
	std::vector<Card>& discard();
	std::vector<Card>& inkPile();

	// 工具操作
	void clearAll();
	void addToLibrary(const Card& c);
	void addCardToHand(const Card& c);
	void drawToHand(int n = 1);
	void drawFromInkPile(int n = 1);
	void discardFromHand(int index);
	void inheritMarks(int srcHandIndex, int dstHandIndex); // 文脉传承（手牌内）
	
	// 初始化玩家牌堆
	void initializePlayerDeck();
	
	// 设置和获取战斗中的待更新数值
	void setPendingCardUpdates(const std::unordered_map<std::string, std::pair<int, int>>& updates);
	const std::unordered_map<std::string, std::pair<int, int>>& getPendingCardUpdates() const;
	
	// 获取资源数量
	int getBoneCount() const;
	int getInkCount() const;

private:
	DeckStore() = default;
	std::vector<Card> hand_;
	std::vector<Card> library_;
	std::vector<Card> discard_;
	std::vector<Card> inkPile_;
	
	// 战斗中的待更新数值
	std::unordered_map<std::string, std::pair<int, int>> pendingCardUpdates_;
};


