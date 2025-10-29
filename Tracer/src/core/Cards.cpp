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
	c = Card{"qingyu_cuishi", u8"青羽翠使", 1, u8"羽部", 1, 1, u8"——《山海经·西山经》\"翠山，其鸟多青羽\"，青羽之鸟，其羽如翠，其声如铃，善空袭水袭", {u8"空袭", u8"水袭"}}; registerCard(c);
	c = Card{"xuanwu_zhi", u8"玄乌之卵", 1, u8"羽部", 0, 2, u8"——《诗经·商颂》\"天命玄鸟，降而生商\"，玄乌之卵，其色如墨，其内如星，孕育神鸟", {u8"一回合成长"}}; registerCard(c);
	c = Card{"xuanwu", u8"玄乌", 2, u8"羽部", 2, 3, u8"——《诗经·商颂》\"天命玄鸟，降而生商\"，玄乌成鸟，其羽如夜，其鸣如雷，善空袭", {u8"空袭"}}; registerCard(c);
	c = Card{"binque", u8"宾雀", 1, u8"羽部", 1, 2, u8"——《礼记·月令》郑玄注\"雀，宾雀也\"，宾雀之鸟，其形如雀，其声如宾，善空袭", {u8"空袭"}}; registerCard(c);
	c = Card{"lingque", u8"灵鹊", 2, u8"羽部", 1, 1, u8"——《周易·卦辞》\"鹊噪而行人至\"，灵鹊之鸟，其声如铃，其智如人，善空袭检索", {u8"空袭", u8"检索"}}; registerCard(c);
	c = Card{"chiling_jiu", u8"赤翎鹫", 8, u8"羽部", 3, 3, u8"——《太平御览》引《山海经》\"赤翎之鹫，其大如鹏\"，赤翎之鹫，其羽如血，其势如鹏，善空袭", {u8"空袭", u8"消耗骨头"}}; registerCard(c);
	c = Card{"bangu_peng", u8"半骨鹏", 3, u8"羽部", 0, 4, u8"——《庄子·逍遥游》\"鹏之背，不知其几千里也\"，半骨之鹏，其骨如石，其翼如云，善空袭", {u8"空袭", u8"半根骨头"}}; registerCard(c);
	c = Card{"sangjiu", u8"桑鸠", 1, u8"羽部", 1, 1, u8"——《诗经·小雅》\"桑扈（鸠）于飞\"，桑鸠之鸟，其羽如桑，其声如鸠，善空袭", {u8"空袭", u8"滋生寄生虫"}}; registerCard(c);
	
	// 犬部
	c = Card{"canglang_youhun", u8"苍狼幼魂", 1, u8"犬部", 1, 1, u8"——《汉书·匈奴传》\"匈奴以狼为祖\"，苍狼幼魂，其魂如狼，其势如风，一回合成长", {u8"一回合成长"}}; registerCard(c);
	c = Card{"shuomuo_canglang", u8"朔漠苍狼", 2, u8"犬部", 3, 2, u8"——《后汉书》\"朔漠，狼居胥山\"，朔漠苍狼，其毛如雪，其眼如星，威震朔漠", {}}; registerCard(c);
	c = Card{"xuezong_xiquan", u8"血踪细犬", 2, u8"犬部", 2, 3, u8"——《诗经·小雅》\"无使尨也吠\"，血踪细犬，其毛如血，其心如铁，善护主", {u8"护主"}}; registerCard(c);
	c = Card{"yegao_kangou", u8"野皋烈狗", 4, u8"犬部", 2, 1, u8"——《尔雅·释兽》\"犺，如犬，食虎豹\"，野皋烈狗，其牙如刀，其力如虎，善食骨", {u8"消耗骨头"}}; registerCard(c);
	c = Card{"langrong_qiushou", u8"狼戎酋首", 4, u8"犬部", 1, 2, u8"——《国语·周语》\"狼戎为酋\"，狼戎酋首，其威如王，其力如狼，善领袖", {u8"领袖力量", u8"消耗骨头"}}; registerCard(c);
	c = Card{"xueyuan_langpei", u8"雪原狼胚", 2, u8"犬部", 1, 1, u8"——《山海经·大荒北经》\"雪原之狼，善掘冰穴\"，雪原狼胚，其毛如雪，其爪如冰，善掘墓", {u8"掘墓人", u8"一回合成长"}}; registerCard(c);
	c = Card{"shuangya_zhanlang", u8"霜牙战狼", 3, u8"犬部", 2, 5, u8"——《淮南子》\"霜牙之兽，左右攫噬\"，霜牙战狼，其牙如霜，其力如雷，善双重攻击", {u8"双重攻击"}}; registerCard(c);
	
	// 鹿部
	c = Card{"yunqu_youmi", u8"云渠幼麋", 1, u8"鹿部", 1, 1, u8"——《上林赋》\"云渠灌乎苑中\"，云渠幼麋，其角如云，其蹄如风，善冲刺成长", {u8"冲刺能手", u8"一回合成长"}}; registerCard(c);
	c = Card{"yunqu_jumi", u8"云渠巨麋", 2, u8"鹿部", 2, 4, u8"——《上林赋》\"云渠灌乎苑中\"，云渠巨麋，其角如云，其体如山，善冲刺", {u8"冲刺能手"}}; registerCard(c);
	c = Card{"dishisanzi", u8"第十三子", 1, u8"鹿部", 0, 1, u8"——《周易·同人》\"同人于野，亨\"，第十三子，其形如鹿，其心如人，善生生不息", {u8"生生不息", u8"形态转换"}, 2}; registerCard(c);
	c = Card{"dishisanzi_juexing", u8"第十三子", 1, u8"鹿部", 2, 1, u8"——《周易·同人》\"同人于野，亨\"，第十三子觉醒，其力如雷，其智如神，善形态转换", {u8"形态转换",u8"空袭",u8"生生不息"}, 0}; registerCard(c);
	c = Card{"shuangti_heying", u8"霜蹄鹤影", 4, u8"鹿部", 1, 2, u8"——《南史》\"霜蹄蹶于长坂\"，霜蹄鹤影，其蹄如霜，其影如鹤，善冲刺死神", {u8"冲刺能手", u8"死神之触",u8"消耗骨头"}, 2}; registerCard(c);
	c = Card{"xuanmu", u8"玄牡", 1, u8"鹿部", 0, 1, u8"——《尚书·武成》\"用玄牡告于后土\"，玄牡之鹿，其色如墨，其角如月，善祭品", {u8"优质祭品"}}; registerCard(c);
	c = Card{"qijiao_shuangqi", u8"岐角双歧鹿", 2, u8"鹿部", 1, 3, u8"——《山海经·中山经》\"岐角之鹿，其枝如叉\"，岐角双歧鹿，其角如叉，其力如雷，善冲刺双向", {u8"冲刺能手", u8"双向攻击"}}; registerCard(c);
	c = Card{"qianfeng_tuolu", u8"千峰驼鹿", 3, u8"鹿部", 3, 7, u8"——《穆天子传》\"驼鹿出于千峰\"，千峰驼鹿，其体如山，其力如峰，善蛮力冲撞", {u8"蛮力冲撞"}}; registerCard(c);
	c = Card{"danxia_ruilu", u8"丹霞瑞鹿", 2, u8"鹿部", 0, 2, u8"——《抱朴子》\"丹霞覆体，其鹿自现\"，丹霞瑞鹿，其色如霞，其血如金，善献祭冲刺", {u8"献祭之血", u8"冲刺能手"}}; registerCard(c);
	c = Card{"jingguan_yeniu", u8"荆关野牛", 2, u8"鹿部", 3, 2, u8"——《左传》\"荆关，楚之要塞\"，荆关野牛，其角如刀，其力如山，善横冲直撞", {u8"横冲直撞"}}; registerCard(c);
	
	// 介部
	c = Card{"juance_mingling", u8"卷册螟蛉", 1, u8"介部", 0, 1, u8"——《诗经·小雅》\"螟蛉有子，蜾蠃负之\"，卷册螟蛉，其形如虫，其智如人，善读书", {}}; registerCard(c);
	c = Card{"daobi_li", u8"刀笔吏", 1, u8"介部", 1, 1, u8"——《史记·汲郑列传》\"刀笔之吏\"，刀笔之吏，其笔如刀，其字如剑，善三向攻击", {u8"三向攻击"}, 2}; registerCard(c);
	c = Card{"shuchong_yizhuan", u8"书虫异篆", 1, u8"介部", 0, 3, u8"——《说文》\"虫者，篆文之蚀\"，书虫异篆，其形如虫，其字如篆，善成长", {u8"一回合成长"}, 2}; registerCard(c);
	c = Card{"shuchong_mojian", u8"书虫墨茧", 0, u8"介部", 0, 3, u8"——《说文》\"虫者，篆文之蚀\"，书虫墨茧，其茧如墨，其内如虫，善成长", {u8"一回合成长"}, 0}; registerCard(c);
	c = Card{"yuren_moke", u8"羽人墨客", 0, u8"介部", 7, 3, u8"——《说文》\"虫者，篆文之蚀\"，羽人墨客，其羽如云，其墨如星，善空袭", {u8"空袭"}, 0}; registerCard(c);
	c = Card{"yunqian_fengchao", u8"芸签蜂巢", 1, u8"介部", 0, 2, u8"——《梦粱录》\"芸签插架，卷轴盈厨\"，芸签蜂巢，其签如芸，其巢如蜂，善内心之蜂", {u8"内心之蜂"}}; registerCard(c);
	c = Card{"yunchuang_fengshi", u8"芸窗蜂使", 0, u8"介部", 1, 1, u8"——《梦粱录》\"芸窗即书窗\"，芸窗蜂使，其窗如芸，其使如蜂，善空袭", {u8"空袭"}, 0}; registerCard(c);
	c = Card{"shuangdao_moke", u8"双刀墨客", 1, u8"介部", 1, 1, u8"——《世说新语》\"墨客挥毫\"，双刀墨客，其刀如双，其墨如血，善双向攻击", {u8"双向攻击"}}; registerCard(c);
	c = Card{"diangao_yihou", u8"典诰蚁后", 2, u8"介部", 0, 3, u8"——《周礼》\"典诰掌诏令\"，典诰蚁后，其令如典，其后如蚁，善蚂蚁蚁后", {u8"蚂蚁",u8"蚁后"}}; registerCard(c);
	c = Card{"chaojing_gongyi", u8"抄经工蚁", 1, u8"介部", 0, 2, u8"——《洛阳伽蓝记》\"抄经生数百\"，抄经工蚁，其工如蚁，其经如抄，善蚂蚁", {u8"蚂蚁"}}; registerCard(c);
	c = Card{"yifei_yi", u8"驿飞蚁", 1, u8"介部", 0, 1, u8"——《汉书》\"驿飞传檄\"，驿飞之蚁，其飞如驿，其传如檄，善空袭蚂蚁", {u8"空袭", u8"蚂蚁"}}; registerCard(c);
	c = Card{"duyu_buhua", u8"蠹鱼不化", 4, u8"介部", 1, 1, u8"——《搜神记》\"蠹鱼三食神仙字，则化身为仙，不死\"，蠹鱼不化，其鱼如蠹，其化如仙，善不死", {u8"不死印记", u8"消耗骨头"}}; registerCard(c);
	c = Card{"dushi_chong", u8"蠹尸虫", 5, u8"介部", 1, 2, u8"——《太平广记》\"蠹尸虫，食书亦食尸\"，蠹尸之虫，其食如蠹，其尸如鬼，善食尸鬼", {u8"食尸鬼", u8"消耗骨头"}}; registerCard(c);
	c = Card{"qiushi", u8"裘虱", 4, u8"介部", 1, 1, u8"——《史记·孟尝君传》\"裘敝金尽，虱处其中\"，裘虱之虫，其虱如裘，其处如金，善双重攻击", {u8"双重攻击", u8"消耗骨头"}, 2}; registerCard(c);
	c = Card{"mianjian_chong", u8"面目简", 2, u8"介部", 0, 2, u8"——《齐民要术》\"作麺简，以充军粮\"，面目之简，其面如简，其目如粮，善一口之量", {u8"一口之量", u8"消耗骨头"}}; registerCard(c);
	
	// 鳞部
	c = Card{"hebo_tuojia", u8"河伯鼍甲", 2, u8"鳞部", 1, 6, u8"——《楚辞·九歌》\"河伯出，鼍鸣吼\"，河伯鼍甲，其甲如河，其鸣如伯，威震江河", {}}; registerCard(c);
	c = Card{"shougong", u8"守宫", 0, u8"鳞部", 1, 1, u8"——《山海经》\"守宫，处壁，捕虫\"，守宫之兽，其处如壁，其捕如虫，善守宫", {}, 2}; registerCard(c);
	c = Card{"tengshe_zihuan", u8"衔尾环蛇", 2, u8"鳞部", 1, 1, u8"——《周易·系辞》\"螣蛇无足而飞\"，衔尾环蛇，其尾如环，其飞如蛇，善不死印记", {u8"不死印记"}, 2}; registerCard(c);
	c = Card{"bichan", u8"碧蟾", 1, u8"鳞部", 1, 2, u8"——《淮南子》\"蟾蜍辟兵，高鸣于月\"，碧蟾之兽，其色如碧，其鸣如月，善高跳", {u8"高跳"}}; registerCard(c);
	c = Card{"shanlongzi", u8"山龙子", 1, u8"鳞部", 1, 2, u8"——《本草纲目》\"石龙子，一名山龙子，自断其尾\"，山龙之子，其尾如龙，其断如生，善断尾求生", {u8"断尾求生"}}; registerCard(c);
	c = Card{"bashe", u8"巴蛇", 2, u8"鳞部", 1, 1, u8"——《山海经·海内南经》\"巴蛇食象，三岁而出其骨\"，巴蛇之兽，其食如象，其骨如岁，善死神之触", {u8"死神之触"}}; registerCard(c);
	c = Card{"mingwei_fengshe", u8"鸣尾风蛇", 3, u8"鳞部", 3, 1, u8"——《异物志》\"鸣尾蛇，声如挝鼓\"，鸣尾风蛇，其尾如鸣，其声如鼓，善消耗骨头", {u8"消耗骨头"}}; registerCard(c);
	c = Card{"xuanbei_dou", u8"玄贝蚪", 0, u8"鳞部", 0, 1, u8"——《周易·震》\"震为玄贝\"，玄贝之蚪，其贝如玄，其震如贝，善水袭成长", {u8"水袭", u8"一回合成长"}}; registerCard(c);
	c = Card{"xuanwu", u8"玄武", 2, u8"鳞部", 2, 2, u8"——《淮南子》\"玄武，龟蛇合体，司北方\"，玄武之神，其体如龟，其合如蛇，善坚硬之躯", {u8"坚硬之躯"}}; registerCard(c);
	c = Card{"tail_segment", u8"断尾", 0, u8"鳞部", 0, 2, u8"——《山海经》\"断尾求生，其尾自断\"，断尾之段，其尾如断，其生如求，善断尾求生", {}, 0}; registerCard(c);
	
	// 其他
	// 破碎的卵（用于滋生寄生虫默认产物）
	c = Card{"posui_deluan", u8"破碎的卵", 0, u8"其他", 0, 1, u8"破碎的卵，其内空空，却孕育着新的生命", {}, 0, false}; registerCard(c);
	c = Card{"maoxiu_wo", u8"卯宿窝", 1, u8"其他", 0, 2, u8"——《史记·天官书》\"卯曰兔宿\"，卯宿之窝，其宿如卯，其窝如兔，善兔窝", {u8"兔窝"}}; registerCard(c);
	c = Card{"baimao_zi", u8"白毫仔", 0, u8"其他", 0, 1, u8"白毫仔，其毛如雪，其形如兔", {}, 0}; registerCard(c);
	c = Card{"shuigong_tuoshi", u8"水工柁师", 2, u8"其他", 1, 3, u8"——《考工记》\"水工掌柁\"，水工柁师，其工如水，其柁如师，善筑坝师", {u8"筑坝师"}}; registerCard(c);
	c = Card{"diba", u8"堤坝", 0, u8"其他", 0, 2, u8"堤坝，其固如石，其势如山", {}, 0, false}; registerCard(c);
	c = Card{"wengjian_choucheng", u8"瓮间臭丞", 2, u8"其他", 1, 2, u8"——《周礼》\"酒正，辨五齐，置臭丞\"，瓮间臭丞，其间如瓮，其臭如丞，善臭臭", {u8"臭臭", u8"消耗骨头"}}; registerCard(c);
	c = Card{"chuanfen_yinshi", u8"穿坟隐士", 1, u8"其他", 0, 6, u8"——《晋书》\"穿坟夜读，人称隐士\"，穿坟隐士，其坟如穿，其隐如士，善高跳守护", {u8"高跳", u8"守护者"}, 2}; registerCard(c);
	c = Card{"baina_ou", u8"百衲偶", 2, u8"其他", 3, 3, u8"——《西湖游览志》\"百衲佛，碎布合成\"，百衲之偶，其衲如百，其合如佛，善全物种", {u8"全物种", u8"蚂蚁类"}, 2}; registerCard(c);
	c = Card{"shulin_shucheng", u8"书林署丞", 2, u8"其他", 2, 2, u8"——《南史》\"书林署，掌天下图籍\"，书林署丞，其林如书，其署如丞，善道具商", {u8"道具商"},2}; registerCard(c);
	c = Card{"maomin", u8"毛民", 4, u8"其他", 7, 7, u8"——《山海经·海外东经》\"毛民国，其民大毛\"，毛民之族，其民如毛，其国如大，威震四方", {}, 2}; registerCard(c);
	c = Card{"taiyi_hundun", u8"太一混沌", 2, u8"其他", 1, 2, u8"——《淮南子》\"太一出两仪，两仪出阴阳\"，太一混沌，其一出两，其仪出阴，善随机", {u8"随机",u8"消耗骨头"}, 2}; registerCard(c);
	
	c = Card{"xianchan_nu", u8"衔蝉奴", 1, u8"其他", 0, 1, u8"——《表异录》\"猫，一名衔蝉\"，衔蝉之奴，其蝉如衔，其奴如猫，善生生不息", {u8"生生不息"}}; registerCard(c);
	c = Card{"jiance_jishu", u8"简册计数触", 1, u8"其他", 0, 1, u8"——《汉书》\"简册数百，计数盈车\"，简册计数触，其册如简，其数如车，善手牌数", {u8"手牌数"}}; registerCard(c);
	c = Card{"zhaogu_jingxu", u8"照骨镜须", 1, u8"其他", 0, 3, u8"——《西京杂记》\"秦王照骨镜，洞见脏腑\"，照骨镜须，其镜如骨，其须如洞，善镜像", {u8"镜像"}}; registerCard(c);
	c = Card{"duoling_suodi", u8"铎铃缩地须", 2, u8"其他", 0, 3, u8"——《周礼》\"铎人掌铃，以缩地传令\"，铎铃缩地须，其铃如铎，其地如缩，善铃铛距离", {u8"铃铛距离"}}; registerCard(c);
	c = Card{"chuanfen_yanzi", u8"穿坟鼹子", 1, u8"其他", 0, 4, u8"——《搜神记》\"鼹为穿坟小兽\"，穿坟鼹子，其坟如穿，其鼹如子，善守护者", {u8"守护者"}}; registerCard(c);
	c = Card{"weijia_tong", u8"猬甲童", 1, u8"其他", 1, 2, u8"——《逸周书》\"猬之毛，童而利\"，猬甲之童，其甲如猬，其童如利，善反伤", {u8"反伤"}}; registerCard(c);
	c = Card{"hegong_tuozi", u8"河工柁子", 1, u8"其他", 1, 1, u8"——《考工记》\"水工掌柁\"，河工柁子，其工如河，其柁如子，善水袭", {u8"水袭"}}; registerCard(c);
	c = Card{"huangyou_chouwei", u8"黄鼬臭尉", 1, u8"其他", 0, 3, u8"——《本草拾遗》\"黄鼬，一名臭尉\"，黄鼬臭尉，其鼬如黄，其臭如尉，善臭臭", {u8"臭臭"}}; registerCard(c);
	c = Card{"xuewei_yousheng", u8"雪尾鼬生", 1, u8"其他", 1, 2, u8"——《异物志》\"雪尾鼬，白若霜雪\"", {}}; registerCard(c);
	c = Card{"cangdun_shuoshu", u8"仓囤硕鼠", 2, u8"其他", 2, 2, u8"——《诗经·魏风》\"硕鼠硕鼠，无食我黍\"，仓囤硕鼠，其鼠如硕，其囤如仓，善丰产之巢", {u8"丰产之巢"}}; registerCard(c);
	c = Card{"dulou_shuwang", u8"髑髅鼠王", 2, u8"其他", 2, 1, u8"——《晋书》\"鼠集髑髅，若戴王冠\"，髑髅鼠王，其髑如髅，其王如鼠，善骨王", {u8"骨王"}}; registerCard(c);
	c = Card{"jiaolong", u8"鲛龙", 3, u8"其他", 4, 2, u8"——《搜神记》\"鲛人水居，客游如人\"，鲛龙之神，其鲛如人，其龙如水，善水袭", {u8"水袭"}}; registerCard(c);
	c = Card{"jiufang_xiongjun", u8"九方熊君", 3, u8"其他", 4, 6, u8"——《穆天子传》\"九方之地，熊为山君\"，九方熊君，其方如九，其君如熊，威震九方", {}}; registerCard(c);
	c = Card{"jianjia_yu", u8"剑甲鱼", 2, u8"其他", 1, 1, u8"剑甲鱼，其甲如剑，其鳞如刃", {u8"消耗骨头"}}; registerCard(c);

	c = Card{"yefei_fuyi", u8"夜飞伏翼", 4, u8"其他", 2, 1, u8"——《尔雅》\"蝙蝠，一名伏翼\"，夜飞伏翼，其飞如夜，其翼如伏，善空袭", {u8"空袭", u8"消耗骨头"}}; registerCard(c);
	c = Card{"dulou_yan", u8"髑髅烟", 0, u8"其他", 0, 1, u8"——《酉阳杂俎》\"髑髅吐烟，百骨朝王\"，髑髅之烟，其髑如髅，其烟如王，善骨王", {u8"骨王"}, 0}; registerCard(c);
	c = Card{"xiqushen_jiao", u8"犀渠神角", 2, u8"其他", 1, 5, u8"——《山海经·中次八经》\"犀渠，一角，触物则毙\"，犀渠神角，其角如神，其触如毙，善嗜血狂热", {u8"嗜血狂热"}, 2}; registerCard(c);
	c = Card{"huansha_xisheng", u8"浣沙溪生", 1, u8"其他", 1, 1, u8"——《拾遗记》\"浣纱之生，拾骨为戏\"，浣沙溪生，其沙如浣，其生如戏，善拾荒者", {u8"拾荒者"}}; registerCard(c);
	c = Card{"xuanhuan", u8"玄獾", 5, u8"其他", 1, 3, u8"——《山海经·北山经》\"玄獾，黑如漆，噬则益力\"，玄獾之兽，其色如漆，其噬如力，善嗜血狂热", {u8"嗜血狂热", u8"消耗骨头"}}; registerCard(c);

	// 死亡卡
	c = Card{"louis", u8"吕布", 1, u8"死亡", 1, 1, u8"——《三国志》\"人中吕布，马中赤兔\"，人中吕布，其力如神，其马如兔，威震天下", {u8"冲刺能手", u8"水袭"}, 0}; registerCard(c);
	c = Card{"jonah", u8"赵云", 3, u8"死亡", 2, 5, u8"——《三国志》\"子龙一身是胆\"，子龙一身是胆，其胆如龙，其勇如神，善内心之蜂", {u8"内心之蜂", u8"反伤"}, 0}; registerCard(c);
	c = Card{"kevin", u8"关羽", 2, u8"死亡", 2, 3, u8"——《三国志》\"关云长义薄云天\"，关云长义薄云天，其义如云，其天如薄，善横冲直撞", {u8"横冲直撞", u8"断尾求生"}, 0}; registerCard(c);
	c = Card{"sean", u8"张飞", 2, u8"死亡", 1, 6, u8"——《三国志》\"张翼德勇冠三军\"，张翼德勇冠三军，其勇如冠，其军如三，善高跳死神", {u8"高跳", u8"死神之触"}, 0}; registerCard(c);
	c = Card{"tamara", u8"马超", 2, u8"死亡", 2, 3, u8"——《三国志》\"马孟起威震西凉\"，马孟起威震西凉，其威如震，其凉如西，善蛮力冲撞", {u8"蛮力冲撞", u8"断尾求生"}, 0}; registerCard(c);
	c = Card{"daniel", u8"黄忠", 2, u8"死亡", 2, 2, u8"——《三国志》\"黄汉升老当益壮\"，黄汉升老当益壮，其老如当，其壮如益，善双重攻击", {u8"双重攻击"}, 0}; registerCard(c);
	c = Card{"cody", u8"典韦", 2, u8"死亡", 3, 1, u8"——《三国志》\"典韦古之恶来\"，典韦古之恶来，其古如恶，其来如韦，善守护者", {u8"守护者", u8"断尾求生"}, 0}; registerCard(c);
	c = Card{"david", u8"许褚", 2, u8"死亡", 2, 4, u8"——《三国志》\"许仲康虎痴也\"，许仲康虎痴也，其虎如痴，其康如仲，善蛮力冲撞", {u8"蛮力冲撞", u8"反伤"}, 0}; registerCard(c);
	c = Card{"tahnee", u8"太史慈", 2, u8"死亡", 1, 3, u8"——《三国志》\"太史子义信义笃烈\"，太史子义信义笃烈，其义如信，其笃如烈，善三向攻击", {u8"三向攻击"}, 0}; registerCard(c);
	c = Card{"berke", u8"甘宁", 2, u8"死亡", 2, 1, u8"——《三国志》\"甘兴霸锦帆贼也\"，甘兴霸锦帆贼也，其霸如锦，其帆如贼，善守护者", {u8"守护者"}, 0}; registerCard(c);
	c = Card{"kaycee", u8"周泰", 1, u8"死亡", 1, 2, u8"——《三国志》\"周幼平忠勇无双\"，周幼平忠勇无双，其忠如勇，其双如无，善双向攻击", {u8"双向攻击", u8"反伤"}, 0}; registerCard(c);
	c = Card{"kaminski", u8"魏延", 1, u8"死亡", 0, 1, u8"——《三国志》\"魏文长反骨也\"，魏文长反骨也，其反如骨，其长如文，善守护者", {u8"守护者", u8"反伤", u8"消耗骨头"}, 0}; registerCard(c);
	c = Card{"reginald", u8"庞德", 3, u8"死亡", 1, 3, u8"——《三国志》\"庞令明白马将军\"，庞令明白马将军，其马如白，其军如令，善死神之触", {u8"死神之触", u8"消耗骨头"}, 0}; registerCard(c);
	c = Card{"Oct19", u8"文丑", 1, u8"死亡", 3, 2, u8"——《三国志》\"文丑河北名将\"，文丑河北名将，其丑如文，其北如名，善死神之触", {u8"死神之触"}, 0}; registerCard(c);
	c = Card{"Luke", u8"颜良", 0, u8"死亡", 4, 4, u8"——《三国志》\"颜良河北名将\"，颜良河北名将，其良如颜，其北如名，威震河北", {}, 0}; registerCard(c);

	//资源类
	c = Card{"moding", u8"墨锭", 0, u8"其他", 0, 1, u8"墨锭，其色如漆，其质如金", {}, 0}; registerCard(c);
	c = Card{"langpi", u8"稀有之墨", 0, u8"其他", 0, 2, u8"——\"墨者，黑也，其质稀有\"", {}, 0, false}; registerCard(c);
	c = Card{"jinang_mao", u8"传奇之墨", 0, u8"其他", 0, 3, u8"——\"墨者，传奇也，其色如金\"", {}, 0, false}; registerCard(c);
	c = Card{"tuopi_mao", u8"平凡之墨", 0, u8"其他", 0, 1, u8"——\"墨者，平凡也，其用甚广\"", {}, 0, false}; registerCard(c);

	//特殊牌
	c = Card{"bingfeng_jianjia", u8"冰封剑甲", 0, u8"其他", 0, 5, u8"冰封剑甲，其甲如冰，其刃如霜", {},0, false}; registerCard(c);
	c = Card{"panshi", u8"磐石", 0, u8"其他", 0, 5, u8"——《诗经》\"磐石之固，不可移也\"，磐石之固，其固如石，其移如不，善磐石之身", {u8"磐石之身"}, 0, false}; registerCard(c);
	c = Card{"jiaoyu", u8"蛟鱼", 0, u8"其他", 0, 1, u8"蛟鱼，其形如龙，其鳞如金", {}, 0, false}; registerCard(c);
	c = Card{"jinkuai", u8"墨块", 0, u8"其他", 0, 2, u8"——《墨经》\"墨块，其形如金，其质如墨\"，墨块之宝，其形如金，其质如墨，珍贵无比", {}, 0, false}; registerCard(c);
	c = Card{"yunshan", u8"云杉", 0, u8"其他", 0, 3, u8"云杉，其高如云，其枝如伞", {u8"高跳"}, 0, false}; registerCard(c);
	c = Card{"xueshan", u8"雪杉", 0, u8"其他", 0, 4, u8"雪杉，其高如雪，其枝如银", {u8"高跳"}, 0, false}; registerCard(c);
	c = Card{"moying", u8"墨影", 0, u8"鹿部", 0, 1, u8"墨影，其形如影，其色如墨", {}, 0}; registerCard(c);
	c = Card{"luma", u8"骡子", 0, u8"鹿部", 0, 5, u8"骡子，其力如马，其形如驴", {u8"冲刺能手"}, 0}; registerCard(c);
	c = Card{"hungry", u8"饥饿", 0, u8"其他", 1, 1, u8"饥饿，其形如饿，其声如饥", {u8"令人生厌"}, 0, false}; registerCard(c);
	c = Card{"muzhuang", u8"木桩", 0, u8"其他", 0, 3, u8"木桩，其形如柱，其质如木", {}, 0, false}; registerCard(c);
	c = Card{"qi_qingwa", u8"墨甲奇兵", 0, u8"其他", 1, 2, u8"——\"墨甲奇兵，其形如蛙，其甲如墨\"", {u8"高跳"}, 0, false}; registerCard(c);
	c = Card{"yueqiu", u8"兵马俑", 0, u8"其他", 1, 40, u8"——《史记·秦始皇本纪》\"兵马俑，其形如鸿，其势如军\"，兵马俑军，其形如鸿，其势如军，威震天下", {u8"高跳", u8"全向打击", u8"磐石"}, 0, false}; registerCard(c);
	c = Card{"tieshou_jia", u8"墨家机关术", 0, u8"其他", 0, 1, u8"——\"墨家机关术，其术如铁，其巧如神\"", {u8"墨家机关术",u8"高跳"}, 0, false}; registerCard(c);

}
