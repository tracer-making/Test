#pragma once

#include "Card.h"
#include <vector>
#include <string>

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
	void drawToHand(int n = 1);
	void drawFromInkPile(int n = 1);
	void discardFromHand(int index);
	void inheritMarks(int srcHandIndex, int dstHandIndex); // 文脉传承（手牌内）
	
	// 初始化玩家牌堆
	void initializePlayerDeck();

private:
	DeckStore() = default;
	std::vector<Card> hand_;
	std::vector<Card> library_;
	std::vector<Card> discard_;
	std::vector<Card> inkPile_;
};


