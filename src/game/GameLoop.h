#pragma once

#include "combat/Monster.h"
#include "game/GameState.h"
#include "quest/Quest.h"
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
    // One SUBJECT (or SUBJECT_ENDS) entry: matches if any group in
    // `keywords` is fully satisfied by the player's free-typed "ask
    // about..." input -- a group is satisfied when *every* word in it
    // appears somewhere among the input's tokens (see matchSubject below).
    // A plain single-word keyword (the only kind data/timeline.txt's own
    // SUBJECT grammar produces -- see speechFromWindow) is just a length-1
    // group; multi-word groups (zone-only, `+`-joined in the file, e.g.
    // `you+gilean`) are how a zone SUBJECT_ENDS entry tells "are you
    // Gilean" apart from "tell me about Gilean" despite both containing
    // the word "gilean" -- see docs/ZONE_NOTES.md's "Ask about anything".
    // `endsConversation` (zone-only, set via SUBJECT_ENDS instead of
    // SUBJECT) tells GameLoop::talkTo to end the conversation immediately
    // after showing this entry's text, rather than returning to the ask
    // picker -- always false for a timeline-window Speech.
    struct SubjectEntry {
        std::vector<std::vector<std::string>> keywords;
        std::string text;
        bool endsConversation = false;
    };

    std::string greeting;                                          // plain SAY/TALK
    std::vector<std::pair<std::string, std::string>> conditional;  // SAY_IF (timeline or zone)
    std::string again;                                             // SAY_AGAIN/TALK_AGAIN, may be empty
    std::vector<std::pair<std::string, std::string>> topics;       // TOPIC (timeline or zone)
    std::vector<SubjectEntry> subjects;                            // SUBJECT (timeline or zone)
    std::string subjectUnknown;    // SUBJECT_UNKNOWN override; empty means fall back to a generic line
    // ASK_LIMIT/ASK_LIMIT_REACHED -- zone-only (see speechFromPoi), always
    // 0/empty for a timeline-window Speech. 0 means unlimited free-text
    // questions per day.
    int askLimit = 0;
    std::string askLimitReachedText;
    // Raises askLimit's cap to this many for the rest of the day once the
    // Intelligence+Wisdom check GameLoop::talkTo rolls succeeds -- zone-
    // only, 0 or equal to askLimit means no extension is possible. See
    // world::PointOfInterest::askLimitHardCap.
    int askLimitHardCap = 0;
    std::string askLimitExtendedText;
    // True if the "Ask about something else..." prompt should skip its
    // suggested-keywords line -- zone-only, always false for a timeline-
    // window Speech. See world::PointOfInterest::suppressAskHints.
    bool suppressAskHints = false;
    // Shown instead of TALK/TALK_AGAIN, attributed to askLimitLockedSpeaker
    // rather than this NPC's own name, whenever ASK_LIMIT's cap has already
    // been reached for the day -- zone-only, empty means no override (the
    // ordinary TALK_AGAIN still plays). See
    // world::PointOfInterest::askLimitLockedText.
    std::string askLimitLockedSpeaker;
    std::string askLimitLockedText;
};

// SAY_IF's condition vocabulary -- see docs/TIMELINE_NOTES.md. A free
// function (not a GameLoop member) so it's directly unit-testable without
// constructing a whole GameLoop.
bool conditionMatches(const std::string& condition, const character::Character& character);

// The wayreth_summons turn-in's real Test -- which "law" (good/neutral/
// evil) the player's choice keeps, per Dragonlance Adventures pp.34-35's
// own per-Robe "Minimum Requirements" (each Robe requires passing the
// Test "without having committed an act contrary to the laws of"
// good/neutrality/evil respectively -- a check on conduct during the
// Test, not the character's preset alignment). See docs/QUEST_NOTES.md.
enum class EthicChoice { Good, Neutral, Evil };

// Preserves the Lawful/Neutral/Chaotic axis of `current`, replaces only
// the Good/Neutral/Evil axis with `choice`. A free function (not a
// GameLoop member), same direct-unit-testability reason as
// conditionMatches above.
character::Alignment withEthic(character::Alignment current, EthicChoice choice);

// Lowercases `raw`, strips anything that isn't a letter/digit/hyphen/
// apostrophe, and splits on the remaining whitespace -- the free-typed
// "ask about..." input's tokenization. A free function (not a GameLoop
// member) for the same direct-unit-testability reason as conditionMatches.
std::vector<std::string> tokenizeAskInput(const std::string& raw);

// The first SubjectEntry (authored order, same "first match wins"
// convention as SAY_IF) any of whose keyword groups is fully satisfied by
// words tokenized from `raw` (every word in the group present somewhere
// among the tokens, not necessarily adjacent or in order), or nullptr if
// none match -- see docs/TIMELINE_NOTES.md / docs/ZONE_NOTES.md's "Ask
// about anything".
const Speech::SubjectEntry* matchSubject(const std::vector<Speech::SubjectEntry>& subjects, const std::string& raw);

// How far GameState satisfies one quest::Objective: kills so far for Slay
// (capped display-side, never above Objective::count), else 1 if the
// VISIT/TALK id is already in visitedLocations/metCharacters, else 0. Every
// objective kind is a query over state the game already tracks (see
// quest/Quest.h) -- these are free functions, not GameLoop members, for the
// same "directly unit-testable without constructing a whole GameLoop"
// reason as conditionMatches above.
int objectiveProgress(const quest::Objective& objective, const GameState& state);
// True once objectiveProgress reaches objective.count (1 for Visit/Talk).
bool objectiveMet(const quest::Objective& objective, const GameState& state);
// True once every objective in the quest is met -- deliberately not named
// "questComplete": "objectives satisfied" and "turned in" (GameState::
// quests holding QuestStatus::Complete) are different states, and a name
// conflating them would eventually cause a bug. See offerOrTurnInQuest.
bool allObjectivesMet(const quest::Quest& quest, const GameState& state);

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
    // Non-empty for a zone POI marked BOAT (world::Zone::boatAt) -- talkTo()
    // whisks the player straight to this world::Location id (a scripted
    // one-time voyage, `boatHours` in-game hours long) the first time such a
    // candidate is actually talked to. Always empty for timeline
    // (canon-character) candidates. See Milestone 36 / docs/ZONE_NOTES.md.
    std::string boatDestinationId;
    int boatHours = 0;
    // Non-empty for a zone POI marked QUEST (world::Zone::questAt) -- the
    // quest::Quest id talkTo() offers/updates/turns in via
    // offerOrTurnInQuest. Always empty for timeline candidates, same
    // restriction as boatDestinationId (see docs/QUEST_NOTES.md: the canon
    // Heroes are deliberately never quest givers). See Milestone 51.
    std::string questId;
    // Non-empty for a zone POI marked GRANTS_ITEM (world::PointOfInterest::
    // grantsItemId/grantsItemName) -- talkTo() adds a character::ItemKind::
    // QuestItem to inventory the first time such a candidate is talked to.
    // Always empty for timeline candidates, same restriction as
    // boatDestinationId. See docs/QUEST_NOTES.md's "DELIVER".
    std::string grantsItemId;
    std::string grantsItemName;
    // Non-empty for a zone POI marked TALK_AFTER (world::PointOfInterest::
    // dialogueAfter) once GameLoop::handleTalk has confirmed every canon
    // character scheduled at this zone's effective timeline location has
    // fully moved on (timeline::Timeline::latestDayEnd). Always empty for
    // timeline candidates -- this is zone-NPC-only content, see
    // docs/ZONE_NOTES.md's "Aftermath dialogue" section. talkTo() shows this
    // instead of the ordinary greeting/again text exactly once, tracked via
    // a separate "<id>:after" GameState::metCharacters entry so it fires
    // correctly even if the player already met this NPC before the Heroes'
    // window ever opened.
    std::string dialogueAfter;
    // Non-empty for a zone POI marked TALK_BEFORE (world::PointOfInterest::
    // dialogueBefore) once GameLoop::handleTalk has confirmed every canon
    // character scheduled at this zone's effective timeline location hasn't
    // arrived yet (timeline::Timeline::earliestDayStart). Always empty for
    // timeline candidates -- zone-NPC-only content, same as dialogueAfter;
    // see docs/ZONE_NOTES.md's "Anticipation dialogue" section. Unlike
    // dialogueAfter, talkTo() shows this on *every* visit while the
    // condition holds, not tracked via GameState::metCharacters -- an
    // ongoing truth, not a one-time event, and the condition self-expires
    // once the Heroes actually arrive.
    std::string dialogueBefore;
    // Non-empty for a zone POI marked RECRUIT (world::PointOfInterest::
    // recruitCompanionId) -- talkTo() offers to add this companion id to the
    // party the first time such a candidate is talked to and it isn't
    // already recruited. Always empty for timeline candidates, same
    // restriction as boatDestinationId/questId/grantsItemId above (canon
    // Heroes are never recruitable -- see docs/QUEST_NOTES.md's equivalent
    // note on quest givers). Milestone 118 gave this a real id payload; it
    // was a plain bool through Milestone 116/117, since only one companion
    // existed.
    std::string recruitCompanionId;
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
    // combat::MonsterCatalog and docs/COMBAT_NOTES.md. `quests` is the
    // roster quest-giver POIs offer from -- see quest::QuestCatalog and
    // docs/QUEST_NOTES.md.
    GameLoop(const world::World& world, const world::OverworldGrid& grid,
              const world::ZoneCatalog& zones, const timeline::Timeline& timeline,
              const combat::MonsterCatalog& monsters, const quest::QuestCatalog& quests,
              GameState initialState, std::string savePath);

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
    // Look ('l'). As of Milestone 43, first checks for any NPC present at
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
    // dialogue/topic logic lives in exactly one place. `boatDestinationId`
    // (non-empty only for a zone POI marked BOAT, see world::Zone::boatAt)
    // moves the player straight to that world::Location the first time such
    // a candidate is talked to, ending the conversation immediately --
    // Milestone 36's sea-travel mechanic, reworked to a scripted one-time
    // voyage rather than a standing ability, see docs/ZONE_NOTES.md.
    // `dialogueAfter`, when non-empty, is shown instead of the ordinary
    // greeting/again text exactly once (tracked via a separate "<id>:after"
    // metCharacters entry, checked before the ordinary alreadyMet branch so
    // it fires regardless of prior "met" state) -- see docs/ZONE_NOTES.md's
    // "Aftermath dialogue" section.
    void talkTo(const TalkCandidate& candidate);
    // Offers, updates, or turns in `questId` as part of talking to
    // `speakerName` -- called from talkTo() when candidate.questId is
    // non-empty, after metCharacters.insert but before the topic-picker
    // loop (the same "talking mutates state" slot the boatDestinationId
    // precedent established). See docs/QUEST_NOTES.md for the full not-started /
    // active-unmet / active-met / complete state table.
    void offerOrTurnInQuest(const std::string& questId, const std::string& speakerName);
    // Checked after every state change an objective can key off of --
    // visiting a new overworld location, talking to someone, killing a
    // monster, or accepting a quest that was already satisfiable -- rather
    // than a generic subscriber/event-bus system (deliberately not built,
    // see docs/ARCHITECTURE.md): there are only ever these four call
    // sites, and it's cheap even if the check finds nothing every time.
    // Promotes any still-Active quest whose objectives are all now met to
    // QuestStatus::ReadyToTurnIn and pushes a one-time "ready to turn in"
    // log line -- ReadyToTurnIn's very purpose is to make this a one-shot
    // notification rather than repeating on every subsequent kill/visit/
    // talk. See docs/QUEST_NOTES.md.
    void checkQuestReadiness();
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
    // Full-screen quest journal ('g') -- lists every quest in
    // GameState::quests (active, then completed), each objective's
    // progress via objectiveProgress. Same one-keypress-block shape as
    // showCharacterSheet/showHelp, not a nested loop -- no scrolling in v1
    // (see docs/QUEST_NOTES.md).
    void showJournal();
    // Rest ('r') -- once per in-game day (Character::lastRestDay), advances
    // hoursElapsed by 8 (an overnight rest), heals 1 hp (DMG p.74's base
    // natural-healing rate, capped at maxHp), and -- for a real caster
    // (character::maxAccessibleSpellLevel > 0) -- memorizes today's spells
    // via performSpellMemorization/character::memorizeSpells. See
    // docs/CHARACTER_NOTES.md's "Rest and spell memorization".
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
    // Shared by handleRest/handleBedRest: for a real caster, either
    // re-memorizes Character::preferredSpellIds as-is (the default, and the
    // only option the very first time this character has ever memorized
    // anything) or -- if the player says no to "keep the same spells
    // memorized?" -- calls chooseSpellLoadout for a fresh one. Returns the
    // flavor-text suffix to append to the rest message ("You re-memorize
    // ..." / "You spend a quiet hour selecting new spells to memorize.").
    // See character::memorizeSpells and docs/CHARACTER_NOTES.md's
    // "Spellcasting" section.
    std::string performSpellMemorization(long long dayAfterRest);
    // Walks a drawPickerFrame loop once per prepared slot, grouped by spell
    // level (lowest first) -- one pick per slot, no cancel mid-flow (the
    // player already chose "no" to keeping the existing loadout to get
    // here). Only offers levels/spells character::spellSlotsPerDay/
    // spellListFor actually make available. Updates
    // Character::preferredSpellIds as the new standing loadout.
    void chooseSpellLoadout();
    // Character sheet ('c'). Loops so a caster can drill into showSpellbook
    // ('s', locally reinterpreted from render::Key::South -- same "local
    // key reinterpretation instead of a new Key value" trick handleShop/
    // runCombat already use for Inventory) and return to the sheet
    // afterward; any other key dismisses, same as before this existed.
    void showCharacterSheet();
    // The character's class's full spell roster, grouped by level -- the
    // detail the sheet's own terse "Spells memorized: ..." line leaves
    // out. Only reachable for a caster (see showCharacterSheet above).
    // Same one-keypress-blocks shape as showCharacterSheet/showHelp.
    void showSpellbook();
    // Opens the '?' help screen listing every command, until one keypress
    // dismisses it -- same one-keypress-blocks shape as showCharacterSheet.
    void showHelp();
    // Opens the 'o' World Map overview screen until one keypress dismisses
    // it -- same one-keypress-blocks shape as showHelp. Read-only: offers
    // no travel/fast-travel action, see render::MapRenderer::drawWorldMapFrame.
    void showWorldMap();
    // Nearest world::Location with isTown or seaLocked set, by straight-line
    // tile distance from state_.x/state_.y (no pathfinding system exists --
    // same restraint as minutesToCross being flat-per-tile). Used by
    // runCombat's knockout handling to send the player to the closest safe
    // place to wake up rather than always Solace -- see docs/COMBAT_NOTES.md.
    // seaLocked locations (e.g. Ice Wall Castle) count too, even though
    // they're not civilian settlements: they're reachable only by a one-time
    // BOAT voyage, so treating them as town-only would strand a
    // knocked-out player somewhere with no way back. Falls back to "solace"
    // if nothing is found (defensive only; can't happen with the current
    // data).
    const world::Location* nearestRefuge() const;
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
    const quest::QuestCatalog& quests_;
    GameState state_;
    std::string savePath_;
    // Persistent scrolling event log (movement-blocked messages, look
    // results, enter/exit lines, combat continuity) -- rendered as
    // MapRenderer's right-hand log panel. Never saved/loaded; purely
    // in-session UI state. See docs/ARCHITECTURE.md.
    std::vector<std::string> log_;
};

} // namespace game
