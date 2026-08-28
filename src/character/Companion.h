#pragma once

#include "character/Character.h"

#include <string>

namespace character {

// Two small, hand-authored recruitable party companions -- Milestone 116
// Phase 1 shipped Bren Alder alone; Milestone 118 ("A real multi-companion
// roster," Phase 3a of the party system) adds Dessa Corrin and turns
// GameState::companion (a single slot) into GameState::companions (a real
// roster). Deliberately still not attempted: player-directed control in
// combat (a UIC-style toggle), deployment order, thief backstab, fighter
// sweep -- see docs/COMBAT_NOTES.md's "Extending this later" section.
// Both builders return a fully-formed level-1 Character using FIXED
// ability scores and starting steel (not character::roll) -- deliberately
// deterministic, so game::SaveGame can reconstruct an identical companion
// on load from its id alone instead of serializing every field. See
// docs/ARCHITECTURE.md's "Party companions" section.
Character buildCompanion();     // Bren Alder, Human Fighter -- id "bren_alder"
Character buildDessaCorrin();   // Dessa Corrin, Human Thief -- id "dessa_corrin"

// Builds the companion matching `id` ("bren_alder" or "dessa_corrin").
// Fails fast (throws std::runtime_error) on an unknown id, same idiom as
// every other loader in this project -- callers (game::SaveGame,
// game::GameLoop::talkTo) only ever pass an id already validated at
// startup by main.cpp's cross-check against isKnownCompanionId below.
Character buildCompanionById(const std::string& id);

// True for exactly "bren_alder" and "dessa_corrin" -- used by main.cpp to
// cross-check every zone's RECRUIT id at startup, the same place/pattern
// quest::QuestCatalog ids and shop-lock quest ids are already cross-checked
// (world:: can't see character:: directly, so ZoneLoader itself can't do
// this validation -- see docs/ZONE_NOTES.md's "Recruiting a companion").
bool isKnownCompanionId(const std::string& id);

} // namespace character
