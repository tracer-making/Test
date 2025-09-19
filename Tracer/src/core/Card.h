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
};


