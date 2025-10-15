#pragma once

#include <string>
#include <vector>

struct Card {
	std::string id;            // 全局唯一ID
	std::string name;          // 名称
	int sacrificeCost = 0;     // 献祭消耗
	std::string category;      // 种类（如：兵器/诗篇/墨/印/卷等）
	int attack = 0;            // 攻击力
	int health = 0;            // 生命值
	std::string face;          // 牌面（资源ID/图片路径/样式名）
	std::vector<std::string> marks; // 印记
	bool canInherit = true;    // 是否可传承（接受过传承的卡变为false）
	std::string instanceId;    // 实例唯一ID（用于区分同名卡牌）
	
	// 默认构造函数
	Card() = default;
	
	// 带参数的构造函数
	Card(const std::string& id, const std::string& name, int sacrificeCost, const std::string& category, int attack, int health, const std::string& face, const std::vector<std::string>& marks = {})
		: id(id), name(name), sacrificeCost(sacrificeCost), category(category), attack(attack), health(health), face(face), marks(marks) {}
};


