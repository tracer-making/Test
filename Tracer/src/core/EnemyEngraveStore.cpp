#include "EnemyEngraveStore.h"
#include "Cards.h"
#include <iostream>

void EnemyEngraveStore::generateEnemyEngrave(const std::vector<std::string>& enemyCardIds) {
    clear();
    
    std::cout << "[ENEMY ENGRAVE] 开始生成敌人意境，敌人卡牌数量: " << enemyCardIds.size() << std::endl;
    
    // 1. 分析敌人预设中数量最多的部族
    enemyJing_ = findMostCommonCategory(enemyCardIds);
    std::cout << "[ENEMY ENGRAVE] 选择的部族: " << enemyJing_ << std::endl;
    
    // 2. 随机选择一个有意义的印记
    enemyYi_ = selectRandomYi();
    std::cout << "[ENEMY ENGRAVE] 选择的印记: " << enemyYi_ << std::endl;
    
    // 3. 设置意境信息
    if (!enemyJing_.empty() && !enemyYi_.empty()) {
        enemyEngraveInfo_ = enemyJing_ + " + " + enemyYi_;
        std::cout << "[ENEMY ENGRAVE] 敌人意境加成: " << enemyEngraveInfo_ << std::endl;
    } else {
        std::cout << "[ENEMY ENGRAVE] 警告：部族或印记为空！部族=" << enemyJing_ << ", 印记=" << enemyYi_ << std::endl;
    }
}

void EnemyEngraveStore::applyToEnemyCard(Card& card) const {
    // 直接为敌人卡牌添加印记，不通过意境系统
    if (!enemyYi_.empty()) {
        if (std::find(card.marks.begin(), card.marks.end(), enemyYi_) == card.marks.end()) {
            card.marks.push_back(enemyYi_);
            std::cout << "[ENEMY ENGRAVE] 为敌人卡牌 " << card.name << " 添加印记: " << enemyYi_ << std::endl;
        } else {
            std::cout << "[ENEMY ENGRAVE] 敌人卡牌 " << card.name << " 已有印记: " << enemyYi_ << std::endl;
        }
    } else {
        std::cout << "[ENEMY ENGRAVE] 警告：enemyYi_ 为空，无法添加印记！" << std::endl;
    }
}

std::vector<std::string> EnemyEngraveStore::getMeaningfulMarks() const {
    // 返回对敌人有意义的印记列表，排除道具商、不死印记等对敌人无收益的印记
    return {
        u8"空袭", u8"水袭", u8"护主", u8"双重攻击", u8"坚硬之躯", u8"冲刺能手",
        u8"高跳", u8"横冲直撞", u8"蛮力冲撞", u8"断尾求生", u8"死神之触",
        u8"反伤", u8"守护者", u8"领袖力量", u8"消耗骨头", u8"掘墓人",
        u8"一回合成长", u8"形态转换", u8"生生不息", u8"优质祭品",
        u8"献祭之血", u8"双向攻击", u8"三向攻击", u8"全向打击",
        u8"内心之蜂", u8"蚂蚁", u8"食尸鬼", u8"一口之量",
        u8"嗜血狂热", u8"拾荒者", u8"令人生厌", u8"臭臭",
        u8"铃铛距离", u8"镜像", u8"手牌数", u8"丰产之巢",
        u8"骨王", u8"磐石之身", u8"磐石", u8"铁兽夹"
    };
}

std::string EnemyEngraveStore::findMostCommonCategory(const std::vector<std::string>& enemyCardIds) const {
    std::map<std::string, int> categoryCount;
    
    // 统计每个部族的数量
    for (const auto& cardId : enemyCardIds) {
        if (cardId.empty()) continue;
        
        Card card = CardDB::instance().make(cardId);
        if (!card.id.empty() && !card.category.empty()) {
            categoryCount[card.category]++;
        }
    }
    
    // 找到数量最多的部族
    std::string mostCommonCategory;
    int maxCount = 0;
    
    for (const auto& pair : categoryCount) {
        if (pair.second > maxCount) {
            maxCount = pair.second;
            mostCommonCategory = pair.first;
        }
    }
    
    std::cout << "[ENEMY ENGRAVE] 敌人部族统计: ";
    for (const auto& pair : categoryCount) {
        std::cout << pair.first << "(" << pair.second << ") ";
    }
    std::cout << "-> 选择: " << mostCommonCategory << std::endl;
    
    return mostCommonCategory;
}

std::string EnemyEngraveStore::selectRandomYi() const {
    auto meaningfulMarks = getMeaningfulMarks();
    if (meaningfulMarks.empty()) return "";
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, meaningfulMarks.size() - 1);
    
    return meaningfulMarks[dis(gen)];
}

