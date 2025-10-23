#pragma once

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <random>
#include "Card.h"

// 敌人意境刻画存储
class EnemyEngraveStore {
public:
    static EnemyEngraveStore& instance() {
        static EnemyEngraveStore s; 
        return s;
    }

    // 为当前战斗生成敌人意境加成
    void generateEnemyEngrave(const std::vector<std::string>& enemyCardIds);
    
    // 将敌人意境效果应用到一张卡牌
    void applyToEnemyCard(Card& card) const;
    
    // 获取当前敌人意境信息
    const std::string& getEnemyYi() const { return enemyYi_; }
    const std::string& getEnemyJing() const { return enemyJing_; }
    const std::string& getEnemyEngraveInfo() const { return enemyEngraveInfo_; }
    
    // 清除当前敌人意境
    void clear() {
        enemyYi_.clear();
        enemyJing_.clear();
        enemyEngraveInfo_.clear();
    }

private:
    EnemyEngraveStore() = default;
    ~EnemyEngraveStore() = default;
    EnemyEngraveStore(const EnemyEngraveStore&) = delete;
    EnemyEngraveStore& operator=(const EnemyEngraveStore&) = delete;

    // 获取有意义的印记列表（排除对敌人无收益的印记）
    std::vector<std::string> getMeaningfulMarks() const;
    
    // 分析敌人预设中数量最多的部族
    std::string findMostCommonCategory(const std::vector<std::string>& enemyCardIds) const;
    
    // 随机选择一个印记
    std::string selectRandomYi() const;
    

    std::string enemyYi_;      // 敌人的意（印记）
    std::string enemyJing_;    // 敌人的境（部族）
    std::string enemyEngraveInfo_; // 敌人意境信息描述
};
