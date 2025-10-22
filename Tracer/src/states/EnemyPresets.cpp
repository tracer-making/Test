#include "EnemyPresets.h"
#include <iostream>
#include <algorithm>
#include <random>

EnemyPresetManager& EnemyPresetManager::instance() {
    static EnemyPresetManager instance;
    return instance;
}

std::vector<int> EnemyPresetManager::getAvailableBattleIds() const {
    return {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 18, 19, 20, 21, 22, 35, 39}; // 23场不同的战斗
}

// ==================== 渔夫一层战斗 ====================
static const std::vector<EnemyPresetRow> BATTLE_1_MATRIX = {//渔夫1层
    {{"yifei_yi",                   "",                  "", ""}, 0},
    {{"yifei_yi",                   "",                  "", ""}, 0},
    {{"yifei_yi",    "",                  "", ""}, 0 },
    {{"yifei_yi",                   "",                  "", ""}, 0},
    {{"muzhuang",             "",                  "", ""}, 0},
    {{"",             "",                  "", ""}, -1},
};


static const std::vector<EnemyPresetRow> BATTLE_2_MATRIX = {//渔夫1层
    {{"yunchuang_fengshi",                 "yunchuang_fengshi",                  "",                  ""}, 0},
    {{"yunchuang_fengshi",                 "",                  "",                  ""}, 0},
    {{"yunchuang_fengshi",                 "",                  "",                  ""}, 0},
    {{"",                 "",                  "",                  ""}, 0},
    {{"",                 "",                  "",                  ""}, 0},
};

static const std::vector<EnemyPresetRow> BATTLE_3_MATRIX = {//渔夫1层
    {{"chaojing_gongyi",            "juance_mingling",           "",        ""}, 1},
    {{"chaojing_gongyi",            "",           "",        ""}, 0 },
    {{"chaojing_gongyi",            "juance_mingling",           "",        ""}, 0},
    {{"",            "",           "",        ""}, 0},
    {{"",            "",           "",        ""}, 0},
};

static const std::vector<EnemyPresetRow> BATTLE_4_MATRIX = {//渔夫1层
    {{"bichan",    "",                  "",                   ""}, 0},
    {{"yegao_kangou",                   "",                  "",                   ""}, 0 },
    {{"bichan",                   "",                  "",                   ""}, 0},
    {{"muzhuang",             "",                  "",                   ""}, 0},
    {{"",             "",                  "",                   ""}, -1},
};

static const std::vector<EnemyPresetRow> BATTLE_5_MATRIX = {//渔夫一层
    {{"yifei_yi",   "bichan",      "",                   ""}, 1 },
    {{"yifei_yi",   "bichan",      "",                   ""}, 1 },
    {{"yunchuang_fengshi", "bichan",   "",                   ""}, 0 },
    {{"yunchuang_fengshi",    "",                  "",                   ""}, 0 },
    {{"yunchuang_fengshi",                   "",                  "",                   ""}, 0 },
    {{"",             "",            "",                   ""}, -1},
    {{"",             "",                  "",                   ""}, -1},
};

// ==================== 渔夫二层战斗 ====================
static const std::vector<EnemyPresetRow> BATTLE_6_MATRIX = {//渔夫二层
    {{"chaojing_gongyi",   "",      "",                   ""}, 0 },
    {{"sangjiu",   "",      "",                   ""}, 0 },
    {{"yifei_yi",   "bichan",      "",                   ""}, 1 },
    {{"shuangdao_moke", "yunchuang_fengshi",   "",                   ""}, 0 },
    {{"yifei_yi",    "",                  "",                   ""}, 0 },
    {{"yunchuang_fengshi",                   "yunchuang_fengshi",                  "",                   ""}, 0 },
    {{"",             "",            "",                   ""}, -1},
    {{"",             "",                  "",                   ""}, -1},
};

static const std::vector<EnemyPresetRow> BATTLE_7_MATRIX = {//渔夫二层
    {{"hegong_tuozi",             "",                  "",                   ""}, 0},
    {{"qingyu_cuishi",                   "yegao_kangou",                  "binque",                   ""}, 1 },
    {{"qingyu_cuishi",                   "yegao_kangou",                  "binque",                   ""}, 0 },
    {{"muzhuang",             "",            "",                   ""}, 0},
    {{"yunshan",             "",                  "",                   ""}, 0},
};

static const std::vector<EnemyPresetRow> BATTLE_8_MATRIX = {//渔夫二层
    {{"yifei_yi",    "",                  "",                   ""}, 0 },
    {{"chaojing_gongyi",    "langrong_qiushou",                  "",                   ""}, 0 },
    {{"chaojing_gongyi",    "",                  "",                   ""}, 0 },
    {{"yifei_yi",    "",                  "",                   ""}, 0 },
    {{"yifei_yi",    "",                  "",                   ""}, 0 },
    {{"yifei_yi",    "",                  "",                   ""}, 0 },
    {{"yifei_yi",                   "xuanwu",                  "",                   ""}, 0 },
    {{"shanlongzi",                   "yifei_yi",                  "",                   ""}, 0 },
    {{"",             "",            "",                   ""}, 0},
    {{"",             "",                  "",                   ""}, 0},
};

// ==================== 矿工二层战斗 ====================
static const std::vector<EnemyPresetRow> BATTLE_9_MATRIX = {//矿工二层
    {{"yunqu_youmi",                   "qijiao_shuangqi",                  "",                   ""}, 1 },
    {{"yunqu_youmi",                   "",                  "",                   ""}, 0 },
    {{"qijiao_shuangqi",             "yunqu_youmi",                  "yunqu_jumi",                   "xuanhuan"}, 3},
    {{"yunqu_youmi",                   "",                  "",                   ""}, 0 },
    {{"qijiao_shuangqi",                   "chuanfen_yanzi",                  "",                   ""}, 0 },
    {{"",             "",            "",                   ""}, 0},
    {{"panshi",             "",                  "",                   ""}, 0},
};
static const std::vector<EnemyPresetRow> BATTLE_39_MATRIX = {//矿工一层
    {{"yunqu_youmi",                   "qijiao_shuangqi",                  "",                   ""}, 1 },
    {{"yunqu_youmi",                   "",                  "",                   ""}, 0 },
    {{"qijiao_shuangqi",                   "chuanfen_yanzi",                  "",                   ""}, 0 },
    {{"",             "",            "",                   ""}, 0},
    {{"panshi",             "",                  "",                   ""}, 0},
};

static const std::vector<EnemyPresetRow> BATTLE_10_MATRIX = {//矿工一层
    {{"yunqu_youmi",                   "",                  "",                   ""}, 0 },
    {{"xuewei_yousheng",             "yunqu_youmi",                  "",                   ""}, 2},
    {{"",                   "",                  "",                   ""}, 0 },
    {{"qijiao_shuangqi",                   "chuanfen_yanzi",                  "",                   ""}, 0 },
    {{"panshi",             "",            "",                   ""}, 0},
    {{"",             "",                  "",                   ""}, 0},
};

static const std::vector<EnemyPresetRow> BATTLE_11_MATRIX = {//矿工二层
    {{"shuomuo_canglang",             "xueyuan_langpei",                  "langrong_qiushou",                   ""}, 2},
    {{"shuomo_canglang",                   "weijia_tong",                  "",                   ""}, 1 },
    {{"weijia_tong",                   "xueyuan_langpei",                  "baimao_zi",                   ""}, 0 },
    {{"yunshan",             "",            "",                   ""}, 0},
    {{"muzhuang",             "",                  "",                   ""}, 0},
};

static const std::vector<EnemyPresetRow> BATTLE_12_MATRIX = {//矿工二层
    {{"langrong_qiushou",             "baimao_zi",                  "jianjia_yu",                   ""}, 2},
    {{"shuomuo_canglang",             "xueyuan_langpei",                  "jianjia_yu",                   ""}, 2},
    {{"shuomo_canglang",                   "weijia_tong",                  "",                   ""}, 1 },
    {{"weijia_tong",                   "xueyuan_langpei",                  "baimao_zi",                   ""}, 0 },
    {{"panshi",             "",            "",                   ""}, 0},
    {{"panshi",             "",                  "",                   ""}, 0},
};

static const std::vector<EnemyPresetRow> BATTLE_13_MATRIX = {//矿工二层
    {{"binque",             "yegao_kangou",                  "weijia_tong",                   ""}, 1},
    {{"binque",             "yegao_kangou",                  "weijia_tong",                   ""}, 1},
    {{"binque",             "yegao_kangou",                  "",                   ""}, 2},
    {{"binque",             "yegao_kangou",                  "",                   ""}, 1},
    {{"yegao_kangou",                   "weijia_tong",                  "",                   ""}, 1 },
    {{"yegao_kangou",                   "weijia_tong",                  "",                   ""}, 0 },
    {{"",             "",            "",                   ""}, 0},
    {{"",             "",                  "",                   ""}, 0},
};
static const std::vector<EnemyPresetRow> BATTLE_35_MATRIX = {//矿工一层
    {{"binque",             "yegao_kangou",                  "",                   ""}, 1},
    {{"yegao_kangou",                   "weijia_tong",                  "",                   ""}, 1 },
    {{"yegao_kangou",                   "weijia_tong",                  "",                   ""}, 0 },
    {{"",             "",            "",                   ""}, 0},
    {{"",             "",                  "",                   ""}, 0},
};

static const std::vector<EnemyPresetRow> BATTLE_14_MATRIX = {//矿工二层
    {{"",             "xuewei_yousheng",                  "sangjiu",                   ""}, 1},
    {{"binque",             "xuewei_yousheng",                  "sangjiu",                   ""}, 2},
    {{"binque",             "xuanwu_zhi",                  "weijia_tong",                   ""}, 3},
    {{"binque",                   "xuanwu_zhi",                  "",                   ""}, 1 },
    {{"binque",                   "xuanwu",                  "",                   ""}, 0 },
    {{"",             "",            "",                   ""}, 0},
    {{"yunshan",             "",                  "",                   ""}, 0},
};

// ==================== 猎人一层战斗 ====================
static const std::vector<EnemyPresetRow> BATTLE_15_MATRIX = {//猎人一层
    {{"yunqu_youmi",             "chuanfen_yanzi",                  "",                   ""}, 1},
    {{"yunqu_youmi",             "weijia_tong",                  "",                   ""}, 1},
    {{"chuanfen_yanzi",                   "",                  "",                   ""}, 0 },
    {{"chuanfen_yanzi",                   "qianfeng_tuolu",                  "",                   ""}, 0 },
    {{"",             "",            "",                   ""}, 0},
    {{"",             "",                  "",                   ""}, 0},
};

// ==================== 猎人二层战斗 ====================
static const std::vector<EnemyPresetRow> BATTLE_16_MATRIX = {//猎人二层
    {{"jingguan_yeniu",             "yunqu_youmi",                  "",                   ""}, 1},
    {{"jingguan_yeniu",             "yunqu_youmi",                  "",                   ""}, 0},
    {{"langrong_qiushou",             "qijiao_shuangqi",                  "",                   ""}, 1},
    {{"langrong_qiushou",             "yunqu_youmi",                  "",                   ""}, 1},
    {{"weijia_tong",                   "",                  "",                   ""}, 0 },
    {{"baimao_zi",                   "jingguan_yeniu",                  "weijia_tong",                   ""}, 0 },
    {{"panshi",             "",            "",                   ""}, 0},
    {{"binfeng_jianjia",             "",                  "",                   ""}, 0},
};

static const std::vector<EnemyPresetRow> BATTLE_17_MATRIX = {//猎人二层
    {{"xuanwu",             "xuanwu_zhi",                  "chiling_jiu",                   ""}, 2},
    {{"huangyou_chouwei",             "xuanwu_zhi",                  "",                   ""}, 1},
    {{"xuanwu",                   "",                  "",                   ""}, 0 },
    {{"chiling_jiu",                   "chuanfen_yanzi",                  "",                   ""}, 0 },
    {{"",             "",            "",                   ""}, 0},
    {{"",             "",                  "",                   ""}, 0},
};

static const std::vector<EnemyPresetRow> BATTLE_22_MATRIX = {//猎人二层
    {{"yunqu_youmi",             "xuanwu_zhi",                  "",                   ""}, 1},
    {{"yunqu_youmi",             "xuanwu_zhi",                  "",                   ""}, 1},
    {{"yunqu_youmi",             "xuanwu_zhi",                  "",                   ""}, 1},
    {{"xuanwu",                   "xuanwu_zhi",                  "",                   ""}, 1 },
    {{"chiling_jiu",                   "chuanfen_yanzi",                  "",                   ""}, 0 },
    {{"",             "",            "",                   ""}, 0},
    {{"",             "",                  "",                   ""}, 0},
};

static const std::vector<EnemyPresetRow> BATTLE_18_MATRIX = {//猎人二层
    {{"",             "",                  "xuanwu",                   ""}, 0},
    {{"",             "xuanwu_zhi",                  "xuanwu",                   ""}, 1},
    {{"binque",             "xuanwu_zhi",                  "xuanwu",                   ""}, 1},
    {{"binque",             "xuanwu_zhi",                  "",                   ""}, 1},
    {{"weijia_tong",             "xuanwu_zhi",                  "",                   ""}, 2},
    {{"xuanwu",                   "xuanwu_zhi",                  "weijia_tong",                   ""}, 3 },
    {{"panshi",             "",            "",                   ""}, 0},
    {{"",             "",                  "",                   ""}, 0},
};

static const std::vector<EnemyPresetRow> BATTLE_19_MATRIX = {//猎人一层
    {{"weijia_tong",             "xuanwu_zhi",                  "",                   ""}, 1},
    {{"",             "xuanwu_zhi",                  "",                   ""}, 0},
    {{"chuanfen_yanzi",                   "xuanwu_zhi",                  "",                   ""}, 0 },
    {{"",             "",            "",                   ""}, 0},
    {{"",             "",                  "",                   ""}, 0},
};

static const std::vector<EnemyPresetRow> BATTLE_20_MATRIX = {//猎人一层
    {{"xuanwu_zhi",             "yunqu_youmi",                  "",                   ""}, 1},
    {{"xuanwu_zhi",             "yunqu_youmi",                  "",                   ""}, 1},
    {{"",             "",                  "",                   ""}, 0},
    {{"",             "",                  "",                   ""}, 0},
    {{"",             "",                  "",                   ""}, 0},
    {{"yunqu_youmi",             "xuanhuan",                  "",                   ""}, 0},
    {{"",             "",            "",                   ""}, 0},
    {{"",             "",                  "",                   ""}, 0},
};

static const std::vector<EnemyPresetRow> BATTLE_21_MATRIX = {//猎人一层
    {{"chuanfen_yanzi",             "yunqu_youmi",                  "",                   ""}, 1},
    {{"chuanfen_yanzi",             "yunqu_youmi",                  "",                   ""}, 1},
    {{"",             "",                  "",                   ""}, 0},
    {{"",             "",                  "",                   ""}, 0},   
    {{"yunqu_youmi",             "",                  "",                   ""}, 0},
    {{"baimao_zi",             "yunqu_youmi",                  "",                   ""}, 2},   
    {{"",             "",                  "",                   ""}, 0},
    {{"",             "",                  "",                   ""}, 0},
};









static const std::vector<EnemyPresetRow> BOSS_FINAL_MATRIX = {
    // 最终Boss第一阶段：强大的敌人军团
    {{"baina_ou",        "daobi_li",         "maomin",     ""}, 1 },
    {{"baina_ou",   "daobi_li",   "maomin",       ""}, 1 },
    {{"",                   "",                  "",                   ""}, 0 },
    {{"",                   "",                  "",                   ""}, 0 },
    {{"baina_ou",    "daobi_li",   "chuanfen_yinshi",  ""}, 1 },
    {{"shuangdao_moke",   "daobi_li",      "chuanfen_yinshi", ""}, 1 },
    {{"baina_ou", "shuangdao_moke",   "",   ""}, 1 },
    {{"baina_ou",    "shuangdao_moke",                  "",                   ""}, 1 },
    {{"baina_ou",                   "",                  "",                   ""}, 0 },
    {{"chuanfen_yinshi",             "",            "",             ""}, 0},
    {{"panshi",             "",            "",             ""}, 0},
};

// 第四层最终Boss第二阶段：终极形态
static const std::vector<EnemyPresetRow> BOSS_FINAL_PHASE2_MATRIX = {
    // 最终Boss第二阶段：更强大的敌人军团
    
    {{"muzhuang",             "",            "",             ""}, 0},
    {{"",             "",            "",             ""}, 0},
};

// Boss战预设（按环境）
static const std::vector<EnemyPresetRow> BOSS_MINER_MATRIX = {
    // 矿工（林地Boss）：偏重地面与障碍
    {{"bashe",               "canglang_youhun",               "shuomo_canglang",               "chuanfen_yanzi"}, 1 },
    {{"bashe",               "canglang_youhun",               "shuomo_canglang",               "chuanfen_yanzi"}, 1 },
    {{"bashe",               "canglang_youhun",               "shuomo_canglang",               "chuanfen_yanzi"}, 1 },
    {{"bashe",               "canglang_youhun",               "shuomo_canglang",               "chuanfen_yanzi"}, 1 },
    {{"bashe",               "canglang_youhun",               "shuomo_canglang",               "chuanfen_yanzi"}, 1 },
    {{"bashe",               "canglang_youhun",               "shuomo_canglang",               "chuanfen_yanzi"}, 1 },
    {{"luma",               "yegao_kangou",               "",               ""}, -1},
    {{"",               "",               "",               ""}, -1},
    {{"panshi",         "",               "",               ""}, 0},
};
static const std::vector<EnemyPresetRow> BOSS_MINER_PHASE2_MATRIX = {
    // 矿工二阶段：更强大的地面单位
    {{"bashe",               "canglang_youhun",               "shuomo_canglang",               "chuanfen_yanzi"}, 1 },
    {{"bashe",               "canglang_youhun",               "shuomo_canglang",               "chuanfen_yanzi"}, 1 },
    {{"bashe",               "canglang_youhun",               "shuomo_canglang",               "chuanfen_yanzi"}, 1 },
    {{"bashe",               "canglang_youhun","",              ""}, 1 },
    {{"bashe",               "canglang_youhun","",               ""}, 1 },
    {{"bashe",                "canglang_youhun",               "",               ""}, 1 },
    {{"",               "",               "",               ""}, 1 },
    {{"xuezong_xiquan",               "",               "",               ""}, 0},
    {{"",               "",               "",               ""}, -1},
    {{"",               "",               "",               ""}, -1},
};
static const std::vector<EnemyPresetRow> BOSS_FISHERMAN_MATRIX = {
    // 渔夫（湿地Boss）：偏重水域单位
    {{"qingyu_cuishi",               "hegong_tuozi",               "",               ""}, 1},
    {{"qingyu_cuishi",               "hegong_tuozi",               "",               ""}, 2},
    {{"qingyu_cuishi",               "hegong_tuozi",               "",               ""}, 1},
    {{"qingyu_cuishi",               "hegong_tuozi",               "",               ""}, 2},
    {{"qingyu_cuishi",               "hegong_tuozi",               "",               ""}, 2},
    {{"qingyu_cuishi",               "hegong_tuozi",               "",               ""}, 1},
    {{"",               "",               "",               ""}, 0},
    {{"",               "",               "",               ""}, 0},
};
static const std::vector<EnemyPresetRow> BOSS_FISHERMAN_PHASE2_MATRIX = {
    // 更多水域生物与召唤
    {{"jiaoyu",               "",               "",               ""}, 0 },
    {{"jiaoyu",               "",               "",               ""}, 0 },
    {{"jiaoyu",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0 },
    {{"jiaoyu",               "",               "",               ""}, 0 },
    {{"jiaoyu",               "",               "",               ""}, 0 },
    {{"jiaoyu",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0},
    {{"",               "",               "",               ""}, 0},
};

static const std::vector<EnemyPresetRow> BOSS_HUNTER_MATRIX = {
    // 猎人（雪原Boss）：偏重远程与追踪
    {{"qi_qingwa",               "baimao_zi",               "",               ""}, 1 },
    {{"qi_qingwa",               "baimao_zi",               "",               ""}, 1 },
    {{"qi_qingwa",               "baimao_zi",               "",               ""}, 1 },
    {{"bichan",               "",               "",               ""}, 0 },
    {{"qi_qingwa",               "bichan",               "",               ""}, 1 },
    {{"",               "bichan",               "",               ""}, -1},
    {{"qi_qingwa",               "",               "qi_qingwa",               "tieshou_jia"}, -1},
    {{"",               "",               "",               ""}, 0},
};




EnemyPresetMatrix EnemyPresetManager::getMatrix(int battleId) const {
    EnemyPresetMatrix m;
    
    switch (battleId) {
        case 1: m.rows = BATTLE_1_MATRIX; break;
        case 2: m.rows = BATTLE_2_MATRIX; break;
        case 3: m.rows = BATTLE_3_MATRIX; break;
        case 4: m.rows = BATTLE_4_MATRIX; break;
        case 5: m.rows = BATTLE_5_MATRIX; break;
        case 6: m.rows = BATTLE_6_MATRIX; break;
        case 7: m.rows = BATTLE_7_MATRIX; break;
        case 8: m.rows = BATTLE_8_MATRIX; break;
        case 9: m.rows = BATTLE_9_MATRIX; break;
        case 10: m.rows = BATTLE_10_MATRIX; break;
        case 11: m.rows = BATTLE_11_MATRIX; break;
        case 12: m.rows = BATTLE_12_MATRIX; break;
        case 14: m.rows = BATTLE_14_MATRIX; break;
        case 15: m.rows = BATTLE_15_MATRIX; break;
        case 16: m.rows = BATTLE_16_MATRIX; break;
        case 17: m.rows = BATTLE_17_MATRIX; break;
        case 18: m.rows = BATTLE_18_MATRIX; break;
        case 19: m.rows = BATTLE_19_MATRIX; break;
        case 20: m.rows = BATTLE_20_MATRIX; break;
        case 21: m.rows = BATTLE_21_MATRIX; break;
        case 22: m.rows = BATTLE_22_MATRIX; break;
        case 35: m.rows = BATTLE_35_MATRIX; break;
        case 39: m.rows = BATTLE_39_MATRIX; break;
        case 100: m.rows = BOSS_MINER_MATRIX; break; // 矿工Boss战
        case 101: m.rows = BOSS_MINER_PHASE2_MATRIX; break; // 矿工Boss二阶段
        case 102: m.rows = BOSS_FISHERMAN_MATRIX; break; // 渔夫Boss战
        case 103: m.rows = BOSS_FISHERMAN_PHASE2_MATRIX; break; // 渔夫Boss二阶段
        case 104: m.rows = BOSS_HUNTER_MATRIX; break; // 猎人Boss战
        case 105: m.rows = BOSS_FINAL_MATRIX; break; // 最终Boss战
        case 106: m.rows = BOSS_FINAL_PHASE2_MATRIX; break; // 最终Boss二阶段
        default: m.rows = BATTLE_1_MATRIX; break; // 默认使用第一场战斗
    }
    
    return m;
}

EnemyPresetMatrix EnemyPresetManager::getBossMatrixForBiome(const std::string& biome) const {
    EnemyPresetMatrix m;
    if (biome == std::string(u8"林地")) {
        m.rows = BOSS_MINER_MATRIX;
    } else if (biome == std::string(u8"湿地")) {
        m.rows = BOSS_FISHERMAN_MATRIX;
    } else if (biome == std::string(u8"雪原")) {
        m.rows = BOSS_HUNTER_MATRIX;
    } else if (biome == std::string(u8"最终Boss")) {
        m.rows = BOSS_FINAL_MATRIX;
    } else {
        m.rows = BOSS_MINER_MATRIX;
    }
    return m;
}

EnemyPresetMatrix EnemyPresetManager::getBossMatrixForBiomePhase(const std::string& biome, int phase) const {
    if (phase <= 1) return getBossMatrixForBiome(biome);
    EnemyPresetMatrix m;
    if (biome == std::string(u8"林地")) {
        m.rows = BOSS_MINER_PHASE2_MATRIX;
    } else if (biome == std::string(u8"湿地")) {
        m.rows = BOSS_FISHERMAN_PHASE2_MATRIX;
    } else if (biome == std::string(u8"雪原")) {
        // 猎人无第二阶段 → 返回第一阶段
        m = getBossMatrixForBiome(biome);
        return m;
    } else if (biome == std::string(u8"最终Boss")) {
        m.rows = BOSS_FINAL_PHASE2_MATRIX;
    } else {
        m.rows = BOSS_MINER_PHASE2_MATRIX;
    }
    return m;
}

// 根据地形层数获取可用的战斗ID池
std::vector<int> EnemyPresetManager::getBattleIdsForBiome(const std::string& biome, int layer) const {
    std::vector<int> availableBattles;
    
    // 动态地形映射：根据实际地形类型选择对应的生物群落战斗预设
    if (biome == std::string(u8"林地") || biome == std::string(u8"森林") || biome == std::string(u8"树林")) {
        // 林地类型 → 矿工战斗预设
        if (layer == 1) {
            availableBattles = {35, 39}; // 矿工一层
        } else if (layer == 2) {
            availableBattles = {9, 10, 11, 12, 14}; // 矿工二层
        } else if (layer == 3) {
            availableBattles = {9, 10, 11, 12, 14}; // 矿工三层（使用二层预设）
        }
    } else if (biome == std::string(u8"湿地") || biome == std::string(u8"沼泽") || biome == std::string(u8"水域")) {
        // 湿地类型 → 渔夫战斗预设
        if (layer == 1) {
            availableBattles = {1, 2, 3, 4, 5}; // 渔夫一层
        } else if (layer == 2) {
            availableBattles = {6, 7, 8}; // 渔夫二层
        } else if (layer == 3) {
            availableBattles = {6, 7, 8}; // 渔夫三层（使用二层预设）
        }
    } else if (biome == std::string(u8"雪地") || biome == std::string(u8"雪山") || biome == std::string(u8"冰原")) {
        // 雪地类型 → 猎人战斗预设
        if (layer == 1) {
            availableBattles = {15, 19, 20, 21}; // 猎人一层
        } else if (layer == 2) {
            availableBattles = {16, 17, 18, 22}; // 猎人二层
        } else if (layer == 3) {
            availableBattles = {16, 17, 18, 22}; // 猎人三层（使用二层预设）
        }
    } else {
        // 未知地形类型，使用默认战斗预设
        std::cout << "[ENEMYPRESETS] 未知地形类型: " << biome << ", 使用默认战斗预设" << std::endl;
        availableBattles = {1, 2, 3, 4, 5}; // 默认使用渔夫一层
    }
    
    return availableBattles;
}

// 随机分配不重复的战斗节点
std::vector<int> EnemyPresetManager::getRandomBattleSequence(const std::string& biome, int layer, int nodeCount) const {
    std::vector<int> availableBattles = getBattleIdsForBiome(biome, layer);
    std::vector<int> selectedBattles;
    
    if (availableBattles.empty() || nodeCount <= 0) {
        return selectedBattles;
    }
    
    // 如果需要的节点数大于可用战斗数，则允许重复
    if (nodeCount > availableBattles.size()) {
        // 先添加所有可用战斗
        selectedBattles = availableBattles;
        // 然后随机补充到所需数量
        for (int i = availableBattles.size(); i < nodeCount; i++) {
            int randomIndex = rand() % availableBattles.size();
            selectedBattles.push_back(availableBattles[randomIndex]);
        }
    } else {
        // 随机选择不重复的战斗
        std::vector<int> tempBattles = availableBattles;
        for (int i = 0; i < nodeCount; i++) {
            int randomIndex = rand() % tempBattles.size();
            selectedBattles.push_back(tempBattles[randomIndex]);
            tempBattles.erase(tempBattles.begin() + randomIndex);
        }
    }
    
    return selectedBattles;
}

/*
使用示例：

// 根据地形和层数获取战斗池
auto forestLayer1 = EnemyPresetManager::instance().getBattleIdsForBiome("林地", 1);
// 返回: {35, 39} (矿工一层)

auto wetlandLayer2 = EnemyPresetManager::instance().getBattleIdsForBiome("湿地", 2);
// 返回: {6, 7, 8} (渔夫二层)

// 随机分配战斗
auto randomBattles = EnemyPresetManager::instance().getRandomBattleSequence("林地", 1, 1);
// 可能返回: {35} 或 {39}

地形层数映射：
- 林地: 一层(矿工一层35,39), 二层(矿工二层9-12,14), 三层(矿工二层9-12,14)
- 湿地: 一层(渔夫一层1-5), 二层(渔夫二层6-8), 三层(渔夫二层6-8)
- 雪地: 一层(猎人一层15,19-21), 二层(猎人二层16-18,22), 三层(猎人二层16-18,22)

注意：三层使用与二层相同的战斗池
*/
