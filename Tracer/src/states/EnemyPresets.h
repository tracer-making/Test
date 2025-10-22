#pragma once

#include "../core/Card.h"
#include <vector>
#include <string>

// 敌方预设行结构
struct EnemyPresetRow {
    std::vector<std::string> cards; // 4列卡牌ID，空字符串表示不放置
    int placementType;              // -1=固定位置固定放置，其他值=预期生成数量（用于调控生成概率）
};

// 4列N行的敌方预设矩阵
struct EnemyPresetMatrix {
    // rows[0] = 敌人第六回合会放置（第一行）
    // rows[1] = 敌人第五回合会放置（第一行）
    // rows[2] = 敌人第四回合会放置（第一行）
    // rows[3] = 敌人第三回合会放置（第一行）
    // rows[4] = 敌人第二回合会放置（第一行）
    // rows[5] = 敌人第一回合会放置（第一行）
    // rows[6] = 初始化敌人第一行
    // rows[7] = 初始化战斗区域第二行
    // rows[8] = 初始化战斗区域第三行
    std::vector<EnemyPresetRow> rows; // 每行包含卡牌和随机标识
};

// 敌方预设管理器
class EnemyPresetManager {
public:
    static EnemyPresetManager& instance();

    // 根据战斗ID获取预设矩阵
    EnemyPresetMatrix getMatrix(int battleId) const;
    // 根据环境获取Boss战预设矩阵（林地/湿地/雪原）
    EnemyPresetMatrix getBossMatrixForBiome(const std::string& biome) const;
    // 根据环境与阶段(1/2)获取Boss预设；若无阶段2则返回阶段1
    EnemyPresetMatrix getBossMatrixForBiomePhase(const std::string& biome, int phase) const;
    
    // 获取可用的战斗ID列表
    std::vector<int> getAvailableBattleIds() const;
    
    // 根据地形层数获取可用的战斗ID池
    std::vector<int> getBattleIdsForBiome(const std::string& biome, int layer) const;
    // 随机分配不重复的战斗节点
    std::vector<int> getRandomBattleSequence(const std::string& biome, int layer, int nodeCount) const;
    

private:
    EnemyPresetManager() = default;
    ~EnemyPresetManager() = default;
    EnemyPresetManager(const EnemyPresetManager&) = delete;
    EnemyPresetManager& operator=(const EnemyPresetManager&) = delete;
};
