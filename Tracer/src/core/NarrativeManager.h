#pragma once

#include <string>
#include <functional>
#include <memory>
#include <map>

class State;
class App;

// 叙事管理器，用于在界面跳转间插入文本播放
class NarrativeManager {
public:
    // 叙事跳转类型
    enum class NarrativeType {
        MainMenuToDeckSelect,      // 主菜单 -> 牌组选择
        DeckSelectToMapExplore,    // 牌组选择 -> 地图探索
        MapExploreToBattle,        // 地图探索 -> 战斗
        BattleToMapExplore,        // 战斗 -> 地图探索
        MapExploreToNode,          // 地图探索 -> 各个节点
        NodeToMapExplore,          // 节点 -> 地图探索
        BattleToVictory,           // 战斗 -> 胜利
        BattleToMemoryRepair,      // 战斗 -> 记忆修复
        // 地图探索到各个节点
        MapExploreToHeritage,      // 地图探索 -> 文脉传承
        MapExploreToEngrave,       // 地图探索 -> 意境刻画
        MapExploreToInkWorkshop,   // 地图探索 -> 墨工坊
        MapExploreToInkGhost,      // 地图探索 -> 墨鬼
        MapExploreToBarter,        // 地图探索 -> 以物易物
        MapExploreToMemoryRepair,  // 地图探索 -> 记忆修复
        MapExploreToRelicPickup,   // 地图探索 -> 墨宝拾遗
        MapExploreToSeeker,        // 地图探索 -> 寻求者
        MapExploreToTemper,        // 地图探索 -> 锤炼
        MapExploreToBurn,          // 地图探索 -> 燃烧
        MapExploreToCombine,       // 地图探索 -> 融合
        MapExploreToInkShop,       // 地图探索 -> 墨店
        MapExploreToWenxinTrial,   // 地图探索 -> 文心试炼
        // 各个节点到地图探索
        HeritageToMapExplore,      // 文脉传承 -> 地图探索
        EngraveToMapExplore,       // 意境刻画 -> 地图探索
        InkWorkshopToMapExplore,   // 墨工坊 -> 地图探索
        InkGhostToMapExplore,      // 墨鬼 -> 地图探索
        BarterToMapExplore,        // 以物易物 -> 地图探索
        MemoryRepairToMapExplore,  // 记忆修复 -> 地图探索
        RelicPickupToMapExplore,   // 墨宝拾遗 -> 地图探索
        SeekerToMapExplore,        // 寻求者 -> 地图探索
        TemperToMapExplore,        // 锤炼 -> 地图探索
        BurnToMapExplore,          // 燃烧 -> 地图探索
        CombineToMapExplore,       // 融合 -> 地图探索
        InkShopToMapExplore,       // 墨店 -> 地图探索
        WenxinTrialToMapExplore,   // 文心试炼 -> 地图探索
        FinalBossVictory,          // 最终Boss胜利 -> 主菜单
        Custom                     // 自定义跳转
    };
    
    // 设置叙事跳转
    static void setNarrativeTransition(NarrativeType type, const std::string& textFile, 
                                      std::function<std::unique_ptr<State>()> nextStateFactory);
    
    // 执行叙事跳转
    static void performNarrativeTransition(App& app, NarrativeType type);
    
    // 执行自定义叙事跳转
    static void performCustomNarrativeTransition(App& app, const std::string& textFile, 
                                               std::function<std::unique_ptr<State>()> nextStateFactory);
    
    // 检查是否有叙事跳转
    static bool hasNarrativeTransition(NarrativeType type);
    
    // 获取叙事文本文件路径
    static std::string getNarrativeTextFile(NarrativeType type);
    
    // 获取下一个状态工厂函数
    static std::function<std::unique_ptr<State>()> getNextStateFactory(NarrativeType type);
    
    // 获取预设名称
    static std::string getPresetName(NarrativeType type);

    // 按 layer/index 组织的战斗过渡助手：
    // before=true 使用 battle_{layer}_{index}_before，否则使用 *_after
    static void performBattlePreset(App& app,
                                    int layer,
                                    int index,
                                    bool before,
                                    std::function<std::unique_ptr<State>()> nextStateFactory,
                                    const std::string& textFile = "assets/narrative/all_narratives.txt");

private:
    struct NarrativeTransition {
        std::string textFile;
        std::function<std::unique_ptr<State>()> nextStateFactory;
    };
    
    static std::map<NarrativeType, NarrativeTransition> transitions_;
};
