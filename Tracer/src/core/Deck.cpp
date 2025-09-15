#include "Deck.h"
#include <algorithm>

DeckStore& DeckStore::instance() {
	static DeckStore s;
	return s;
}

std::vector<Card>& DeckStore::hand() { return hand_; }
std::vector<Card>& DeckStore::library() { return library_; }
std::vector<Card>& DeckStore::discard() { return discard_; }

void DeckStore::clearAll() {
	hand_.clear(); library_.clear(); discard_.clear();
}

void DeckStore::addToLibrary(const Card& c) { library_.push_back(c); }

void DeckStore::drawToHand(int n) {
	while (n-- > 0 && !library_.empty()) {
		hand_.push_back(library_.back());
		library_.pop_back();
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


