#pragma once

#include <string>
#include <vector>

namespace quest {

// What kind of thing a quest objective asks for. Every kind is a *query
// over state the game already persists* rather than a counter that starts
// when the quest is accepted:
//
//   Visit -- game::GameState::visitedLocations (since Milestone 3)
//   Talk  -- game::GameState::metCharacters    (since Milestone 18)
//   Slay  -- game::GameState::monsterKills     (added with this system)
//
// One consequence worth knowing when authoring: a quest can be immediately
// completable the moment it's accepted, if the player already did the thing
// before ever being asked. That is unavoidable for Visit/Talk (those sets
// predate quests entirely), so Slay behaves the same way for consistency
// rather than snapshotting a per-quest baseline. See docs/QUEST_NOTES.md.
enum class ObjectiveKind {
    Visit,
    Talk,
    Slay,
};

struct Objective {
    ObjectiveKind kind = ObjectiveKind::Visit;
    // A world::Location id (Visit), a met-id (Talk), or a combat::Monster id
    // (Slay). Kept as a raw string and resolved in game/ -- quest/ stays
    // decoupled from world/, combat/ and character/, exactly as
    // timeline::PresenceWindow holds raw SAY_IF condition strings without
    // knowing what they mean.
    //
    // A met-id is NOT a display name: canon characters use their own
    // timeline id ("tanis"), zone NPCs use the synthesized
    // "<zoneId>:<POI char>" form ("haven:G") that game::GameLoop::handleTalk
    // builds. QuestLoader can't see zones or the timeline, so a wrong id
    // parses cleanly and then silently never matches -- the same
    // silent-failure class as an unvalidated PRESENCE location id. See
    // docs/GOTCHAS.md.
    std::string targetId;
    // Only meaningful for Slay; Visit/Talk leave it at 1.
    int count = 1;
    // The line shown in the journal, authored rather than derived. Deriving
    // "Slay three timber wolves" would need name lookups across
    // combat::MonsterCatalog / world::World / timeline::Timeline that this
    // module deliberately can't see -- so the label is written out in the
    // data file, the same way PRESENCE carries its own flavor text.
    std::string label;
};

// One hand-authored quest, loaded from data/quests.txt -- see
// docs/QUEST_NOTES.md for the file grammar. Static content: loaded fresh
// every run and never mutated during play (the player's *progress* lives in
// game::GameState::quests, keyed by this id).
struct Quest {
    std::string id;
    std::string name; // journal title

    std::string offerText;    // first talk, before accepting
    std::string acceptText;   // shown on accept
    std::string progressText; // talked to while accepted but unfinished
    std::string completeText; // shown on turn-in

    // Free text naming who/where to return to (e.g. "the Notice Board") --
    // used only in the "ready to turn in" notification/journal note (see
    // game::GameLoop::checkQuestReadiness), never mechanically tied to a
    // real zone POI. Required, not derived: deriving it would mean
    // reverse-searching every zone's QUEST bindings for this id, and nothing
    // stops a quest from having zero or more than one giver POI in the
    // grammar even though every authored quest so far has exactly one.
    std::string giver;

    // Optional gate on who this quest is even offered to, using the same
    // condition vocabulary as SAY_IF (game::conditionMatches). Empty means
    // "offer to everyone". Lets a quest belong to, say, a Knight of
    // Solamnia without needing a separate quest-giver POI per class.
    std::string requirement;

    // At least one, in authored order -- QuestLoader rejects a quest with
    // none. All must be met to turn the quest in.
    std::vector<Objective> objectives;

    int rewardSteel = 0;
    int rewardXp = 0;
    // True if turning this quest in promotes the character to Knight of
    // the Sword (character::KnightOrder::Sword) -- see
    // game::GameLoop::offerOrTurnInQuest and docs/QUEST_NOTES.md. A bare
    // flag, not a reward amount, since a title isn't a quantity.
    bool rewardKnightSword = false;
    // True if turning this quest in grants Solamnic Armor (a
    // character::ArmorId::SolamnicArmor plus an ordinary Shield item, see
    // game::GameLoop::offerOrTurnInQuest) -- same "named, specific,
    // compile-time flag" shape as rewardKnightSword above, not a generic
    // item-reward mapping (see docs/QUEST_NOTES.md's "Deliberately not in
    // v1" for why that's still out of scope).
    bool rewardSolamnicArmor = false;
};

// A loaded roster of quests, static content like timeline::Timeline and
// combat::MonsterCatalog -- loaded fresh every run by QuestLoader, never
// mutated during play.
class QuestCatalog {
public:
    void addQuest(Quest quest);

    // The quest with this id, or nullptr if there is none. Callers in game/
    // treat nullptr defensively even though main.cpp cross-validates every
    // zone's QUEST id against this catalog at startup.
    const Quest* find(const std::string& id) const;

    size_t size() const { return quests_.size(); }

private:
    std::vector<Quest> quests_;
};

} // namespace quest
