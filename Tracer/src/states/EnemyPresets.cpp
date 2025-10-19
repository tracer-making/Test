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
    {{"chiling_jiu",        "bangu_peng",        "", ""}, true },
    // rows[1] = 敌人第五回合放置（第一行）
    {{"yegao_kangou",       "langrong_qiushou",  "", ""}, true },
    // rows[2] = 敌人第四回合放置（第一行）
    {{"xueyuan_langpei",    "wengjian_choucheng","", ""}, true },
    // rows[3] = 敌人第三回合放置（第一行）
    {{"shuangya_zhanlang",  "langrong_qiushou",  "", ""}, true },
    // rows[4] = 敌人第二回合放置（第一行）
    {{"wengjian_choucheng", "xueyuan_langpei",   "", ""}, true },
    // rows[5] = 敌人第一回合放置（第一行）
    {{"canglang_youhun",    "",                  "", ""}, true },
    // rows[6] = 初始化敌人第一行
    {{"",                   "",                  "", ""}, false},
    // rows[7] = 初始化战斗区域第二行
    {{"panshi",             "",                  "", ""}, false},
    // rows[8] = 初始化战斗区域第三行
    {{"panshi",             "",                  "", ""}, false},
};

// 第二场战斗：狼群围攻
static const std::vector<EnemyPresetRow> BATTLE_2_MATRIX = {
    {{"shuomuo_canglang", "xuezong_xiquan",     "shuangya_zhanlang", "langrong_qiushou"}, true },
    {{"canglang_youhun",  "xueyuan_langpei",   "yegao_kangou",      ""}, true },
    {{"langrong_qiushou", "shuangya_zhanlang", "",                  ""}, true },
    {{"xueyuan_langpei",  "canglang_youhun",   "",                  ""}, true },
    {{"canglang_youhun",  "",                  "",                  ""}, true },
    {{"",                 "",                  "",                  ""}, true },
    {{"",                 "",                  "",                  ""}, false},
    {{"",                 "",                  "",                  ""}, false},
    {{"",                 "",                  "",                  ""}, false},
};

// 第三场战斗：空中威胁
static const std::vector<EnemyPresetRow> BATTLE_3_MATRIX = {
    {{"chiling_jiu", "bangu_peng", "lingque", "binque"}, true },
    {{"chiling_jiu", "bangu_peng", "",        ""}, true },
    {{"lingque",     "binque",     "",        ""}, true },
    {{"binque",      "",           "",        ""}, true },
    {{"",            "",           "",        ""}, true },
    {{"",            "",           "",        ""}, true },
    {{"",            "",           "",        ""}, false},
    {{"",            "",           "",        ""}, false},
    {{"",            "",           "",        ""}, false},
};

// 第四场战斗：混合军团
static const std::vector<EnemyPresetRow> BATTLE_4_MATRIX = {
    {{"chiling_jiu",        "shuangya_zhanlang",  "wengjian_choucheng", "xuezong_xiquan"}, true },
    {{"bangu_peng",         "langrong_qiushou",   "yegao_kangou",       ""}, true },
    {{"xueyuan_langpei",    "canglang_youhun",   "shuangya_zhanlang",  ""}, true },
    {{"wengjian_choucheng", "xueyuan_langpei",   "",                   ""}, true },
    {{"canglang_youhun",    "",                  "",                   ""}, true },
    {{"",                   "",                  "",                   ""}, true },
    {{"",                   "",                  "",                   ""}, false},
    {{"panshi",             "",                  "",                   ""}, false},
    {{"panshi",             "",                  "",                   ""}, false},
};

// 第五场战斗：终极挑战
static const std::vector<EnemyPresetRow> BATTLE_5_MATRIX = {
    {{"chiling_jiu",        "bangu_peng",         "xuezong_xiquan",     "shuomuo_canglang"}, true },
    {{"shuangya_zhanlang", "langrong_qiushou",   "yegao_kangou",       "wengjian_choucheng"}, true },
    {{"xueyuan_langpei",    "canglang_youhun",   "shuangya_zhanlang",  ""}, true },
    {{"langrong_qiushou",   "yegao_kangou",      "",                   ""}, true },
    {{"wengjian_choucheng", "xueyuan_langpei",   "",                   ""}, true },
    {{"canglang_youhun",    "",                  "",                   ""}, true },
    {{"",                   "",                  "",                   ""}, false},
    {{"panshi",             "panshi",            "",                   ""}, false},
    {{"panshi",             "",                  "",                   ""}, false},
};

EnemyPresetMatrix EnemyPresetManager::getMatrix(int battleId) const {
    EnemyPresetMatrix m;
    
    switch (battleId) {
        case 1: m.rows = BATTLE_1_MATRIX; break;
        case 2: m.rows = BATTLE_2_MATRIX; break;
        case 3: m.rows = BATTLE_3_MATRIX; break;
        case 4: m.rows = BATTLE_4_MATRIX; break;
        case 5: m.rows = BATTLE_5_MATRIX; break;
        default: m.rows = BATTLE_1_MATRIX; break; // 默认使用第一场战斗
    }
    
    return m;
}
