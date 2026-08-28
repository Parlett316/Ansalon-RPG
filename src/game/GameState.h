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

// A quest's turn-in state. Absence from GameState::quests is the fourth,
// implicit state ("not started") -- see quest/Quest.h. Stored as a raw
// enum int in the save file (see game::SaveGame), same convention as
// RACE/CLASS/ALIGNMENT, but append-only-safe: unlike those, nothing here
// depends on a fixed ordering, so a future status can be added without
// corrupting old saves -- ReadyToTurnIn was added this way, after Active/
// Complete already shipped (see docs/QUEST_NOTES.md).
enum class QuestStatus {
    Active = 0,
    Complete = 1,
    // Every objective is satisfied but the player hasn't talked to the
    // giver again yet -- game::GameLoop::checkQuestReadiness sets this the
    // moment it happens (right after a visit/talk/kill, or immediately on
    // accepting an already-satisfied quest) and pushes the one-time
    // "ready to turn in" log line; offerOrTurnInQuest reads it back to
    // decide whether talking to the giver dispenses the reward. See
    // docs/QUEST_NOTES.md.
    ReadyToTurnIn = 2,
};

// One recruited party companion. `id` is one of character::
// isKnownCompanionId's known ids ("bren_alder", "dessa_corrin") -- the
// thing game::SaveGame writes to disk and game::GameLoop::talkTo compares
// against to decide whether a given RECRUIT candidate is already in the
// party. `character` is a full character::Character built by character::
// buildCompanionById(id); only its currentHp ever diverges from what that
// call would produce fresh, since buildCompanionById is otherwise pure.
struct RecruitedCompanion {
    std::string id;
    character::Character character;
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
// only overworld travel does. minutesElapsed (below) is a separate sub-hour
// carry fed only by overworld movement -- never itself read for day/hour
// math, so hoursElapsed's role as sole source of truth for those is unchanged.
struct GameState {
    character::Character character; // produced once by CharacterCreator before the loop starts

    Mode mode = Mode::Overworld;
    int x = 0;
    int y = 0;
    long long hoursElapsed = 0;
    // 0-59 -- sub-hour remainder accumulated by world::TerrainInfo::
    // minutesToCross in GameLoop::tryMoveOverworld, rolled into hoursElapsed
    // once it reaches 60 (Milestone 67). Rest/BedRest don't touch this --
    // only overworld movement does.
    int minutesElapsed = 0;
    std::unordered_set<std::string> visitedLocations;
    // Ids of every canon/NPC character the player has ever talked to (via
    // 't') at least once -- timeline characters use their CanonCharacter
    // id, zone NPCs use "<zoneId>:<POI char>" (they have no id of their
    // own). See game::GameLoop::talkTo.
    std::unordered_set<std::string> metCharacters;
    // Ids (same "<zoneId>:<POI char>" shape as metCharacters above) of
    // every world::BoatVoyage a player has actually boarded -- separate
    // from metCharacters because that set is inserted into on every talk,
    // accepted or not, so it can't also gate "have I taken this voyage" or
    // declining would permanently forfeit it. See game::GameLoop::talkTo.
    std::unordered_set<std::string> voyagesTaken;
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

    // The party's recruited companions -- Milestone 116 Phase 1 shipped
    // exactly one slot; Milestone 118 ("A real multi-companion roster")
    // turned it into a real roster. Empty means no companion recruited yet.
    // Recruited via a zone's RECRUIT line (game::GameLoop::talkTo); combat
    // (game::GameLoop::runCombat) mutates each entry's character.currentHp
    // directly, exactly like the player's own. game::SaveGame persists
    // each entry's id + currentHp only, not every field -- see
    // docs/GOTCHAS.md.
    std::vector<RecruitedCompanion> companions;
};

} // namespace game
