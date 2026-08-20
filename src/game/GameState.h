#pragma once

#include "character/Character.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace game {

enum class Mode {
    Overworld,
    Zone,
};

// A quest's turn-in state. Absence from GameState::quests is the third,
// implicit state ("not started") -- see quest/Quest.h. Stored as a raw
// enum int in the save file (see game::SaveGame), same convention as
// RACE/CLASS/ALIGNMENT, but append-only-safe: unlike those, nothing here
// depends on a fixed ordering, so a future status can be added without
// corrupting old saves. Deliberately just two values rather than a stage
// index -- no authored quest needs more, and the save line widens for free
// if one ever does (see docs/QUEST_NOTES.md).
enum class QuestStatus {
    Active = 0,
    Complete = 1,
};

// Where to resume when leaving a zone that was entered via a PORTAL from
// another zone (e.g. stepping out of the Inn of the Last Home's interior
// back into Solace's town square), rather than from the overworld.
struct ZoneReturnPoint {
    std::string zoneId;
    int x = 0;
    int y = 0;
};

// The player's position and progress in the world. x/y are overworld tile
// coordinates (same space as world::OverworldGrid), always kept valid even
// while inside a zone -- entering a zone doesn't move the overworld
// position, it just changes what's rendered/walked on, so exiting returns
// the player to exactly where they were. zoneX/zoneY/currentZoneId are only
// meaningful when mode == Mode::Zone. hoursElapsed is the single source of
// truth for in-game time -- day/hour are derived from it
// (hoursElapsed/24, hoursElapsed%24) wherever displayed, rather than
// tracked as separate fields, so they can never drift out of sync. Walking
// inside a zone does not advance hoursElapsed (see docs/ARCHITECTURE.md) --
// only overworld travel does.
struct GameState {
    character::Character character; // produced once by CharacterCreator before the loop starts

    Mode mode = Mode::Overworld;
    int x = 0;
    int y = 0;
    long long hoursElapsed = 0;
    std::unordered_set<std::string> visitedLocations;
    // Ids of every canon/NPC character the player has ever talked to (via
    // 't') at least once -- timeline characters use their CanonCharacter
    // id, zone NPCs use "<zoneId>:<POI char>" (they have no id of their
    // own). See game::GameLoop::talkTo.
    std::unordered_set<std::string> metCharacters;
    // True once the player has arranged passage by sea (Milestone 36) --
    // lets tryMoveOverworld cross ocean tiles (world::TerrainInfo::
    // crossableByBoat). Granted as a side effect of talking to a zone POI
    // marked BOAT (see world::PointOfInterest::isBoat), never revoked.
    bool hasBoat = false;
    // Quest id -> current status. See quest::Quest/quest::QuestCatalog for
    // the static quest definitions this indexes into, and
    // game::GameLoop::offerOrTurnInQuest for how it's mutated.
    std::unordered_map<std::string, QuestStatus> quests;
    // Lifetime kills per combat::Monster::id, incremented in
    // GameLoop::runCombat on every kill regardless of any quest. A SLAY
    // objective is a query over this, the same way a VISIT/TALK objective
    // queries visitedLocations/metCharacters above -- see
    // docs/QUEST_NOTES.md. Never decremented.
    std::unordered_map<std::string, int> monsterKills;

    std::string currentZoneId;
    int zoneX = 0;
    int zoneY = 0;
    std::vector<ZoneReturnPoint> zoneStack; // parent zone(s) to pop back to on
                                             // exit -- see ZoneReturnPoint above
};

} // namespace game
