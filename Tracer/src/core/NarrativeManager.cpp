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
        // 地图探索到各个节点
        case NarrativeType::MapExploreToHeritage:
            return "map_explore_to_heritage";
        case NarrativeType::MapExploreToEngrave:
            return "map_explore_to_engrave";
        case NarrativeType::MapExploreToInkWorkshop:
            return "map_explore_to_ink_workshop";
        case NarrativeType::MapExploreToInkGhost:
            return "map_explore_to_ink_ghost";
        case NarrativeType::MapExploreToBarter:
            return "map_explore_to_barter";
        case NarrativeType::MapExploreToMemoryRepair:
            return "map_explore_to_memory_repair";
        case NarrativeType::MapExploreToRelicPickup:
            return "map_explore_to_relic_pickup";
        case NarrativeType::MapExploreToSeeker:
            return "map_explore_to_seeker";
        case NarrativeType::MapExploreToTemper:
            return "map_explore_to_temper";
        case NarrativeType::MapExploreToBurn:
            return "map_explore_to_burn";
        case NarrativeType::MapExploreToCombine:
            return "map_explore_to_combine";
        case NarrativeType::MapExploreToInkShop:
            return "map_explore_to_ink_shop";
        // 各个节点到地图探索
        case NarrativeType::HeritageToMapExplore:
            return "heritage_to_map_explore";
        case NarrativeType::EngraveToMapExplore:
            return "engrave_to_map_explore";
        case NarrativeType::InkWorkshopToMapExplore:
            return "ink_workshop_to_map_explore";
        case NarrativeType::InkGhostToMapExplore:
            return "ink_ghost_to_map_explore";
        case NarrativeType::BarterToMapExplore:
            return "barter_to_map_explore";
        case NarrativeType::MemoryRepairToMapExplore:
            return "memory_repair_to_map_explore";
        case NarrativeType::RelicPickupToMapExplore:
            return "relic_pickup_to_map_explore";
        case NarrativeType::SeekerToMapExplore:
            return "seeker_to_map_explore";
        case NarrativeType::TemperToMapExplore:
            return "temper_to_map_explore";
        case NarrativeType::BurnToMapExplore:
            return "burn_to_map_explore";
        case NarrativeType::CombineToMapExplore:
            return "combine_to_map_explore";
        case NarrativeType::InkShopToMapExplore:
            return "ink_shop_to_map_explore";
        case NarrativeType::FinalBossVictory:
            return "final_boss_victory";
        case NarrativeType::Custom:
            return "custom";
        default:
            return "unknown";
    }
}
