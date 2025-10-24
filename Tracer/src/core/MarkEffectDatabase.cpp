#include "MarkEffectDatabase.h"

MarkEffectDatabase::MarkEffectDatabase() {
    // 攻击类印记
    markDescriptions_[u8"双重攻击"] = u8"带有该印记的卡牌会在攻击时额外攻击对面位置一次";
    markDescriptions_[u8"双向攻击"] = u8"带有该印记的卡牌会攻击对位的左右两侧位置各一次";
    markDescriptions_[u8"三向攻击"] = u8"带有该印记的卡牌会攻击对位的左右两侧位置和对位各一次";
    markDescriptions_[u8"全向打击"] = u8"带有该印记的卡牌会攻击对面每个造物";
    markDescriptions_[u8"空袭"] = u8"带有该印记的卡牌可直接攻击持牌人，即使其对位有其他造物";
    markDescriptions_[u8"水袭"] = u8"带有该印记的卡牌会在其对手回合中潜入水下，它潜水时，其持牌人会遭到对方造物直接攻击";
    markDescriptions_[u8"高跳"] = u8"带有该印记的卡牌会拦下带有空袭印记的对方造物";
    markDescriptions_[u8"冲刺能手"] = u8"持牌人回合结束时，带有该印记的卡牌会向默认方向移动";
    markDescriptions_[u8"蛮力冲撞"] = u8"持牌人回合结束时，带有该印记的卡牌将向默认方向移动。沿途的造物均会被推向同一方向";
    markDescriptions_[u8"横冲直撞"] = u8"持牌人回合结束时，带有该印记的卡牌会向默认方向移动。沿途的造物会被丢到它身后";
    markDescriptions_[u8"嗜血狂热"] = u8"当带有该印记的卡牌杀死一个造物时，它的攻击力增加1点";
    
    // 防御类印记
    markDescriptions_[u8"守护者"] = u8"当一个空位可能受到攻击时带有该印记的卡牌会移动至该位置承受攻击";
    markDescriptions_[u8"护主"] = u8"如对手的造物对面位置是空的，则带有该印记的卡牌会进入那个位置";
    markDescriptions_[u8"领袖力量"] = u8"带有该印记的卡牌相邻的造物增加1点攻击力";
    markDescriptions_[u8"掘墓人"] = u8"在持牌人回合结束时，带有该印记的卡牌将产生1根魂骨";
    markDescriptions_[u8"生生不息"] = u8"带有该印记的卡牌被献祭后不会阵亡";
    markDescriptions_[u8"不死印记"] = u8"当带有该印记的卡牌死亡时，你的手牌里会出现一张同样的牌";
    markDescriptions_[u8"坚硬之躯"] = u8"带有该印记的卡牌第一次受到攻击时免疫";
    markDescriptions_[u8"磐石之身"] = u8"带有该印记的卡牌免疫死神之触和臭臭的印记效果";
    markDescriptions_[u8"反伤"] = u8"带有该印记的牌被攻击时，攻击者也会受到1点伤害";
    
    // 特殊效果类印记
    markDescriptions_[u8"一回合成长"] = u8"带有该印记的卡牌在牌桌上1个回合后将成长为更强大的形态";
    markDescriptions_[u8"内心之蜂"] = u8"带有该印记的卡牌收到攻击时，你的手牌中将出现一张蜜蜂（1/1 空袭）";
    markDescriptions_[u8"滋生寄生虫"] = u8"当打出带有该印记的牌时，对面位置会出现一颗蛋";
    markDescriptions_[u8"断尾求生"] = u8"当带有该印记的卡牌可能受到攻击时，会在原地留下一截尾巴，带有该印记的卡牌则会向右移动";
    markDescriptions_[u8"死神之触"] = u8"如带有该印记的卡牌使另一只造物受伤，那只造物将当场死亡";
    markDescriptions_[u8"令人生厌"] = u8"带有该印记的卡牌对位的造物的攻击力将会增加1点";
    markDescriptions_[u8"臭臭"] = u8"在带有该印记的造物对位的造物降低1点攻击力";
    markDescriptions_[u8"蚂蚁"] = u8"攻击力等于场上持牌人侧所有蚂蚁数量";
    markDescriptions_[u8"蚁后"] = u8"使用带有该印记的卡牌时，你的手牌中将会出现一张抄经工蚁（0/1 蚂蚁）";
    markDescriptions_[u8"一口之量"] = u8"当带有该印记的卡牌被献祭时，它的状态值会被添加到献祭来的新卡上";
    markDescriptions_[u8"兔窝"] = u8"使用带有该印记的卡牌时，你的手牌里会出现一张白毫仔（0/1）";
    markDescriptions_[u8"筑坝师"] = u8"使用带有该印记的卡牌时，附近空位均会出现堤坝卡牌（0/2）";
    markDescriptions_[u8"检索"] = u8"使用带有该印记的卡牌时，你可以在牌组中找出任意一张卡牌并加入手牌";
    markDescriptions_[u8"道具商"] = u8"使用带有该印记的卡牌时，只要你的道具栏未满即可随机获得一个道具";
    markDescriptions_[u8"丰产之巢"] = u8"使用带有该印记的卡牌时，你的手牌里会出现一张同样的牌";
    markDescriptions_[u8"食尸鬼"] = u8"如你拥有的造物在战斗中死亡，你手中的带有该印记的牌会自动补位";
    markDescriptions_[u8"冰封禁锢"] = u8"带有该印记的卡牌死亡时，被冰封在里面的造物会取代它的位置";
    markDescriptions_[u8"铁兽夹"] = u8"带有该印记的卡牌死亡时，它对面的造物也会同时死亡。同时你的手牌中将会出现一张狼皮）";
    markDescriptions_[u8"拾荒者"] = u8"当场上出现带有该印记的卡牌时，它对面的造物死亡时也会产生魂骨";
    
    // 消耗类印记
    markDescriptions_[u8"消耗骨头"] = u8"使用此卡需要消耗魂骨";
    markDescriptions_[u8"优质祭品"] = u8"带有该印记的卡牌在献祭时可算作3点墨量而非1点";
    markDescriptions_[u8"献祭之血"] = u8"带有该印记的卡牌的攻击力等于本回合献祭次数";
    markDescriptions_[u8"半根骨头"] = u8"带有该印记的卡牌的攻击力会加上当前魂骨数的一半";
    markDescriptions_[u8"手牌数"] = u8"带有该印记的卡牌的攻击力等于持牌人当前手牌数量";
    markDescriptions_[u8"镜像"] = u8"带有该印记的卡牌的攻击力等于对位卡牌的攻击力";
    markDescriptions_[u8"铃铛距离"] = u8"带有该印记的卡牌的攻击力会根据位置从左到右分别为4、3、2、1";
    markDescriptions_[u8"骨王"] = u8"带有该印记的卡牌死亡时，原本获得一根魂骨将会变成四根";
    
    // 随机印记
    markDescriptions_[u8"随机"] = u8"抽到带有该印记的卡牌时，此印记将会随机替换成另一印记";
}

std::string MarkEffectDatabase::getMarkDescription(const std::string& mark) const {
    auto it = markDescriptions_.find(mark);
    if (it != markDescriptions_.end()) {
        return it->second;
    }
    return u8"未知印记效果";
}
