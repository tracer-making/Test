#pragma once

#include <string>
#include <vector>

struct Card {
	std::string id;            // 全局唯一ID
	std::string name;
	int attack = 0;
	int health = 0;
	std::vector<std::string> marks; // 印记
};


