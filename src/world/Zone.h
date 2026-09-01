#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace world {

// A one-time scripted sea voyage granted by talking to a POI marked BOAT
// (Milestone 36's sea-travel mechanic, reworked to a scripted trip rather
// than a permanent flag -- see docs/ARCHITECTURE.md). Carries an id payload
// that needs cross-file validation (the destination must be a real
// world::Location), so -- like PORTAL's target zone id, or QUEST's quest id
// -- it lives in its own Zone-level map, not on PointOfInterest, and is
// validated where a loaded World exists: ZoneCatalog::loadForWorld.
struct BoatVoyage {
    std::string destinationLocationId;
    int hours = 0; // in-game hours the voyage advances GameState::hoursElapsed
                    // by -- invented for pacing, not sourced (see
                    // docs/ARCHITECTURE.md).
};

// A single point of interest inside a zone -- a specific tile that shows a
// description when the player stands on it, the overworld-Location
// equivalent for interior scenes.
struct PointOfInterest {
    char code = '?';
    std::string name;
    std::string description;
    // Spoken line shown when the player presses 't' (talk) while standing
    // on this tile -- empty means this POI is just scenery, not an NPC.
    // Set via a TALK line in the zone file (see docs/ZONE_NOTES.md), which
    // must reference an already-declared POI char.
    std::string dialogue;
    // Reactive variants of `dialogue`, checked in authored order against
    // the player's character (race/class/alignment) the first time this
    // POI is talked to -- the first matching condition wins over the plain
    // `dialogue`. Set via zero or more SAY_IF lines (see
    // docs/ZONE_NOTES.md), which must reference a POI that also has a
    // TALK line. Same condition vocabulary as timeline::PresenceWindow,
    // evaluated by game::conditionMatches (not here -- world stays
    // decoupled from character::Character, same reasoning as Timeline.h).
    std::vector<std::pair<std::string, std::string>> conditionalDialogue;
    // Shown instead of the generic recognition fallback on every talk
    // after the first -- empty means fall back to that generic line. Set
    // via an optional TALK_AGAIN line (see docs/ZONE_NOTES.md).
    std::string dialogueAgain;
    // Shown instead of `dialogue`/`dialogueAgain` the first time this POI is
    // talked to once every canon character scheduled at this zone's
    // effective timeline location has fully moved on (checked against
    // timeline::Timeline::latestDayEnd, independent of whether this POI has
    // ever been talked to before -- see GameLoop::talkTo). Empty means this
    // POI has no aftermath reaction. Set via an optional TALK_AFTER line
    // (see docs/ZONE_NOTES.md's "Aftermath dialogue" section), which must
    // reference a POI that already has a TALK line, same rule as SAY_IF/
    // TOPIC.
    std::string dialogueAfter;
    // Shown instead of `dialogue`/`dialogueAgain` while every canon
    // character scheduled at this zone's effective timeline location hasn't
    // arrived yet (checked against timeline::Timeline::earliestDayStart --
    // see GameLoop::handleTalk). Empty means this POI has no anticipation
    // reaction. Unlike dialogueAfter, this is shown on *every* visit while
    // the condition holds, not tracked via GameState::metCharacters -- it's
    // an ongoing truth ("they still aren't here"), not a one-time event, and
    // the condition itself stops firing on its own once they arrive. Set via
    // an optional TALK_BEFORE line (see docs/ZONE_NOTES.md's "Anticipation
    // dialogue" section), which must reference a POI that already has a
    // TALK line, same rule as TALK_AFTER.
    std::string dialogueBefore;
    // Topics offered after the greeting, in authored order -- empty means
    // no topic-picker menu. Set via zero or more TOPIC lines (see
    // docs/ZONE_NOTES.md), which must reference a POI that also has a
    // TALK line.
    std::vector<std::pair<std::string, std::string>> topics;
    // Free-text "ask about..." subjects, in authored order -- each tuple is
    // (keyword groups, dialogue text, endsConversation, dayStart, dayEnd).
    // Each "keyword group" is one OR'd alternative (comma-separated in the
    // file); a group with more than one word (`+`-joined in the file, e.g.
    // `you+gilean`) matches only when *every* word in it appears somewhere
    // in the player's typed input, not just one -- see game::matchSubject
    // and docs/ZONE_NOTES.md's "Ask about anything". A plain single-word
    // keyword is just a length-1 group. Empty means no free-text asking is
    // offered for this POI. Set via zero or more SUBJECT lines (dayStart=0,
    // dayEnd=-1, i.e. always available), which must reference a POI that
    // also has a TALK line, same rule as TOPIC; via SUBJECT_ENDS (identical
    // shape, endsConversation forced true) -- GameLoop::talkTo ends the
    // conversation immediately after showing an endsConversation entry's
    // text, rather than returning to the ask picker; or via SUBJECT_WHEN
    // (identical shape, explicit dayStart/dayEnd -- dayEnd of -1 means
    // open-ended, same sentinel timeline::CharacterSubject uses) for a
    // subject that should only be offered during a specific in-game day
    // range, e.g. a before/after pair sharing one keyword list across two
    // disjoint ranges so an NPC's answer changes once a dated story beat
    // has actually happened -- see docs/ZONE_NOTES.md's "Ask about
    // anything" and GameLoop::speechFromPoi, which filters this list by
    // the current day before anything reaches the picker. game::GameLoop
    // does the actual keyword matching so this stays a plain data holder,
    // same decoupling reasoning as conditionalDialogue above.
    std::vector<std::tuple<std::vector<std::vector<std::string>>, std::string, bool, int, int>> subjects;
    // Shown when a free-typed subject matches none of the above -- empty
    // means fall back to game::GameLoop::talkTo's generic line. Set via an
    // optional SUBJECT_UNKNOWN line.
    std::string subjectUnknown;
    // Caps free-text "ask about..." questions at this many per in-game day
    // -- 0 (the default) means unlimited. Set via an optional ASK_LIMIT
    // line (see docs/ZONE_NOTES.md's "Ask about anything"), which must
    // reference a POI that also has a TALK line and a matching
    // ASK_LIMIT_REACHED line, same pairing ZoneLoader enforces for BOAT's
    // destination/hours. First (and so far only) use: Astinus of Palanthas,
    // who cuts a conversation short once his patience runs out.
    int askLimit = 0;
    // Shown the moment askLimit (or, once extended, askLimitHardCap)
    // questions have been answered for the day, ending the conversation
    // immediately after -- set via an optional ASK_LIMIT_REACHED line.
    // Required (and only meaningful) when askLimit is nonzero; ZoneLoader
    // fails fast if the two don't appear together.
    std::string askLimitReachedText;
    // Raises askLimit's per-day cap to this many if the player's character
    // passes the check GameLoop::talkTo rolls at the moment askLimit would
    // otherwise end the conversation -- 0 or equal to askLimit means no
    // extension is possible (the default). Set via ASK_LIMIT's optional
    // second number; see docs/ZONE_NOTES.md's "Ask about anything" for the
    // Intelligence-and-Wisdom check this project layers on top of the real
    // PHB proficiency-check formula (this specific two-ability combination
    // is this project's own homebrew, not itself a printed 2e mechanic).
    int askLimitHardCap = 0;
    // Shown once, in place of askLimitReachedText, the moment the check
    // above succeeds -- set via an optional ASK_LIMIT_EXTENDED line,
    // required (and only meaningful) when askLimitHardCap > askLimit;
    // ZoneLoader fails fast if the two don't appear together.
    std::string askLimitExtendedText;
    // True if the "Ask about something else..." prompt should skip its
    // "You could ask about: ..." suggestion line entirely -- set via an
    // optional ASK_ANYTHING line, which must reference a POI that also has
    // a TALK line and at least one SUBJECT. For a POI whose SUBJECT pool
    // is meant to feel like it covers everything (see docs/ZONE_NOTES.md's
    // "Ask about anything"), listing every keyword defeats the point --
    // the player should feel free to ask about anything, not be steered
    // toward a curated shortlist. First (and so far only) use: Astinus of
    // Palanthas, whose 47-entry pool is exactly the case this exists for.
    bool suppressAskHints = false;
    // Shown instead of TALK/TALK_AGAIN (checked every visit, an ongoing
    // truth like dialogueBefore rather than a one-time event like
    // dialogueAfter) whenever ASK_LIMIT's cap has already been reached for
    // the day -- set via an optional ASK_LIMIT_LOCKED line, which requires
    // askLimit to be nonzero (meaningless otherwise) and, unlike every
    // other dialogue field on PointOfInterest, carries its own speaker
    // name rather than being attributed to this POI's own name: the point
    // is that a *different* character (e.g. one of Astinus's Aesthetics)
    // turns the player away without Astinus himself needing to reappear.
    // Omitting this line preserves the original behavior -- the ordinary
    // TALK_AGAIN still plays, and the "Ask about something else..." option
    // simply doesn't appear.
    std::string askLimitLockedSpeaker;
    std::string askLimitLockedText;
    // True if the player can press 'b' (shop) while standing on this tile
    // to browse/buy from character::Equipment's catalog -- set via a SHOP
    // line in the zone file, which must reference an already-declared POI
    // char (see docs/ZONE_NOTES.md).
    bool isShop = false;
    // Which character::ShopCatalog this shop offers, stored as the plain
    // name string from the zone file's optional SHOP <char> <catalog>
    // second token ("general" if omitted) -- world:: never references
    // character::ShopCatalog itself (stays decoupled from character::,
    // see docs/ARCHITECTURE.md); game::GameLoop::handleShop translates
    // the string right before calling into character::Equipment. Only
    // meaningful when isShop is true. Validated against the fixed set of
    // known catalog names by ZoneLoader itself -- no cross-file lookup
    // needed, unlike shopLockQuestId below (see docs/ZONE_NOTES.md).
    std::string shopCatalog = "general";
    // True if the player can press 'z' (bed rest) while standing on this
    // tile to fully heal and advance 8 hours -- set via a BED line in the
    // zone file, which must reference an already-declared POI char, same
    // "no TALK prerequisite" rule as SHOP (see docs/ZONE_NOTES.md).
    bool isBed = false;
    // Non-empty if talking to this POI grants a character::ItemKind::
    // QuestItem the first time -- set via a GRANTS_ITEM line in the zone
    // file (grantsItemId + grantsItemName, e.g. "raw_tharkadan_ore" /
    // "Raw Tharkadan Ore"), same "must reference an already-declared POI
    // with a TALK line" validation as BOAT (see docs/ZONE_NOTES.md and
    // docs/QUEST_NOTES.md's "DELIVER").
    std::string grantsItemId;
    std::string grantsItemName;
    // Non-empty if talking to this POI offers to recruit a party companion
    // (accept/decline, same "Board"/"Not yet" picker shape as BOAT) -- the
    // companion id to recruit, e.g. "bren_alder". Set via a RECRUIT line,
    // which must reference an already-declared POI that also has a TALK
    // line (see docs/ZONE_NOTES.md). Same "id payload needing cross-file
    // validation" shape as grantsItemId -- whether the id is a real
    // character:: companion is checked later, in main.cpp (Milestone 118;
    // Milestone 116 Phase 1 had no id at all, since only one companion
    // existed).
    std::string recruitCompanionId;
};

// A loaded walkable interior (e.g. Solace's town square), hand-authored in
// data/zones/<id>.txt -- see docs/ZONE_NOTES.md for the file grammar. Small
// by convention (must fit inside MapRenderer's viewport -- see
// docs/ARCHITECTURE.md): unlike OverworldGrid, Zone has no camera/scrolling.
class Zone {
public:
    // Built by ZoneLoader from a parsed data/zones/<id>.txt file. `rows`
    // must be rectangular (every row the same length) -- ZoneLoader
    // enforces this before constructing. `portals` maps a POI char to the
    // id of another zone it steps into (e.g. an Inn door leading to the
    // Inn's own interior zone) -- see docs/ZONE_NOTES.md. `timelineAnchorPoi`
    // ('\0' for none) and `timelineLocationId` ("" to default to this
    // zone's own catalog id) are optional zone-interior-encounter fields --
    // see docs/TIMELINE_NOTES.md. `quests` maps a POI char to the id of a
    // quest::Quest it offers -- see docs/QUEST_NOTES.md. `boatVoyages` maps
    // a POI char to the scripted sea voyage it grants -- see BoatVoyage above.
    // `shopLocks` maps a shop POI char to the id of a quest::Quest that
    // must be Complete before the shop will open (see docs/ZONE_NOTES.md's
    // SHOP_LOCKED) -- same "id payload needing cross-file validation"
    // shape as quests above, not a plain PointOfInterest bool, since only
    // main.cpp can confirm the quest id is real. `recruits` maps a POI char
    // to the id of the party companion it recruits (Milestone 118) -- same
    // "id payload needing cross-file validation" shape, checked against
    // character::isKnownCompanionId by main.cpp.
    Zone(std::string name, std::vector<std::string> rows, int entryX, int entryY,
         std::unordered_map<char, PointOfInterest> pois,
         std::unordered_map<char, std::string> portals, char timelineAnchorPoi,
         std::string timelineLocationId, std::unordered_map<char, std::string> quests,
         std::unordered_map<char, BoatVoyage> boatVoyages,
         std::unordered_map<char, std::string> shopLocks,
         std::unordered_map<char, std::string> recruits);

    const std::string& name() const { return name_; }
    int width() const { return width_; }
    int height() const { return height_; }
    int entryX() const { return entryX_; }
    int entryY() const { return entryY_; }

    // Returns '#' (wall/impassable) for any out-of-bounds coordinate, so a
    // zone's edges are always a hard boundary without needing walls drawn
    // explicitly around the whole grid.
    char tileCodeAt(int x, int y) const;

    // Returns the point of interest at (x, y), or nullptr if the tile there
    // isn't a POI.
    const PointOfInterest* poiAt(int x, int y) const;

    // Returns the target zone id if (x, y) is a portal tile, or nullptr.
    const std::string* portalAt(int x, int y) const;

    // Every portal declared in this zone, for ZoneCatalog to follow at load
    // time (a zone reached only via a portal has no matching Location, so
    // it isn't found any other way -- see ZoneCatalog::loadForWorld).
    const std::unordered_map<char, std::string>& portals() const { return portals_; }

    // Returns the quest id offered at (x, y), or nullptr if the tile there
    // isn't a quest-giver.
    const std::string* questAt(int x, int y) const;

    // Every quest declared in this zone, keyed by POI char -- for main.cpp
    // to cross-validate each id against the loaded quest::QuestCatalog at
    // startup (ZoneLoader can't see QuestCatalog and shouldn't -- see
    // docs/QUEST_NOTES.md).
    const std::unordered_map<char, std::string>& quests() const { return quests_; }

    // Returns the quest id that must be Complete before (x, y)'s shop will
    // open, or nullptr if that tile isn't a shop-lock POI (either not a
    // shop at all, or a shop with no SHOP_LOCKED line).
    const std::string* shopLockAt(int x, int y) const;

    // Every shop lock declared in this zone, keyed by POI char -- for
    // main.cpp to cross-validate each id against the loaded
    // quest::QuestCatalog at startup, same reasoning as quests() above.
    const std::unordered_map<char, std::string>& shopLocks() const { return shopLocks_; }

    // Every RECRUIT declared in this zone, keyed by POI char -- for
    // main.cpp to cross-validate each companion id against
    // character::isKnownCompanionId at startup, same reasoning as quests()
    // above (Milestone 118).
    const std::unordered_map<char, std::string>& recruits() const { return recruits_; }

    // Returns the boat voyage granted at (x, y), or nullptr if the tile
    // there doesn't grant one.
    const BoatVoyage* boatAt(int x, int y) const;

    // Every boat voyage declared in this zone, keyed by POI char -- for
    // ZoneCatalog::loadForWorld to cross-validate each destination id
    // against the loaded World at startup (ZoneLoader can't see World and
    // shouldn't -- same reasoning as quests() above).
    const std::unordered_map<char, BoatVoyage>& boatVoyages() const { return boatVoyages_; }

    // The POI char where canon-character presence is checked/talkable
    // inside this zone (see docs/TIMELINE_NOTES.md), or '\0' if this zone
    // has none authored.
    char timelineAnchorPoi() const { return timelineAnchorPoi_; }
    // Which timeline location id to check presence for -- empty means "use
    // this zone's own catalog id" (true for every top-level zone, whose
    // filename/catalog id already matches a Location id).
    const std::string& timelineLocationId() const { return timelineLocationId_; }

private:
    std::string name_;
    int width_ = 0;
    int height_ = 0;
    int entryX_ = 0;
    int entryY_ = 0;
    std::vector<std::string> rows_;
    std::unordered_map<char, PointOfInterest> pois_;
    std::unordered_map<char, std::string> portals_;
    char timelineAnchorPoi_ = '\0';
    std::string timelineLocationId_;
    std::unordered_map<char, std::string> quests_;
    std::unordered_map<char, BoatVoyage> boatVoyages_;
    std::unordered_map<char, std::string> shopLocks_;
    std::unordered_map<char, std::string> recruits_;
};

} // namespace world
