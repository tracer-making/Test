#include "Deck.h"
#include "Cards.h"
#include "App.h"
#include "EngraveStore.h"
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

void DeckStore::addCardToHand(const Card& c) { 
	Card card = c;
	// 意境系统：应用已组合的意境效果到卡牌
	EngraveStore::instance().applyToCard(card);
	hand_.push_back(card); 
}

void DeckStore::drawToHand(int n) {
	while (n-- > 0 && !library_.empty()) {
		Card card = library_.back();
		library_.pop_back();
		
		// 意境系统：应用已组合的意境效果到卡牌
		EngraveStore::instance().applyToCard(card);
		
		hand_.push_back(card);
	}
}

void DeckStore::drawFromInkPile(int n) {
	while (n-- > 0 && !inkPile_.empty()) {
		Card card = inkPile_.back();
		inkPile_.pop_back();
		
		// 意境系统：应用已组合的意境效果到卡牌
		EngraveStore::instance().applyToCard(card);
		
		hand_.push_back(card);
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
	
	// 根据选择的初始牌组设置不同的卡牌
	std::vector<std::string> initialCardIds;
	std::string selectedDeck = App::getSelectedInitialDeck();
	
	if (selectedDeck == "default_deck") {
		// 默认牌组：雪尾鼬生、碧蟾、朔漠苍狼
		initialCardIds = {"xuewei_yousheng", "bichan", "shuomuo_canglang"};
	} else if (selectedDeck == "xuanmu_deck") {
		// 玄牧牌组：玄牧、千峰驼鹿、穿坟鼹子
		initialCardIds = {"xuanmu", "qianfeng_tuolu", "chuanfen_yanzi"};
	} else if (selectedDeck == "ant_deck") {
		// 蚂蚁牌组：典诰蚁后、驿飞蚁、黄鼬臭尉
		initialCardIds = {"diangao_yihou", "yifei_yi", "huangyou_chouwei"};
	} else if (selectedDeck == "daobi_deck") {
		// 刀笔吏牌组：刀笔吏、卷册螟蛉、卷册螟蛉
		initialCardIds = {"daobi_li", "juance_mingling", "juance_mingling"};
	} else if (selectedDeck == "bone_deck") {
		// 魂骨牌组：浣沙溪生、雪原狼胚、野皋犺狗
		initialCardIds = {"huansha_xisheng", "xueyuan_langpei", "yegao_kangou"};
	} else if (selectedDeck == "weak_deck") {
		// 弱小牌组：白毫仔、玄贝蚪、守宫
		initialCardIds = {"baimao_zi", "xuanbei_dou", "shougong"};
	} else {
		// 默认牌组（如果没有选择或选择无效）
		initialCardIds = {"xuewei_yousheng", "bichan", "shuomuo_canglang"};
	}
	
	// 每个牌组都会获得两张兔皮
	initialCardIds.push_back("tuopi_mao");
	initialCardIds.push_back("tuopi_mao");
	
	// 将卡牌添加到牌库
	for (const auto& cardId : initialCardIds) {
		Card card = CardDB::instance().make(cardId);
		if (!card.id.empty()) {
			// 为牌库中的卡牌生成简单的数字实例ID
			static int cardCounter = 0;
			card.instanceId = std::to_string(cardCounter++);
			addToLibrary(card);
		}
	}
	
	// 初始化墨锭牌堆（10张墨锭）
	for (int i = 0; i < 10; ++i) {
		Card inkCard = CardDB::instance().make("moding");
		if (!inkCard.id.empty()) {
			// 为墨锭生成简单的数字实例ID
			static int cardCounter = 0;
			inkCard.instanceId = std::to_string(cardCounter++);
			inkPile_.push_back(inkCard);
		}
	}
	
	// 注意：不抽取手牌，让各个功能界面根据需要自行处理
}

void DeckStore::setPendingCardUpdates(const std::unordered_map<std::string, std::pair<int, int>>& updates) {
	pendingCardUpdates_ = updates;
}

const std::unordered_map<std::string, std::pair<int, int>>& DeckStore::getPendingCardUpdates() const {
	return pendingCardUpdates_;
}

int DeckStore::getBoneCount() const {
	// 计算所有卡牌中消耗骨头的数量
	int boneCount = 0;
	for (const auto& card : hand_) {
		for (const auto& mark : card.marks) {
			if (mark == u8"消耗骨头") {
				boneCount++;
				break;
			}
		}
	}
	return boneCount;
}

int DeckStore::getInkCount() const {
	// 计算所有卡牌的献祭消耗（墨滴）
	int inkCount = 0;
	for (const auto& card : hand_) {
		inkCount += card.sacrificeCost;
	}
	return inkCount;
}


