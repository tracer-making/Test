#pragma once

#include <vector>
#include <string>

// 全局持久化地图存储
class MapStore {
public:
	struct MapNodeData {
		int layer = 0;
		int x = 0;
		int y = 0;
		enum class NodeType { START, NORMAL, BOSS, ELITE, SHOP, EVENT } type = NodeType::NORMAL;
		bool visited = false;
		bool accessible = false;
		std::vector<int> connections;
		int size = 44;
		std::string label;
	};

	static MapStore& instance() {
		static MapStore s;
		return s;
	}

	// 状态
	bool hasMap() const { return generated_; }
	void clear() {
		layerNodes_.clear();
		numLayers_ = 0;
		startNodeIdx_ = -1;
		bossNodeIdx_ = -1;
		currentMapLayer_ = 1;
		playerCurrentNode_ = -1;
		accessibleNodes_.clear();
		generated_ = false;
	}

	// 数据访问
	std::vector<std::vector<MapNodeData>>& layerNodes() { return layerNodes_; }
	int& numLayers() { return numLayers_; }
	int& startNodeIdx() { return startNodeIdx_; }
	int& bossNodeIdx() { return bossNodeIdx_; }
	int& currentMapLayer() { return currentMapLayer_; }
	int& playerCurrentNode() { return playerCurrentNode_; }
	std::vector<int>& accessibleNodes() { return accessibleNodes_; }
	bool& generated() { return generated_; }

private:
	MapStore() = default;
	~MapStore() = default;
	MapStore(const MapStore&) = delete;
	MapStore& operator=(const MapStore&) = delete;

	bool generated_ = false;
	std::vector<std::vector<MapNodeData>> layerNodes_{};
	int numLayers_ = 0;
	int startNodeIdx_ = -1;
	int bossNodeIdx_ = -1;
	int currentMapLayer_ = 1;
	int playerCurrentNode_ = -1;
	std::vector<int> accessibleNodes_{};
};


