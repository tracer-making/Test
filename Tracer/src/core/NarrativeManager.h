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

private:
    struct NarrativeTransition {
        std::string textFile;
        std::function<std::unique_ptr<State>()> nextStateFactory;
    };
    
    static std::map<NarrativeType, NarrativeTransition> transitions_;
};
