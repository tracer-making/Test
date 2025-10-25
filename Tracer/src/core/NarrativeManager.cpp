#include "NarrativeManager.h"
#include "App.h"
#include "../states/TextPlayerState.h"
#include <map>

std::map<NarrativeManager::NarrativeType, NarrativeManager::NarrativeTransition> NarrativeManager::transitions_;

void NarrativeManager::setNarrativeTransition(NarrativeType type, const std::string& textFile, 
                                           std::function<std::unique_ptr<State>()> nextStateFactory) {
    transitions_[type] = {textFile, nextStateFactory};
}

void NarrativeManager::performNarrativeTransition(App& app, NarrativeType type) {
    auto it = transitions_.find(type);
    if (it != transitions_.end()) {
        // 创建文本播放状态，传入下一个状态的工厂函数
        auto textPlayer = std::make_unique<TextPlayerState>();
        textPlayer->setTextFile("assets/narrative/all_narratives.txt");
        textPlayer->setPresetName(getPresetName(type));
        textPlayer->setNextStateFactory(it->second.nextStateFactory);
        app.setState(std::move(textPlayer));
    }
}

void NarrativeManager::performCustomNarrativeTransition(App& app, const std::string& textFile, 
                                                     std::function<std::unique_ptr<State>()> nextStateFactory) {
    auto textPlayer = std::make_unique<TextPlayerState>();
    textPlayer->setTextFile(textFile);
    textPlayer->setNextStateFactory(nextStateFactory);
    app.setState(std::move(textPlayer));
}

bool NarrativeManager::hasNarrativeTransition(NarrativeType type) {
    return transitions_.find(type) != transitions_.end();
}

std::string NarrativeManager::getNarrativeTextFile(NarrativeType type) {
    auto it = transitions_.find(type);
    return (it != transitions_.end()) ? it->second.textFile : "";
}

std::function<std::unique_ptr<State>()> NarrativeManager::getNextStateFactory(NarrativeType type) {
    auto it = transitions_.find(type);
    return (it != transitions_.end()) ? it->second.nextStateFactory : nullptr;
}

std::string NarrativeManager::getPresetName(NarrativeType type) {
    switch (type) {
        case NarrativeType::MainMenuToDeckSelect:
            return "main_menu_to_deck_select";
        case NarrativeType::DeckSelectToMapExplore:
            return "deck_select_to_map_explore";
        case NarrativeType::MapExploreToBattle:
            return "map_explore_to_battle";
        case NarrativeType::BattleToMapExplore:
            return "battle_to_map_explore";
        case NarrativeType::MapExploreToNode:
            return "map_explore_to_node";
        case NarrativeType::NodeToMapExplore:
            return "node_to_map_explore";
        case NarrativeType::BattleToVictory:
            return "battle_to_victory";
        case NarrativeType::BattleToMemoryRepair:
            return "battle_to_memory_repair";
        case NarrativeType::Custom:
            return "custom";
        default:
            return "unknown";
    }
}
