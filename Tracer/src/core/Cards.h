#pragma once

#include "Card.h"
#include <string>
#include <vector>
#include <unordered_map>

// 全局卡牌数据库：注册游戏内的所有卡牌原型，通过ID检索
class CardDB {
public:
	static CardDB& instance();

	// 注册/覆盖卡牌原型（根据 id）
	void registerCard(const Card& proto);

	// 是否存在该卡
	bool contains(const std::string& id) const;

	// 获取原型指针（只读，可能为 nullptr）
	const Card* find(const std::string& id) const;

	// 生成一张卡牌实例（拷贝原型）。若不存在则返回默认 Card（id 为空）
	Card make(const std::string& id) const;

	// 所有卡ID
	std::vector<std::string> allIds() const;

	// 加载内置卡（启动时调用，一次即可）
	void loadBuiltinCards();

private:
	CardDB() = default;
	std::unordered_map<std::string, Card> idToProto_;
};
