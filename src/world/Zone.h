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
    // Free-text "ask about..." subjects, in authored order -- each pairs a
    // comma-separated keyword-alias list with its dialogue text. Empty means
    // no free-text asking is offered for this POI. Set via zero or more
    // SUBJECT lines (see docs/ZONE_NOTES.md's "Ask about anything"), which
    // must reference a POI that also has a TALK line, same rule as TOPIC.
    // game::GameLoop does the actual keyword matching (see
    // game::matchSubject) so this stays a plain data holder, same
    // decoupling reasoning as conditionalDialogue above.
    std::vector<std::pair<std::vector<std::string>, std::string>> subjects;
    // Shown when a free-typed subject matches none of the above -- empty
    // means fall back to game::GameLoop::talkTo's generic line. Set via an
    // optional SUBJECT_UNKNOWN line.
    std::string subjectUnknown;
    // True if the player can press 'b' (shop) while standing on this tile
    // to browse/buy from character::Equipment's catalog -- set via a SHOP
    // line in the zone file, which must reference an already-declared POI
    // char (see docs/ZONE_NOTES.md).
    bool isShop = false;
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
    Zone(std::string name, std::vector<std::string> rows, int entryX, int entryY,
         std::unordered_map<char, PointOfInterest> pois,
         std::unordered_map<char, std::string> portals, char timelineAnchorPoi,
         std::string timelineLocationId, std::unordered_map<char, std::string> quests,
         std::unordered_map<char, BoatVoyage> boatVoyages);

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
};

} // namespace world
