#include "Deck.h"
#include "Cards.h"
#include <algorithm>

DeckStore& DeckStore::instance() {
	static DeckStore s;
	return s;
}

std::vector<Card>& DeckStore::hand() { return hand_; }
std::vector<Card>& DeckStore::library() { return library_; }
std::vector<Card>& DeckStore::discard() { return discard_; }
std::vector<Card>& DeckStore::inkPile() { return inkPile_; }

void DeckStore::clearAll() {
	hand_.clear(); library_.clear(); discard_.clear(); inkPile_.clear();
}

void DeckStore::addToLibrary(const Card& c) { library_.push_back(c); }

void DeckStore::drawToHand(int n) {
	while (n-- > 0 && !library_.empty()) {
		hand_.push_back(library_.back());
		library_.pop_back();
	}
}

void DeckStore::drawFromInkPile(int n) {
	while (n-- > 0 && !inkPile_.empty()) {
		hand_.push_back(inkPile_.back());
		inkPile_.pop_back();
	}
}

void DeckStore::discardFromHand(int index) {
	if (index < 0 || index >= (int)hand_.size()) return;
	discard_.push_back(hand_[index]);
	hand_.erase(hand_.begin() + index);
}

void DeckStore::inheritMarks(int srcHandIndex, int dstHandIndex) {
	if (srcHandIndex < 0 || dstHandIndex < 0) return;
	if (srcHandIndex >= (int)hand_.size() || dstHandIndex >= (int)hand_.size()) return;
	if (srcHandIndex == dstHandIndex) return;
	Card& src = hand_[srcHandIndex];
	Card& dst = hand_[dstHandIndex];
	for (const auto& m : src.marks) {
		if (std::find(dst.marks.begin(), dst.marks.end(), m) == dst.marks.end()) dst.marks.push_back(m);
	}
	// 牺牲：移出源手牌
	hand_.erase(hand_.begin() + srcHandIndex);
	// 如果源在目标之前，目标索引左移，调用方应自行处理显示
}

void DeckStore::initializePlayerDeck() {
	// 清空所有牌堆
	clearAll();
	
	// 确保CardDB已加载
	CardDB::instance().loadBuiltinCards();
	
	// 使用战斗界面中的初始牌堆：两张书林署丞 + 一张守宫 + 一张刀笔吏 + 一张蛇自环
	std::vector<std::string> initialCardIds = {
		"shulin_shucheng",  // 书林署丞
		"shulin_shucheng",  // 书林署丞
		"shougong",         // 守宫
		"daobi_li",         // 刀笔吏
		"tengshe_zihuan"    // 蛇自环
	};
	
	// 将卡牌添加到牌库
	for (const auto& cardId : initialCardIds) {
		Card card = CardDB::instance().make(cardId);
		if (!card.id.empty()) {
			addToLibrary(card);
		}
	}
	
	// 初始化墨锭牌堆（10张墨锭）
	for (int i = 0; i < 10; ++i) {
		Card inkCard = CardDB::instance().make("moding");
		if (!inkCard.id.empty()) {
			inkPile_.push_back(inkCard);
		}
	}
	
	// 注意：不抽取手牌，让各个功能界面根据需要自行处理
}


