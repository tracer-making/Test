#include "Cards.h"

CardDB& CardDB::instance() {
	static CardDB db; return db;
}

void CardDB::registerCard(const Card& proto) {
	if (proto.id.empty()) return;
	idToProto_[proto.id] = proto;
}

bool CardDB::contains(const std::string& id) const {
	return idToProto_.find(id) != idToProto_.end();
}

const Card* CardDB::find(const std::string& id) const {
	auto it = idToProto_.find(id);
	if (it == idToProto_.end()) return nullptr;
	return &it->second;
}

Card CardDB::make(const std::string& id) const {
	auto it = idToProto_.find(id);
	if (it == idToProto_.end()) return Card{};
	
	Card card = it->second;
	
	// 生成简单的数字实例ID
	static int cardCounter = 0;
	card.instanceId = std::to_string(cardCounter++);
	
	return card;
}

std::vector<std::string> CardDB::allIds() const {
	std::vector<std::string> ids; ids.reserve(idToProto_.size());
	for (const auto& kv : idToProto_) ids.push_back(kv.first);
	return ids;
}

void CardDB::loadBuiltinCards() {
	if (!idToProto_.empty()) return; // 只加载一次
	Card c;
	
	// 羽部
	c = Card{"qingyu_cuishi", u8"青羽翠使", 1, u8"羽部", 1, 1, "face_qingyu", {u8"空袭", u8"水袭"}}; registerCard(c);
	c = Card{"xuanwu_zhi", u8"玄乌之卵", 1, u8"羽部", 0, 2, "face_xuanwu_egg", {u8"一回合成长"}}; registerCard(c);
	c = Card{"xuanwu", u8"玄乌", 2, u8"羽部", 2, 3, "face_xuanwu", {u8"空袭"}}; registerCard(c);
	c = Card{"binque", u8"宾雀", 1, u8"羽部", 1, 2, "face_binque", {u8"空袭"}}; registerCard(c);
	c = Card{"lingque", u8"灵鹊", 2, u8"羽部", 1, 1, "face_lingque", {u8"空袭", u8"检索"}}; registerCard(c);
	c = Card{"chiling_jiu", u8"赤翎鹫", 8, u8"羽部", 3, 3, "face_chiling", {u8"空袭", u8"消耗骨头"}}; registerCard(c);
	c = Card{"bangu_peng", u8"半骨鹏", 3, u8"羽部", 0, 4, "face_bangu", {u8"空袭", u8"半根骨头"}}; registerCard(c);
	c = Card{"sangjiu", u8"桑鸠", 1, u8"羽部", 1, 1, "face_sangjiu", {u8"空袭", u8"滋生寄生虫"}}; registerCard(c);
	
	// 犬部
	c = Card{"canglang_youhun", u8"苍狼幼魂", 1, u8"犬部", 1, 1, "face_canglang_young", {u8"一回合成长"}}; registerCard(c);
	c = Card{"shuomuo_canglang", u8"朔漠苍狼", 2, u8"犬部", 3, 2, "face_shuomuo", {}}; registerCard(c);
	c = Card{"xuezong_xiquan", u8"血踪细犬", 2, u8"犬部", 2, 3, "face_xuezong", {u8"护主"}}; registerCard(c);
	c = Card{"yegao_kangou", u8"野皋烈狗", 4, u8"犬部", 2, 1, "face_yegao", {u8"消耗骨头"}}; registerCard(c);
	c = Card{"langrong_qiushou", u8"狼戎酋首", 4, u8"犬部", 1, 2, "face_langrong", {u8"领袖力量", u8"消耗骨头"}}; registerCard(c);
	c = Card{"xueyuan_langpei", u8"雪原狼胚", 2, u8"犬部", 1, 1, "face_xueyuan", {u8"掘墓人", u8"一回合成长"}}; registerCard(c);
	c = Card{"shuangya_zhanlang", u8"霜牙战狼", 3, u8"犬部", 2, 5, "face_shuangya", {u8"双重攻击"}}; registerCard(c);
	
	// 鹿部
	c = Card{"yunqu_youmi", u8"云渠幼麋", 1, u8"鹿部", 1, 1, "face_yunqu_young", {u8"冲刺能手", u8"一回合成长"}}; registerCard(c);
	c = Card{"yunqu_jumi", u8"云渠巨麋", 2, u8"鹿部", 2, 4, "face_yunqu_giant", {u8"冲刺能手"}}; registerCard(c);
	c = Card{"dishisanzi", u8"第十三子", 1, u8"鹿部", 0, 1, "face_dishisan", {u8"生生不息", u8"形态转换"}, 2}; registerCard(c);
	c = Card{"dishisanzi_juexing", u8"第十三子", 1, u8"鹿部", 2, 1, "face_dishisan", {u8"形态转换",u8"空袭",u8"生生不息"}, 0}; registerCard(c);
	c = Card{"shuangti_heying", u8"霜蹄鹤影", 4, u8"鹿部", 1, 2, "face_shuangti", {u8"冲刺能手", u8"死神之触",u8"消耗骨头"}, 2}; registerCard(c);
	c = Card{"xuanmu", u8"玄牡", 1, u8"鹿部", 0, 1, "face_xuanmu", {u8"优质祭品"}}; registerCard(c);
	c = Card{"qijiao_shuangqi", u8"岐角双歧鹿", 2, u8"鹿部", 1, 3, "face_qijiao", {u8"冲刺能手", u8"双向攻击"}}; registerCard(c);
	c = Card{"qianfeng_tuolu", u8"千峰驼鹿", 3, u8"鹿部", 3, 7, "face_qianfeng", {u8"蛮力冲撞"}}; registerCard(c);
	c = Card{"danxia_ruilu", u8"丹霞瑞鹿", 2, u8"鹿部", 0, 2, "face_danxia", {u8"献祭之血", u8"冲刺能手"}}; registerCard(c);
	c = Card{"jingguan_yeniu", u8"荆关野牛", 2, u8"鹿部", 3, 2, "face_jingguan", {u8"横冲直撞"}}; registerCard(c);
	
	// 介部
	c = Card{"juance_mingling", u8"卷册螟蛉", 1, u8"介部", 0, 1, "face_juance", {}}; registerCard(c);
	c = Card{"daobi_li", u8"刀笔吏", 1, u8"介部", 1, 1, "face_daobi", {u8"三向攻击"}, 2}; registerCard(c);
	c = Card{"shuchong_yizhuan", u8"书虫异篆", 1, u8"介部", 0, 3, "face_shuchong", {u8"一回合成长"}, 2}; registerCard(c);
	c = Card{"shuchong_mojian", u8"书虫墨茧", 0, u8"介部", 0, 3, "face_shuchong", {u8"一回合成长"}, 0}; registerCard(c);
	c = Card{"yuren_moke", u8"羽人墨客", 0, u8"介部", 7, 3, "face_shuchong", {u8"空袭"}, 0}; registerCard(c);
	c = Card{"yunqian_fengchao", u8"芸签蜂巢", 1, u8"介部", 0, 2, "face_yunqian", {u8"内心之蜂"}}; registerCard(c);
	c = Card{"yunchuang_fengshi", u8"芸窗蜂使", 0, u8"介部", 1, 1, "face_yunchuang", {u8"空袭"}, 0}; registerCard(c);
	c = Card{"shuangdao_moke", u8"双刀墨客", 1, u8"介部", 1, 1, "face_shuangdao", {u8"双向攻击"}}; registerCard(c);
	c = Card{"diangao_yihou", u8"典诰蚁后", 2, u8"介部", 0, 3, "face_diangao", {u8"蚂蚁",u8"蚁后"}}; registerCard(c);
	c = Card{"chaojing_gongyi", u8"抄经工蚁", 1, u8"介部", 0, 2, "face_chaojing", {u8"蚂蚁"}}; registerCard(c);
	c = Card{"yifei_yi", u8"驿飞蚁", 1, u8"介部", 0, 1, "face_yifei", {u8"空袭", u8"蚂蚁"}}; registerCard(c);
	c = Card{"duyu_buhua", u8"蠹鱼不化", 4, u8"介部", 1, 1, "face_duyu", {u8"不死印记", u8"消耗骨头"}}; registerCard(c);
	c = Card{"dushi_chong", u8"蠹尸虫", 5, u8"介部", 1, 2, "face_dushi", {u8"食尸鬼", u8"消耗骨头"}}; registerCard(c);
	c = Card{"qiushi", u8"裘虱", 4, u8"介部", 1, 1, "face_qiushi", {u8"双重攻击", u8"消耗骨头"}, 2}; registerCard(c);
	c = Card{"mianjian_chong", u8"面目简", 2, u8"介部", 0, 2, "face_mianjian", {u8"一口之量", u8"消耗骨头"}}; registerCard(c);
	
	// 鳞部
	c = Card{"hebo_tuojia", u8"河伯鼍甲", 2, u8"鳞部", 1, 6, "face_hebo", {}}; registerCard(c);
	c = Card{"shougong", u8"守宫", 0, u8"鳞部", 1, 1, "face_shougong", {}, 2}; registerCard(c);
	c = Card{"tengshe_zihuan", u8"衔尾环蛇", 2, u8"鳞部", 1, 1, "face_tengshe", {u8"不死印记"}, 2}; registerCard(c);
	c = Card{"bichan", u8"碧蟾", 1, u8"鳞部", 1, 2, "face_bichan", {u8"高跳"}}; registerCard(c);
	c = Card{"shanlongzi", u8"山龙子", 1, u8"鳞部", 1, 2, "face_shanlong", {u8"断尾求生"}}; registerCard(c);
	c = Card{"bashe", u8"巴蛇", 2, u8"鳞部", 1, 1, "face_bashe", {u8"死神之触"}}; registerCard(c);
	c = Card{"mingwei_fengshe", u8"鸣尾风蛇", 3, u8"鳞部", 3, 1, "face_mingwei", {u8"消耗骨头"}}; registerCard(c);
	c = Card{"xuanbei_dou", u8"玄贝蚪", 0, u8"鳞部", 0, 1, "face_xuanbei", {u8"水袭", u8"一回合成长"}}; registerCard(c);
	c = Card{"xuanwu", u8"玄武", 2, u8"鳞部", 2, 2, "face_xuanwu_young", {u8"坚硬之躯"}}; registerCard(c);
	c = Card{"tail_segment", u8"断尾", 0, u8"鳞部", 0, 2, "face_tail", {}, 0}; registerCard(c);
	
	// 其他
	// 破碎的卵（用于滋生寄生虫默认产物）
	c = Card{"posui_deluan", u8"破碎的卵", 0, u8"其他", 0, 1, "face_posui", {}, 0, false}; registerCard(c);
	c = Card{"maoxiu_wo", u8"卯宿窝", 1, u8"其他", 0, 2, "face_maoxiu", {u8"兔窝"}}; registerCard(c);
	c = Card{"baimao_zi", u8"白毫仔", 0, u8"其他", 0, 1, "face_baimao", {}, 0}; registerCard(c);
	c = Card{"shuigong_tuoshi", u8"水工柁师", 2, u8"其他", 1, 3, "face_shuigong", {u8"筑坝师"}}; registerCard(c);
	c = Card{"diba", u8"堤坝", 0, u8"其他", 0, 2, "face_diba", {}, 0, false}; registerCard(c);
	c = Card{"wengjian_choucheng", u8"瓮间臭丞", 2, u8"其他", 1, 2, "face_wengjian", {u8"臭臭", u8"消耗骨头"}}; registerCard(c);
	c = Card{"chuanfen_yinshi", u8"穿坟隐士", 1, u8"其他", 0, 6, "face_chuanfen", {u8"高跳", u8"守护者"}, 2}; registerCard(c);
	c = Card{"baina_ou", u8"百衲偶", 2, u8"其他", 3, 3, "face_baina", {u8"全物种", u8"蚂蚁类"}, 2}; registerCard(c);
	c = Card{"shulin_shucheng", u8"书林署丞", 2, u8"其他", 2, 2, "face_shulin", {u8"道具商"},2}; registerCard(c);
	c = Card{"maomin", u8"毛民", 4, u8"其他", 7, 7, "face_maomin", {}, 2}; registerCard(c);
	c = Card{"taiyi_hundun", u8"太一混沌", 2, u8"其他", 1, 2, "face_taiyi", {u8"随机",u8"消耗骨头"}, 2}; registerCard(c);
	
	c = Card{"xianchan_nu", u8"衔蝉奴", 1, u8"其他", 0, 1, "face_xianchan", {u8"生生不息"}}; registerCard(c);
	c = Card{"jiance_jishu", u8"简册计数触", 1, u8"其他", 0, 1, "face_jiance", {u8"手牌数"}}; registerCard(c);
	c = Card{"zhaogu_jingxu", u8"照骨镜须", 1, u8"其他", 0, 3, "face_zhaogu", {u8"镜像"}}; registerCard(c);
	c = Card{"duoling_suodi", u8"铎铃缩地须", 2, u8"其他", 0, 3, "face_duoling", {u8"铃铛距离"}}; registerCard(c);
	c = Card{"chuanfen_yanzi", u8"穿坟鼹子", 1, u8"其他", 0, 4, "face_chuanfen_yan", {u8"守护者"}}; registerCard(c);
	c = Card{"weijia_tong", u8"猬甲童", 1, u8"其他", 1, 2, "face_weijia", {u8"反伤"}}; registerCard(c);
	c = Card{"hegong_tuozi", u8"河工柁子", 1, u8"其他", 1, 1, "face_hegong", {u8"水袭"}}; registerCard(c);
	c = Card{"huangyou_chouwei", u8"黄鼬臭尉", 1, u8"其他", 0, 3, "face_huangyou", {u8"臭臭"}}; registerCard(c);
	c = Card{"xuewei_yousheng", u8"雪尾鼬生", 1, u8"其他", 1, 2, "face_xuewei", {}}; registerCard(c);
	c = Card{"cangdun_shuoshu", u8"仓囤硕鼠", 2, u8"其他", 2, 2, "face_cangdun", {u8"丰产之巢"}}; registerCard(c);
	c = Card{"dulou_shuwang", u8"髑髅鼠王", 2, u8"其他", 2, 1, "face_dulou", {u8"骨王"}}; registerCard(c);
	c = Card{"jiaolong", u8"鲛龙", 3, u8"其他", 4, 2, "face_jiaolong", {u8"水袭"}}; registerCard(c);
	c = Card{"jiufang_xiongjun", u8"九方熊君", 3, u8"其他", 4, 6, "face_jiufang", {}}; registerCard(c);
	c = Card{"jianjia_yu", u8"剑甲鱼", 2, u8"其他", 1, 1, "face_jianjia", {u8"消耗骨头"}}; registerCard(c);

	c = Card{"yefei_fuyi", u8"夜飞伏翼", 4, u8"其他", 2, 1, "face_yefei", {u8"空袭", u8"消耗骨头"}}; registerCard(c);
	c = Card{"dulou_yan", u8"髑髅烟", 0, u8"其他", 0, 1, "face_dulou_yan", {u8"骨王"}, 0}; registerCard(c);
	c = Card{"xiqushen_jiao", u8"犀渠神角", 2, u8"其他", 1, 5, "face_xiqushen", {u8"嗜血狂热"}, 2}; registerCard(c);
	c = Card{"huansha_xisheng", u8"浣沙溪生", 1, u8"其他", 1, 1, "face_huansha", {u8"拾荒者"}}; registerCard(c);
	c = Card{"xuanhuan", u8"玄獾", 5, u8"其他", 1, 3, "face_xuanhuan", {u8"嗜血狂热", u8"消耗骨头"}}; registerCard(c);

	// 死亡卡
	c = Card{"louis", u8"Louis", 1, u8"死亡", 1, 1, "face_louis", {u8"冲刺能手", u8"水袭"}, 0}; registerCard(c);
	c = Card{"jonah", u8"Jonah", 3, u8"死亡", 2, 5, "face_jonah", {u8"内心之蜂", u8"反伤"}, 0}; registerCard(c);
	c = Card{"kevin", u8"Kevin", 2, u8"死亡", 2, 3, "face_kevin", {u8"横冲直撞", u8"断尾求生"}, 0}; registerCard(c);
	c = Card{"sean", u8"Sean", 2, u8"死亡", 1, 6, "face_sean", {u8"高跳", u8"死神之触"}, 0}; registerCard(c);
	c = Card{"tamara", u8"Tamara", 2, u8"死亡", 2, 3, "face_tamara", {u8"蛮力冲撞", u8"断尾求生"}, 0}; registerCard(c);
	c = Card{"daniel", u8"Daniel", 2, u8"死亡", 2, 2, "face_daniel", {u8"双重攻击"}, 0}; registerCard(c);
	c = Card{"cody", u8"Cody", 2, u8"死亡", 3, 1, "face_cody", {u8"守护者", u8"断尾求生"}, 0}; registerCard(c);
	c = Card{"david", u8"David", 2, u8"死亡", 2, 4, "face_david", {u8"蛮力冲撞", u8"反伤"}, 0}; registerCard(c);
	c = Card{"tahnee", u8"Tahnee", 2, u8"死亡", 1, 3, "face_tahnee", {u8"三向攻击"}, 0}; registerCard(c);
	c = Card{"berke", u8"Berke", 2, u8"死亡", 2, 1, "face_berke", {u8"守护者"}, 0}; registerCard(c);
	c = Card{"kaycee", u8"Kaycee", 1, u8"死亡", 1, 2, "face_kaycee", {u8"双向攻击", u8"反伤"}, 0}; registerCard(c);
	c = Card{"kaminski", u8"Kaminski", 1, u8"死亡", 0, 1, "face_kaminski", {u8"守护者", u8"反伤", u8"消耗骨头"}, 0}; registerCard(c);
	c = Card{"reginald", u8"Reginald", 3, u8"死亡", 1, 3, "face_reginald", {u8"死神之触", u8"消耗骨头"}, 0}; registerCard(c);
	c = Card{"Oct19", u8"Oct19", 1, u8"死亡", 3, 2, "face_Oct19", {u8"死神之触"}, 0}; registerCard(c);
	c = Card{"Luke", u8"Luke", 0, u8"死亡", 4, 4, "face_Luke", {}, 0}; registerCard(c);

	//资源类
	c = Card{"moding", u8"墨锭", 0, u8"其他", 0, 1, "face_moding", {}, 0}; registerCard(c);
	c = Card{"langpi", u8"狼皮", 0, u8"其他", 0, 2, "face_langpi", {}, 0, false}; registerCard(c);
	c = Card{"jinang_mao", u8"金羊皮", 0, u8"其他", 0, 3, "face_jinang", {}, 0, false}; registerCard(c);
	c = Card{"tuopi_mao", u8"兔皮", 0, u8"其他", 0, 1, "face_tuopi", {}, 0, false}; registerCard(c);

	//特殊牌
	c = Card{"bingfeng_jianjia", u8"冰封剑甲", 0, u8"其他", 0, 5, "face_bingfeng_jianjia", {},0, false}; registerCard(c);
	c = Card{"panshi", u8"磐石", 0, u8"其他", 0, 5, "face_panshi", {u8"磐石之身"}, 0, false}; registerCard(c);
	c = Card{"jiaoyu", u8"蛟鱼", 0, u8"其他", 0, 1, "face_jiaoyu", {}, 0, false}; registerCard(c);
	c = Card{"jinkuai", u8"金块", 0, u8"其他", 0, 2, "face_jinkuai", {}, 0, false}; registerCard(c);
	c = Card{"yunshan", u8"云杉", 0, u8"其他", 0, 3, "face_yunshan", {u8"高跳"}, 0, false}; registerCard(c);
	c = Card{"xueshan", u8"雪杉", 0, u8"其他", 0, 4, "face_xueshan", {u8"高跳"}, 0, false}; registerCard(c);
	c = Card{"moying", u8"墨影", 0, u8"鹿部", 0, 1, "face_moying", {}, 0}; registerCard(c);
	c = Card{"luma", u8"骡子", 0, u8"鹿部", 0, 5, "face_luma", {u8"冲刺能手"}, 0}; registerCard(c);
	c = Card{"hungry", u8"饥饿", 0, u8"其他", 1, 1, "face_hungry", {u8"令人生厌"}, 0, false}; registerCard(c);
	c = Card{"muzhuang", u8"木桩", 0, u8"其他", 0, 3, "face_muzhuang", {}, 0, false}; registerCard(c);
	c = Card{"qi_qingwa", u8"奇怪的青蛙", 0, u8"其他", 1, 2, "face_qi_qingwa", {u8"高跳"}, 0, false}; registerCard(c);
	c = Card{"yueqiu", u8"月球", 0, u8"其他", 1, 40, "face_yueqiu", {u8"高跳", u8"全向打击", u8"磐石"}, 0, false}; registerCard(c);
	c = Card{"tieshou_jia", u8"铁兽夹", 0, u8"其他", 0, 1, "face_tieshou", {u8"铁兽夹",u8"高跳"}, 0, false}; registerCard(c);

}
