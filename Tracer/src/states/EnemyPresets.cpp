#include "EnemyPresets.h"

EnemyPresetManager& EnemyPresetManager::instance() {
    static EnemyPresetManager instance;
    return instance;
}

std::vector<int> EnemyPresetManager::getAvailableBattleIds() const {
    return {1, 2, 3, 4, 5}; // 5场不同的战斗
}

// 第一场战斗：基础战斗
static const std::vector<EnemyPresetRow> BATTLE_1_MATRIX = {
    // rows[0] = 敌人第六回合放置（第一行）
    {{"chiling_jiu",        "bangu_peng",        "", ""}, 2 },
    // rows[1] = 敌人第五回合放置（第一行）
    {{"yegao_kangou",       "langrong_qiushou",  "", ""}, 2 },
    // rows[2] = 敌人第四回合放置（第一行）
    {{"xueyuan_langpei",    "wengjian_choucheng","", ""}, 2 },
    // rows[3] = 敌人第三回合放置（第一行）
    {{"shuangya_zhanlang",  "langrong_qiushou",  "", ""}, 2 },
    // rows[4] = 敌人第二回合放置（第一行）
    {{"wengjian_choucheng", "xueyuan_langpei",   "", ""}, 2 },
    // rows[5] = 敌人第一回合放置（第一行）
    {{"canglang_youhun",    "",                  "", ""}, 1 },
    // rows[6] = 初始化敌人第一行
    {{"",                   "",                  "", ""}, 0},
    // rows[7] = 初始化战斗区域第二行
    {{"panshi",             "",                  "", ""}, -1},
    // rows[8] = 初始化战斗区域第三行
    {{"panshi",             "",                  "", ""}, -1},
};

// 第二场战斗：狼群围攻
static const std::vector<EnemyPresetRow> BATTLE_2_MATRIX = {
    {{"shuomuo_canglang", "xuezong_xiquan",     "shuangya_zhanlang", "langrong_qiushou"}, 3 },
    {{"canglang_youhun",  "xueyuan_langpei",   "yegao_kangou",      ""}, 2 },
    {{"langrong_qiushou", "shuangya_zhanlang", "",                  ""}, 2 },
    {{"xueyuan_langpei",  "canglang_youhun",   "",                  ""}, 2 },
    {{"canglang_youhun",  "",                  "",                  ""}, 1 },
    {{"",                 "",                  "",                  ""}, 0 },
    {{"",                 "",                  "",                  ""}, 0},
    {{"",                 "",                  "",                  ""}, 0},
    {{"",                 "",                  "",                  ""}, 0},
};

// 第三场战斗：空中威胁
static const std::vector<EnemyPresetRow> BATTLE_3_MATRIX = {
    {{"chiling_jiu", "bangu_peng", "lingque", "binque"}, 2 },
    {{"chiling_jiu", "bangu_peng", "",        ""}, 2 },
    {{"lingque",     "binque",     "",        ""}, 2 },
    {{"binque",      "",           "",        ""}, 1 },
    {{"",            "",           "",        ""}, 0 },
    {{"",            "",           "",        ""}, 0 },
    {{"",            "",           "",        ""}, 0},
    {{"",            "",           "",        ""}, 0},
    {{"",            "",           "",        ""}, 0},
};

// 第四场战斗：混合军团
static const std::vector<EnemyPresetRow> BATTLE_4_MATRIX = {
    {{"chiling_jiu",        "shuangya_zhanlang",  "wengjian_choucheng", "xuezong_xiquan"}, 3 },
    {{"bangu_peng",         "langrong_qiushou",   "yegao_kangou",       ""}, 2 },
    {{"xueyuan_langpei",    "canglang_youhun",   "shuangya_zhanlang",  ""}, 2 },
    {{"wengjian_choucheng", "xueyuan_langpei",   "",                   ""}, 2 },
    {{"canglang_youhun",    "",                  "",                   ""}, 1 },
    {{"",                   "",                  "",                   ""}, 0 },
    {{"",                   "",                  "",                   ""}, 0},
    {{"panshi",             "",                  "",                   ""}, -1},
    {{"panshi",             "",                  "",                   ""}, -1},
};

// 第五场战斗：终极挑战
static const std::vector<EnemyPresetRow> BATTLE_5_MATRIX = {
    {{"chiling_jiu",        "bangu_peng",         "xuezong_xiquan",     "shuomuo_canglang"}, 3 },
    {{"shuangya_zhanlang", "langrong_qiushou",   "yegao_kangou",       "wengjian_choucheng"}, 3 },
    {{"xueyuan_langpei",    "canglang_youhun",   "shuangya_zhanlang",  ""}, 2 },
    {{"langrong_qiushou",   "yegao_kangou",      "",                   ""}, 2 },
    {{"wengjian_choucheng", "xueyuan_langpei",   "",                   ""}, 2 },
    {{"canglang_youhun",    "",                  "",                   ""}, 1 },
    {{"",                   "",                  "",                   ""}, 0 },
    {{"panshi",             "panshi",            "",                   ""}, -1},
    {{"panshi",             "",                  "",                   ""}, -1},
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
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0 },
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
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0},
    {{"",               "",               "",               ""}, 0},
    {{"panshi",         "panshi",         "panshi",         ""}, -1},
    {{"panshi",         "panshi",         "",               ""}, -1},
};
static const std::vector<EnemyPresetRow> BOSS_FISHERMAN_PHASE2_MATRIX = {
    // 更多水域生物与召唤
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0},
    {{"",               "",               "",               ""}, 0},
    {{"panshi",         "",               "",               ""}, 0},
};

static const std::vector<EnemyPresetRow> BOSS_HUNTER_MATRIX = {
    // 猎人（雪原Boss）：偏重远程与追踪
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0 },
    {{"",               "",               "",               ""}, 0},
    {{"",               "",               "",               ""}, 0},
    {{"panshi",         "",               "",               ""}, -1},
};




EnemyPresetMatrix EnemyPresetManager::getMatrix(int battleId) const {
    EnemyPresetMatrix m;
    
    switch (battleId) {
        case 1: m.rows = BATTLE_1_MATRIX; break;
        case 2: m.rows = BATTLE_2_MATRIX; break;
        case 3: m.rows = BATTLE_3_MATRIX; break;
        case 4: m.rows = BATTLE_4_MATRIX; break;
        case 5: m.rows = BATTLE_5_MATRIX; break;
        case 100: m.rows = BOSS_MINER_MATRIX; break; // 矿工Boss战
        case 101: m.rows = BOSS_MINER_PHASE2_MATRIX; break; // 矿工Boss二阶段
        case 102: m.rows = BOSS_FISHERMAN_MATRIX; break; // 渔夫Boss战
        case 103: m.rows = BOSS_FISHERMAN_PHASE2_MATRIX; break; // 渔夫Boss二阶段
        case 104: m.rows = BOSS_HUNTER_MATRIX; break; // 猎人Boss战
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
    } else {
        m.rows = BOSS_MINER_PHASE2_MATRIX;
    }
    return m;
}
