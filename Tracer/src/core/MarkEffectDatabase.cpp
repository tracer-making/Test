#include "MarkEffectDatabase.h"

void MarkEffectDatabase::initialize() {
    // 攻击类印记
    markDescriptions_[u8"双重攻击"] = u8"对位攻击两次，造成双倍伤害";
    markDescriptions_[u8"双向攻击"] = u8"攻击左右斜对位，避开对位";
    markDescriptions_[u8"三向攻击"] = u8"攻击对位+左右斜对位";
    markDescriptions_[u8"全向打击"] = u8"攻击所有敌方单位";
    markDescriptions_[u8"空袭"] = u8"攻击时无视对位，直接攻击后排";
    markDescriptions_[u8"水袭"] = u8"攻击时造成额外伤害";
    markDescriptions_[u8"高跳"] = u8"攻击时跳过前排直接攻击后排";
    markDescriptions_[u8"冲刺能手"] = u8"攻击时优先攻击前排";
    markDescriptions_[u8"蛮力冲撞"] = u8"攻击时造成额外伤害并击退";
    
    // 防御类印记
    markDescriptions_[u8"护主"] = u8"保护相邻友军，承受其受到的伤害";
    markDescriptions_[u8"领袖力量"] = u8"提升相邻友军的攻击力";
    markDescriptions_[u8"掘墓人"] = u8"友军死亡时获得额外攻击力";
    markDescriptions_[u8"生生不息"] = u8"每回合开始时恢复生命值";
    markDescriptions_[u8"不死印记"] = u8"死亡时立即复活，但攻击力减半";
    markDescriptions_[u8"坚硬之躯"] = u8"减少受到的物理伤害";
    markDescriptions_[u8"磐石之身"] = u8"免疫所有负面效果";
    markDescriptions_[u8"反伤"] = u8"受到攻击时对攻击者造成反伤";
    
    // 特殊效果类印记
    markDescriptions_[u8"内心之蜂"] = u8"攻击时召唤蜜蜂协助战斗";
    markDescriptions_[u8"滋生寄生虫"] = u8"攻击时在敌方身上寄生";
    markDescriptions_[u8"断尾求生"] = u8"受到致命伤害时消耗印记保命";
    markDescriptions_[u8"死神之触"] = u8"攻击时有一定概率直接杀死目标";
    markDescriptions_[u8"令人生厌"] = u8"降低敌方攻击力";
    markDescriptions_[u8"臭臭"] = u8"降低敌方攻击力";
    markDescriptions_[u8"蚂蚁"] = u8"攻击力等于场上所有蚂蚁数量";
    markDescriptions_[u8"蚁后"] = u8"召唤蚂蚁协助战斗";
    markDescriptions_[u8"一口之量"] = u8"攻击时吞噬目标";
    markDescriptions_[u8"兔窝"] = u8"每回合召唤兔子";
    markDescriptions_[u8"筑坝师"] = u8"每回合建造防御工事";
    markDescriptions_[u8"检索"] = u8"从牌库中搜索特定卡牌";
    markDescriptions_[u8"道具商"] = u8"提供特殊道具效果";
    
    // 消耗类印记
    markDescriptions_[u8"消耗骨头"] = u8"使用此卡需要消耗魂骨";
    markDescriptions_[u8"优质祭品"] = u8"献祭时提供额外效果";
    markDescriptions_[u8"献祭之血"] = u8"攻击力等于本回合献祭次数";
    markDescriptions_[u8"半根骨头"] = u8"攻击力加上当前魂骨数的一半";
    markDescriptions_[u8"手牌数"] = u8"攻击力等于当前手牌数量";
    markDescriptions_[u8"镜像"] = u8"攻击力等于对位卡牌的攻击力";
    markDescriptions_[u8"铃铛距离"] = u8"根据位置从左到右攻击力为4、3、2、1";
    
    // 随机印记
    markDescriptions_[u8"随机"] = u8"战斗开始时随机获得一个印记";
}

std::string MarkEffectDatabase::getMarkDescription(const std::string& mark) const {
    auto it = markDescriptions_.find(mark);
    if (it != markDescriptions_.end()) {
        return it->second;
    }
    return u8"未知印记效果";
}
