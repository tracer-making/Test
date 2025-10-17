#pragma once

#include <string>
#include <vector>

// 全局道具存储（单例）
class ItemStore {
public:
	struct Item {
		std::string id;
		std::string name;
		std::string description;
		int count = 0;
	};

	static ItemStore& instance();

	// 读取/修改
	const std::vector<Item>& items() const;
	std::vector<Item>& items();
	int totalCount() const; // 总道具件数（按条目累加count>0计为该条目个数，或按条目数？这里按条目数量）

	void addItem(const std::string& id, const std::string& name, const std::string& desc, int count = 1);
	bool removeItem(const std::string& id, int count = 1);
	bool hasItem(const std::string& id) const;
	int getItemCount(const std::string& id) const;
	void clear();

    // 全局战斗增益
    int extraInitialBones = 0; // 进入战斗时额外+骨头

private:
	ItemStore() = default;
	std::vector<Item> items_;
};


