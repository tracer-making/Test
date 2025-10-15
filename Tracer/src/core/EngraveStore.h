#pragma once

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "Card.h"

// 全局：意境刻画存储
class EngraveStore {
public:
	static EngraveStore& instance() {
		static EngraveStore s; return s;
	}

    // 收集备选：意(印记) 与 境(部类)
    void selectYi(const std::string& mark) { addYi(mark); }
    void selectJing(const std::string& category) { addJing(category); }

    void addYi(const std::string& mark) {
        if (mark.empty()) return;
        if (std::find(yis_.begin(), yis_.end(), mark) == yis_.end()) yis_.push_back(mark);
        // 自动组合：当有至少一个意和一个境时，自动组合第一个
        autoCombine();
    }
    void addJing(const std::string& cat) {
        if (cat.empty()) return;
        if (std::find(jings_.begin(), jings_.end(), cat) == jings_.end()) jings_.push_back(cat);
        // 自动组合：当有至少一个意和一个境时，自动组合第一个
        autoCombine();
    }

	// 获取/清理
    const std::vector<std::string>& yis() const { return yis_; }
    const std::vector<std::string>& jings() const { return jings_; }
	const std::map<std::string, std::vector<std::string>>& bindings() const { return categoryToMarks_; }
    // 手动选择组合（一次只能有一个组合）
    void combine(const std::string& cat, const std::string& mark) {
        if (cat.empty() || mark.empty()) return;
        // 清除所有现有组合，只保留新的组合
        categoryToMarks_.clear();
        auto& vec = categoryToMarks_[cat];
        vec.push_back(mark);
        // 设置组合提示
        lastCombineHint_ = cat + " + " + mark;
    }

    // 获取最后的组合提示
    const std::string& getLastCombineHint() const { return lastCombineHint_; }
    void clearLastCombineHint() { lastCombineHint_.clear(); }

	// 将意境效果应用到一张卡：若卡牌category命中绑定，则补充对应印记（若未存在）
	void applyToCard(Card& card) const {
		auto it = categoryToMarks_.find(card.category);
		if (it == categoryToMarks_.end()) return;
		for (const auto& mk : it->second) {
			if (std::find(card.marks.begin(), card.marks.end(), mk) == card.marks.end()) {
				card.marks.push_back(mk);
			}
		}
	}

private:
    // 自动组合：只有当有恰好一个意和一个境时才自动组合
    void autoCombine() {
        if (yis_.size() == 1 && jings_.size() == 1) {
            // 恰好只有一个意和一个境，自动组合
            combine(jings_[0], yis_[0]);
        }
        // 如果有多个意或多个境，不自动组合，让玩家选择
    }

private:
	EngraveStore() = default; ~EngraveStore() = default;
	EngraveStore(const EngraveStore&) = delete; EngraveStore& operator=(const EngraveStore&) = delete;

    std::vector<std::string> yis_;
    std::vector<std::string> jings_;
	std::map<std::string, std::vector<std::string>> categoryToMarks_;
    std::string lastCombineHint_; // 最后的组合提示
};


