#include "ItemStore.h"

ItemStore& ItemStore::instance() {
	static ItemStore s; return s;
}

const std::vector<ItemStore::Item>& ItemStore::items() const { return items_; }
std::vector<ItemStore::Item>& ItemStore::items() { return items_; }

int ItemStore::totalCount() const {
	return (int)items_.size();
}

void ItemStore::addItem(const std::string& id, const std::string& name, const std::string& desc, int count) {
	for (auto& it : items_) {
		if (it.id == id) { it.count += count; return; }
	}
	items_.push_back(Item{id, name, desc, count});
}

bool ItemStore::removeItem(const std::string& id, int count) {
	for (auto it = items_.begin(); it != items_.end(); ++it) {
		if (it->id == id) {
			it->count -= count;
			if (it->count <= 0) items_.erase(it);
			return true;
		}
	}
	return false;
}

bool ItemStore::hasItem(const std::string& id) const {
	for (const auto& it : items_) if (it.id == id && it.count > 0) return true; return false;
}

int ItemStore::getItemCount(const std::string& id) const {
	for (const auto& it : items_) if (it.id == id) return it.count; return 0;
}

void ItemStore::clear() { items_.clear(); }


