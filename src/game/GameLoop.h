#pragma once

#include "combat/Monster.h"
#include "game/GameState.h"
#include "timeline/Timeline.h"
#include "world/OverworldGrid.h"
#include "world/World.h"
#include "world/ZoneCatalog.h"

#include <string>
#include <utility>
#include <vector>

namespace game {

// A talk interaction's content, adapted from either a
// timeline::PresenceWindow or a world::PointOfInterest so
// GameLoop::talkTo has one shape to work with regardless of source --
// see docs/TIMELINE_NOTES.md / docs/ZONE_NOTES.md for the underlying
// SAY/SAY_IF/SAY_AGAIN/TOPIC and TALK/TALK_AGAIN grammar.
struct Speech {
    std::string greeting;                                          // plain SAY/TALK
    std::vector<std::pair<std::string, std::string>> conditional;  // SAY_IF (timeline or zone)
    std::string again;                                             // SAY_AGAIN/TALK_AGAIN, may be empty
    std::vector<std::pair<std::string, std::string>> topics;       // TOPIC (timeline or zone)
};

// SAY_IF's condition vocabulary -- see docs/TIMELINE_NOTES.md. A free
// function (not a GameLoop member) so it's directly unit-testable without
// constructing a whole GameLoop.
bool conditionMatches(const std::string& condition, const character::Character& character);

// One talkable candidate at the player's current tile -- either a zone
// POI's own dialogue, or a canon character the timeline places there
// today (overworld, or a zone's TIMELINE_ANCHOR tile -- see
// docs/TIMELINE_NOTES.md). `id` is what GameState::metCharacters tracks:
// a canon character's own stable id (e.g. "tanis"), or a zone-synthesized
// "<zoneId>:<char>" for a zone-native NPC -- see GameLoop::handleTalk.
struct TalkCandidate {
    std::string id;
    std::string name;
    Speech speech;
    // True for a zone POI marked BOAT (world::PointOfInterest::isBoat) --
    // talkTo() grants GameState::hasBoat the first time such a candidate is
    // actually talked to. Always false for timeline (canon-character)
    // candidates. See Milestone 36 / docs/ZONE_NOTES.md.
    bool grantsBoat = false;
};

// A look-at-someone candidate -- read-only counterpart to TalkCandidate (no
// id/Speech: Look never mutates GameState::metCharacters or grants
// anything). `description` is exactly the text that no longer auto-prints
// to the passive log for NPCs as of Milestone 43 (see
// GameLoop::announceOverworldTile/announceZoneTile) -- Look is how a
// player recovers it on demand.
struct LookCandidate {
    std::string name;
    std::string description;
};

// Owns the render -> read-key -> update cycle. Movement is dispatched
// directly from render::Key -- there is no verb/command parser (see
// docs/ARCHITECTURE.md). As of Milestone 3, the loop has two modes
// (game::Mode) -- overworld and zone (interior) -- and routes movement/look
// to whichever is active; GameState.mode is the single source of truth for
// which one that is.
class GameLoop {
public:
    // `savePath` is where GameState autosaves after every processed
    // keypress (see run()) -- see game::SaveGame and docs/ARCHITECTURE.md.
    // `timeline` is the canon-character schedule queried while rendering
    // the overworld -- see timeline::Timeline and docs/TIMELINE_NOTES.md.
    // `monsters` is the roster random encounters draw from -- see
    // combat::MonsterCatalog and docs/COMBAT_NOTES.md.
    GameLoop(const world::World& world, const world::OverworldGrid& grid,
              const world::ZoneCatalog& zones, const timeline::Timeline& timeline,
              const combat::MonsterCatalog& monsters, GameState initialState, std::string savePath);

    void run();

private:
    void tryMoveOverworld(int dx, int dy);
    void tryMoveZone(int dx, int dy);
    // Pushes the current tile's "standing here" content to log_ -- called
    // once per arrival (movement, mode transitions, and the very first
    // frame), not every render, so it behaves like any other logged event
    // rather than an always-redrawn status block. See docs/ARCHITECTURE.md.
    // Deliberately silent on plain terrain / an empty zone tile -- the map
    // glyph already shows what's there, and logging every wilderness step
    // would flood the panel. As of Milestone 43, an NPC (a present canon
    // character, or a zone POI with non-empty dialogue) only gets a
    // one-line "<Name> is here." -- their full description/flavor text no
    // longer auto-prints; see lookOverworld/lookZone below for where it
    // moved. A Location's own heading/description and a scenery POI's
    // description are unaffected.
    void announceOverworldTile();
    void announceZoneTile();
    // Look (';'). As of Milestone 43, first checks for any NPC present at
    // the player's current tile (a canon character via timeline::Timeline,
    // or -- zone only -- a talkable POI/the zone's TIMELINE_ANCHOR
    // presence) and, if any, hands off to pickAndLook to show their
    // description. Only when nobody's present does this fall back to the
    // original per-mode behavior: lookOverworld reports the compass
    // direction of the nearest off-screen Location; lookZone (a zone has
    // no camera, so nothing is ever off-screen) just says so.
    void lookOverworld();
    void lookZone();
    void handleEnter();
    // Talk to whoever's on the player's current tile -- a present canon
    // character on the overworld (see timeline::Timeline), a zone's own
    // talkable POI, and/or (as of Milestone 23) a canon character present
    // at a zone's TIMELINE_ANCHOR tile. Builds a TalkCandidate list from
    // whichever apply, then hands off to pickAndTalk. See
    // docs/TIMELINE_NOTES.md / docs/ZONE_NOTES.md.
    void handleTalk();
    // Shared by every handleTalk path: 0 candidates -> "nothing to talk
    // to" message; 1 -> talk directly; 2+ -> a drawPickerFrame loop asking
    // which one (same nested-loop, locally-reinterpreted-Key shape as
    // handleShop/talkTo's own topic-picker loop).
    void pickAndTalk(const std::vector<TalkCandidate>& candidates);
    // Shows one character's dialogue -- their real authored greeting (or
    // the first matching SAY_IF condition, see conditionMatches below)
    // the first time `id` is talked to, `speech.again` (or a generic
    // recognition line if empty) every time after (see
    // GameState::metCharacters), then a topic-picker loop if `speech` has
    // any. Shared by every talk path (zone POI, single-character
    // overworld, post-picker overworld) so "have I met them"/reactive-
    // dialogue/topic logic lives in exactly one place. `grantsBoat` (true
    // only for a zone POI marked BOAT, see world::PointOfInterest::isBoat)
    // sets GameState::hasBoat the first time such a candidate is talked to
    // -- Milestone 36's sea-travel mechanic, see docs/ZONE_NOTES.md.
    void talkTo(const std::string& id, const std::string& name, const Speech& speech, bool grantsBoat = false);
    // Shared by lookOverworld/lookZone once they've gathered who's
    // present, always called with a non-empty list: 1 candidate -> shows
    // their description directly; 2+ -> a drawPickerFrame loop asking
    // which one first (same nested-loop, locally-reinterpreted-Key shape
    // as pickAndTalk above).
    void pickAndLook(const std::vector<LookCandidate>& candidates);
    // Browse/buy at the shop POI the player is standing on (zones only) --
    // see character/Equipment.h. Takes over input in its own nested loop,
    // same architectural shape as runCombat, until the player leaves.
    void handleShop();
    // Carried-items screen -- always available (not gated on standing at a
    // shop POI, unlike handleShop, since gear is on the player's person
    // regardless of location). Same nested-loop shape as handleShop;
    // Enter calls character::equipInventoryItem on the selected item.
    void handleInventory();
    // Dedicated scrollable full-history view of log_ ('v') -- the live
    // side panel MapRenderer draws every frame only ever shows a tail, so
    // this is the one place the player can scroll back through everything
    // ever logged. Same nested-loop shape as handleShop/handleInventory,
    // reinterpreting North/South locally as "scroll" -- see
    // docs/ARCHITECTURE.md.
    void handleLog();
    // Rest ('r') -- once per in-game day (Character::lastRestDay), advances
    // hoursElapsed by 8 (an overnight rest), heals 1 hp (DMG p.74's base
    // natural-healing rate, capped at maxHp), and -- for a real caster
    // (character::maxSpellSlotsPerDay > 0) -- memorizes today's spells (see
    // character::memorizeSpells). See docs/CHARACTER_NOTES.md's "Rest and
    // spell memorization".
    void handleRest();
    // Bed Rest ('z') -- like Rest, gated to once per in-game day via
    // Character::lastRestDay (the two share the same gate: one overnight
    // action per day, whichever kind), but only usable standing on a zone
    // POI marked BED (world::PointOfInterest::isBed). Advances hoursElapsed
    // by 8, same as Rest, but heals fully to maxHp instead of 1 hp -- a
    // deliberate simplification of the DMG's literal "complete bed-rest"
    // tier (3 hp/day + a weekly Constitution bonus, DMG p.74) chosen for
    // this project's timeline-driven pace. See docs/CHARACTER_NOTES.md and
    // docs/ZONE_NOTES.md's "Beds" section.
    void handleBedRest();
    void showCharacterSheet();
    // Opens the '?' help screen listing every command, until one keypress
    // dismisses it -- same one-keypress-blocks shape as showCharacterSheet.
    void showHelp();
    // Nearest world::Location with isTown set, by straight-line tile
    // distance from state_.x/state_.y (no pathfinding system exists --
    // same restraint as hoursToCross being flat-per-tile). Used by
    // runCombat's knockout handling to send the player to the closest
    // civilian settlement rather than always Solace -- see
    // docs/COMBAT_NOTES.md. Falls back to "solace" if no town is found
    // (defensive only; can't happen with the current data).
    const world::Location* nearestTown() const;
    // Takes over rendering/input in its own loop until the fight ends
    // (victory, flee, or the player is knocked out) -- see
    // docs/ARCHITECTURE.md and docs/COMBAT_NOTES.md for why this is a
    // nested loop rather than a new GameState::mode.
    void runCombat(const combat::Monster& monster);
    // Appends one entry to log_ (the scrolling event log MapRenderer shows
    // in its side panel -- see docs/ARCHITECTURE.md), capping its size so a
    // long session doesn't grow the vector unbounded.
    void pushLog(std::string text);

    const world::World& world_;
    const world::OverworldGrid& grid_;
    const world::ZoneCatalog& zones_;
    const timeline::Timeline& timeline_;
    const combat::MonsterCatalog& monsters_;
    GameState state_;
    std::string savePath_;
    // Persistent scrolling event log (movement-blocked messages, look
    // results, enter/exit lines, combat continuity) -- rendered as
    // MapRenderer's right-hand log panel. Never saved/loaded; purely
    // in-session UI state. See docs/ARCHITECTURE.md.
    std::vector<std::string> log_;
};

} // namespace game
