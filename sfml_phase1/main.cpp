// Full-migration Phase 1+2+3 -- see docs/CURRENT_WORK.md and the plans this
// was built from. A real, pixel-space overworld + zone-interior + combat
// screen driven by real save data: proves the rendering/collision/round-
// resolution approach end to end. Deliberately standalone rather than
// reusing game::GameLoop::run() -- see this file's CMakeLists.txt comment
// for why. Loads a save file via game::SaveGame and autosaves back to the
// same path after every processed KeyPressed event, unconditionally --
// the same convention GameLoop::run() uses (GameLoop.cpp:408,
// docs/ARCHITECTURE.md's "When it saves"), just applied at this build's
// own per-event granularity instead of console's per-outer-loop-iteration
// one. Never touches the real ansalon_rpg target's code path.
//
// Phase 3 (combat) intentionally ports only the core melee loop -- real
// random encounters, positional movement, target picking, monster/companion
// AI (including each monster's own passive on-turn/on-death specials, which
// cost nothing extra since they're not chooser-driven), flee, victory/
// leveling, and knockout. Spellcasting (M) and item use (I -- see the
// CombatItemKind block below), and Thief backstab/Fighter sweep (automatic,
// no key of their own -- see combatBackstabBonus/combatAdjacentWeakInstances
// above the combat lambdas below), all shipped in later sessions, see
// docs/CURRENT_WORK.md for their own scope writeups and the full picture.

#include "CombatSprite.h"

#include "character/Alignment.h"
#include "character/CharClass.h"
#include "character/Companion.h"
#include "character/Dice.h"
#include "character/Equipment.h"
#include "character/Knighthood.h"
#include "character/Leveling.h"
#include "character/Race.h"
#include "character/Spellcasting.h"
#include "character/WizardOrder.h"
#include "combat/Combat.h"
#include "combat/CombatGrid.h"
#include "combat/Monster.h"
#include "combat/MonsterLoader.h"
#include "game/GameState.h"
#include "game/SaveGame.h"
#include "quest/Quest.h"
#include "quest/QuestLoader.h"
#include "timeline/Timeline.h"
#include "timeline/TimelineLoader.h"
#include "world/BattleMap.h"
#include "world/BattleMapCatalog.h"
#include "world/OverworldGrid.h"
#include "world/Terrain.h"
#include "world/World.h"
#include "world/WorldLoader.h"
#include "world/Zone.h"
#include "world/ZoneCatalog.h"
#include "world/ZoneTile.h"

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/System/Time.hpp>

#ifdef _WIN32
// Only used to reach the real OS window handle for a genuine maximize
// (ShowWindow/SW_MAXIMIZE) -- SFML has no native "maximized" window state.
// NOMINMAX avoids windows.h's min/max macros breaking the std::min/std::max
// calls used throughout this file.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <deque>
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::size_t kMaxLogLines = 14;
constexpr unsigned kSidebarCharSize = 16;
// Placeholder pixel-per-tile scale for zone interiors -- there's no real
// reference image for these like the overworld has, so this is a plain
// colored-tile placeholder, not art. Zones are capped at 44x16
// (docs/ZONE_NOTES.md), so at this scale every zone fits the map viewport
// with no scrolling needed.
constexpr float kZoneTilePx = 20.f;
// Combat's tactical grid. Deliberately bigger than (and now diverging from)
// render::MapRenderer::kCombatGridWidth/Height, the console version's own
// 15x9 -- Milestone 185, at the user's request for a real Gold Box-style
// battlefield (DQoK.pdf's own screens run roughly 50x25). The console's
// ASCII grid physically cannot follow: a 50-cell-wide row blows past
// kProseWrapWidth, which is exactly why 15 was chosen there in the first
// place (see docs/COMBAT_NOTES.md's "Positional combat grid"). ansalon_rpg
// keeps its own 15x9 open-floor grid unchanged; see docs/PARITY_MATRIX.md
// for the recorded divergence. Redefined locally rather than including
// render/MapRenderer.h -- that header pulls in render::Console, which isn't
// linked into this target and shouldn't become a dependency just for two
// ints (this file's presentation code already stays independent of
// render::, same boundary Phase 1/2 established).
//
// Width no longer needs to stay odd the way the console's did (that was
// specifically for GameLoop::runCombat's own symmetric monster-spawn
// math) -- combatStartEncounter's spawn here just centers on
// kCombatGridWidth/2, which is well-defined either way; an even width
// only shifts the exact center column by half a cell, not a real
// gameplay difference.
//
// The whole field no longer fits on screen at a readable tile size, so the
// camera now scrolls (a real per-frame view, not a fixed centered draw) --
// see the combat branch of the main draw loop below, which reuses the
// overworld's own clamped-follow-the-player math.
constexpr int kCombatGridWidth = 50;
constexpr int kCombatGridHeight = 25;
constexpr float kCombatTilePx = 56.f;

// DQoK-style combat HUD (Milestone 199): a full-width bottom command/message
// bar, combat-only -- see combatMapView below for why the map's own viewport
// shrinks to make room for it rather than overlapping. Invented, unverified
// layout number, same "flagged, not sourced" idiom as kAiStepAnimationMs
// below -- sanity-check live once built (the bar has to fit an optional
// status line, a wrapped paced message + its continue prompt, and the Idle
// command row + movement line, several of which can wrap to 2 lines).
constexpr float kCombatBottomBarHeight = 140.f;

// Pluralizes a monster's display name for the group-arrival/victory summary
// lines -- copied verbatim from game::pluralMonsterName (GameLoop.cpp),
// which isn't itself exported (GameLoop.cpp isn't linked into this target).
std::string pluralMonsterName(const std::string& name) {
    if (name == "Timber Wolf") return "Timber Wolves";
    if (name == "Lizard Man") return "Lizard Men";
    return name + "s";
}

// Translates a zone file's plain SHOP catalog name into
// character::ShopCatalog -- copied verbatim from game::shopCatalogFor
// (GameLoop.cpp:32-40), same "not exported, GameLoop.cpp isn't linked
// here" reasoning as pluralMonsterName above.
character::ShopCatalog shopCatalogFor(const std::string& name) {
    if (name == "armory") return character::ShopCatalog::Armory;
    if (name == "market") return character::ShopCatalog::MarketGoods;
    if (name == "salvage") return character::ShopCatalog::Salvage;
    if (name == "bazaar") return character::ShopCatalog::Bazaar;
    if (name == "harbor") return character::ShopCatalog::HarborTrade;
    if (name == "magic") return character::ShopCatalog::Magic;
    return character::ShopCatalog::General;
}

// A talk interaction's content -- local counterpart to game::Speech
// (GameLoop.h), which isn't linkable here (GameLoop.cpp is built on
// render::Console/render::MapRenderer, which this target deliberately
// excludes -- see this file's own top-of-file comment). Full parity with
// game::Speech, including subjects/subjectUnknown/askLimit*/
// suppressAskHints/askLimitLocked* -- free-text "ask about something
// else..." (this phase) needs all of them, see docs/CURRENT_WORK.md.
struct DialogueSpeech {
    // One SUBJECT (or SUBJECT_ENDS) entry -- see game::Speech::SubjectEntry
    // (GameLoop.h) for the full matching-semantics doc comment, copied
    // verbatim in spirit here.
    struct SubjectEntry {
        std::vector<std::vector<std::string>> keywords;
        std::string text;
        bool endsConversation = false;
    };

    std::string greeting;                                          // plain SAY/TALK
    std::vector<std::pair<std::string, std::string>> conditional;  // SAY_IF
    std::string again;                                             // SAY_AGAIN/TALK_AGAIN
    std::vector<std::pair<std::string, std::string>> topics;       // TOPIC
    std::vector<SubjectEntry> subjects;                            // SUBJECT (timeline or zone)
    std::string subjectUnknown;    // SUBJECT_UNKNOWN override; empty means fall back to a generic line
    int askLimit = 0;              // ASK_LIMIT -- zone-only, 0 means unlimited
    std::string askLimitReachedText;
    int askLimitHardCap = 0;       // ASK_LIMIT's Int+Wis-check extension cap -- zone-only
    std::string askLimitExtendedText;
    bool suppressAskHints = false; // zone-only
    std::string askLimitLockedSpeaker;
    std::string askLimitLockedText;
};

// One talkable candidate at the player's current tile -- trimmed local
// counterpart to game::TalkCandidate (GameLoop.h), same "not linkable here"
// reasoning as DialogueSpeech above. questId carries the real quest id (see
// world::Zone::questAt) the same way boatDestinationId/recruitCompanionId
// below carry theirs -- quest turn-in is wired up via questBegin/
// DialogueUiState::Quest* (see dialogueContinue).
struct DialogueCandidate {
    std::string id;
    std::string name;
    DialogueSpeech speech;
    std::string dialogueAfter;
    std::string dialogueBefore;
    std::string grantsItemId;
    std::string grantsItemName;
    std::string questId;
    // Non-empty for a zone POI marked BOAT (world::Zone::boatAt) --
    // dialogueContinue whisks the player straight to this world::Location
    // id (a scripted voyage, boatHours in-game hours long) once "Board" is
    // chosen. See Milestone 36 / docs/ZONE_NOTES.md for the console-side
    // grammar this mirrors.
    std::string boatDestinationId;
    int boatHours = 0;
    // Non-empty for a zone POI marked RECRUIT (world::PointOfInterest::
    // recruitCompanionId) -- dialogueOfferRecruitOrTopics offers to add this
    // companion id to the party once "Join me" is chosen, unless it's
    // already recruited. See Milestone 118 / docs/COMBAT_NOTES.md's
    // "Extending this later" section for the console-side grammar this
    // mirrors.
    std::string recruitCompanionId;
};

// A look-at-someone/something candidate -- trimmed local counterpart to
// game::LookCandidate (GameLoop.h:192-195), same "not linkable here"
// reasoning as DialogueCandidate above. No id/speech: Look never mutates
// state.metCharacters or grants anything, it's read-only.
struct LookCandidate {
    std::string name;
    std::string description;
};

// Coarse 8-point compass direction from a dx/dy pair -- copied verbatim from
// game::compassDirection (GameLoop.cpp:44-51), same "not exported,
// GameLoop.cpp isn't linked here" reasoning as conditionMatches below. Grid y
// grows downward (row 0 is the top), so "north" is negative dy -- easy to get
// backwards, worth calling out same as the original.
const char* compassDirection(int dx, int dy) {
    constexpr double kPi = 3.14159265358979323846;
    double angle = std::atan2(static_cast<double>(-dy), static_cast<double>(dx)); // 0 = east, increases counter-clockwise
    static const char* kDirs[8] = {"east", "northeast", "north", "northwest",
                                    "west", "southwest", "south", "southeast"};
    int index = static_cast<int>(std::lround(angle / (kPi / 4.0))) & 7;
    return kDirs[index];
}

// SAY_IF's condition vocabulary -- copied verbatim from game::conditionMatches
// (GameLoop.cpp), same "not exported, GameLoop.cpp isn't linked here"
// reasoning as pluralMonsterName above. See docs/TIMELINE_NOTES.md.
bool conditionMatches(const std::string& condition, const character::Character& c) {
    using character::Alignment;
    if (condition == "good") {
        return c.alignment == Alignment::LawfulGood || c.alignment == Alignment::NeutralGood ||
               c.alignment == Alignment::ChaoticGood;
    }
    if (condition == "evil") {
        return c.alignment == Alignment::LawfulEvil || c.alignment == Alignment::NeutralEvil ||
               c.alignment == Alignment::ChaoticEvil;
    }
    if (condition == "human") return c.race == character::RaceId::Human;
    if (condition == "dwarf") return c.race == character::RaceId::Dwarf;
    if (condition == "elf") return c.race == character::RaceId::Elf;
    if (condition == "gnome") return c.race == character::RaceId::Gnome;
    if (condition == "halfelf") return c.race == character::RaceId::HalfElf;
    if (condition == "kender") return c.race == character::RaceId::Kender;
    if (condition == "fighter") return c.charClass == character::ClassId::Fighter;
    if (condition == "mage") return c.charClass == character::ClassId::Mage;
    if (condition == "cleric") return c.charClass == character::ClassId::Cleric;
    if (condition == "thief") return c.charClass == character::ClassId::Thief;
    if (condition == "tinker") return c.charClass == character::ClassId::Tinker;
    if (condition == "knight") return c.knightOrder != character::KnightOrder::None;
    if (condition == "sword_knight") return c.knightOrder == character::KnightOrder::Sword;
    if (condition == "sword_eligible") {
        return c.knightOrder == character::KnightOrder::Crown && c.level >= 3 &&
               character::meetsKnightOfSwordRequirements(c.scores);
    }
    if (condition == "rose_eligible") {
        return c.knightOrder == character::KnightOrder::Sword && c.level >= 4 &&
               character::meetsKnightOfRoseRequirements(c.scores);
    }
    if (condition == "str_13") return c.scores.strength >= character::kFrostreaverMinStrength;
    if (condition == "wayreth_eligible") {
        return c.charClass == character::ClassId::Mage && c.level >= 3;
    }
    return false;
}

// The wayreth_summons turn-in's real Test -- which "law" (good/neutral/
// evil) the player's choice keeps -- copied verbatim from game::EthicChoice
// (GameLoop.h:84), same "not linkable here" reasoning as conditionMatches
// above. See docs/QUEST_NOTES.md.
enum class EthicChoice { Good, Neutral, Evil };

// Preserves the Lawful/Neutral/Chaotic axis of `current`, replaces only the
// Good/Neutral/Evil axis with `choice` -- copied verbatim from
// game::withEthic (GameLoop.cpp:252-256). Alignment's declaration order
// (LawfulGood..ChaoticGood, LawfulNeutral..ChaoticNeutral,
// LawfulEvil..ChaoticEvil) makes this exact: index % 3 is the law/chaos
// component, index / 3 is the good/neutral/evil group -- EthicChoice's own
// declaration order (Good, Neutral, Evil) matches that group order, so no
// separate lookup table is needed.
character::Alignment withEthic(character::Alignment current, EthicChoice choice) {
    int lawChaos = static_cast<int>(current) % 3;
    int ethicGroup = static_cast<int>(choice);
    return static_cast<character::Alignment>(ethicGroup * 3 + lawChaos);
}

// How far GameState satisfies one quest::Objective -- copied verbatim from
// game::objectiveProgress/objectiveMet/allObjectivesMet (GameLoop.cpp:
// 317-349), same "not linkable here" reasoning as conditionMatches above.
int objectiveProgress(const quest::Objective& objective, const game::GameState& state) {
    switch (objective.kind) {
        case quest::ObjectiveKind::Visit:
            return state.visitedLocations.count(objective.targetId) > 0 ? 1 : 0;
        case quest::ObjectiveKind::Talk:
            return state.metCharacters.count(objective.targetId) > 0 ? 1 : 0;
        case quest::ObjectiveKind::Slay: {
            auto it = state.monsterKills.find(objective.targetId);
            return it == state.monsterKills.end() ? 0 : it->second;
        }
        case quest::ObjectiveKind::Deliver: {
            int count = 0;
            for (const auto& item : state.character.inventory) {
                if (item.kind == character::ItemKind::QuestItem && item.questItemId == objective.targetId) {
                    ++count;
                }
            }
            return count;
        }
    }
    return 0; // unreachable -- every ObjectiveKind is handled above
}

bool objectiveMet(const quest::Objective& objective, const game::GameState& state) {
    return objectiveProgress(objective, state) >= objective.count;
}

bool allObjectivesMet(const quest::Quest& quest, const game::GameState& state) {
    for (const auto& objective : quest.objectives) {
        if (!objectiveMet(objective, state)) return false;
    }
    return true;
}

// Adapts a timeline::PresenceWindow into DialogueSpeech -- local
// counterpart to game::speechFromWindow (GameLoop.cpp:110-128).
// subjects/subjectUnknown come from Timeline::subjectsFor/
// subjectUnknownFor (Milestone 72) rather than reading `window` directly,
// so a character's day-gated subject pool layers in underneath that
// window's own SUBJECT entries. askLimit*/suppressAskHints/
// askLimitLocked* stay default -- always false/empty for a timeline
// window, same as console.
DialogueSpeech speechFromWindow(const timeline::Timeline& timeline, const timeline::CanonCharacter& character,
                                 const timeline::PresenceWindow& window, int day) {
    DialogueSpeech speech;
    speech.greeting = window.dialogue;
    speech.conditional = window.conditionalDialogue;
    speech.again = window.dialogueAgain;
    speech.topics = window.topics;
    for (const auto& subject : timeline.subjectsFor(character, window, day)) {
        std::vector<std::vector<std::string>> groups;
        for (const std::string& keyword : subject.keywords) groups.push_back({keyword});
        speech.subjects.push_back(DialogueSpeech::SubjectEntry{std::move(groups), subject.text, false});
    }
    speech.subjectUnknown = timeline.subjectUnknownFor(character, window);
    return speech;
}

// Adapts a world::PointOfInterest into DialogueSpeech -- the zone-native
// counterpart to speechFromWindow above, local counterpart to
// game::speechFromPoi (GameLoop.cpp:138-157). `day` filters poi.subjects
// down to entries whose SUBJECT_WHEN day range (or the implicit
// always-available 0/-1 range plain SUBJECT/SUBJECT_ENDS carry) actually
// contains it.
DialogueSpeech speechFromPoi(const world::PointOfInterest& poi, int day) {
    DialogueSpeech speech;
    speech.greeting = poi.dialogue;
    speech.conditional = poi.conditionalDialogue;
    speech.again = poi.dialogueAgain;
    speech.topics = poi.topics;
    for (const auto& [keywords, text, endsConversation, dayStart, dayEnd] : poi.subjects) {
        if (day < dayStart || (dayEnd != -1 && day > dayEnd)) continue;
        speech.subjects.push_back(DialogueSpeech::SubjectEntry{keywords, text, endsConversation});
    }
    speech.subjectUnknown = poi.subjectUnknown;
    speech.askLimit = poi.askLimit;
    speech.askLimitReachedText = poi.askLimitReachedText;
    speech.askLimitHardCap = poi.askLimitHardCap;
    speech.askLimitExtendedText = poi.askLimitExtendedText;
    speech.suppressAskHints = poi.suppressAskHints;
    speech.askLimitLockedSpeaker = poi.askLimitLockedSpeaker;
    speech.askLimitLockedText = poi.askLimitLockedText;
    return speech;
}

// Lowercases `raw`, strips anything that isn't a letter/digit/hyphen/
// apostrophe, and splits on the remaining whitespace -- the free-typed
// "ask about..." input's tokenization. Copied verbatim from
// game::tokenizeAskInput (GameLoop.cpp:265-280), same "not linkable here"
// reasoning as conditionMatches above.
std::vector<std::string> tokenizeAskInput(const std::string& raw) {
    std::vector<std::string> tokens;
    std::string current;
    for (char c : raw) {
        char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        bool isWordChar = std::isalnum(static_cast<unsigned char>(lower)) != 0 || lower == '-' || lower == '\'';
        if (isWordChar) {
            current.push_back(lower);
        } else if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    }
    if (!current.empty()) tokens.push_back(current);
    return tokens;
}

// The first SubjectEntry (authored order, "first match wins") any of whose
// keyword groups is fully satisfied by words tokenized from `raw` -- every
// word in the group present somewhere among the tokens, not necessarily
// adjacent or in order -- or nullptr if none match. Copied verbatim from
// game::matchSubject (GameLoop.cpp:282-315).
const DialogueSpeech::SubjectEntry* matchSubject(const std::vector<DialogueSpeech::SubjectEntry>& subjects,
                                                   const std::string& raw) {
    std::vector<std::string> tokens = tokenizeAskInput(raw);
    for (const auto& subject : subjects) {
        for (const std::vector<std::string>& group : subject.keywords) {
            bool allWordsPresent = true;
            for (const std::string& word : group) {
                std::string lowerWord = word;
                std::transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(),
                               [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                bool wordPresent = false;
                for (const std::string& token : tokens) {
                    if (token == lowerWord) {
                        wordPresent = true;
                        break;
                    }
                }
                if (!wordPresent) {
                    allWordsPresent = false;
                    break;
                }
            }
            if (allWordsPresent) return &subject;
        }
    }
    return nullptr;
}

std::string formatDayTime(long long hoursElapsed) {
    const long long day = hoursElapsed / 24;
    const long long hour = hoursElapsed % 24;
    std::ostringstream oss;
    oss << "Day " << day << ", " << (hour < 10 ? "0" : "") << hour << ":00";
    return oss.str();
}

// Greedy word-wrap so overlay/log text never runs off its panel. Measures
// each candidate line's real rendered width via the actual font/size
// rather than an assumed average character-advance width -- a fixed
// per-character constant (tried first, see git history) drifts from the
// font's true glyph widths just enough that the error compounds over a
// long line and lets the tail run off the window edge, invisibly, since
// nothing else clips it.
std::vector<std::string> wrapToPixelWidth(const sf::Font& font, unsigned charSize, const std::string& text,
                                           float maxWidthPx) {
    std::vector<std::string> lines;
    std::istringstream words(text);
    std::string word;
    std::string current;
    while (words >> word) {
        const std::string candidate = current.empty() ? word : current + " " + word;
        sf::Text probe(font, candidate, charSize);
        if (probe.getLocalBounds().size.x > maxWidthPx && !current.empty()) {
            lines.push_back(current);
            current = word;
        } else {
            current = candidate;
        }
    }
    if (!current.empty()) {
        lines.push_back(current);
    }
    if (lines.empty()) {
        lines.push_back(std::string());
    }
    return lines;
}

// Placeholder fill color per zone tile code -- plain colored rectangles,
// not art (see kZoneTilePx above). POI/portal tiles hold their own POI
// letter code in the grid (not '.'), so they fall through to default here
// too; ZoneLoader::loadFromFile already fails fast unless every grid
// character is either one of the codes below or a declared POI, so
// `default` is unreachable for any zone that loaded at all -- just a
// plain-floor fallback, not a "spot the gap" signal. The POI's own icon
// (see PoiKind/drawPoiIcon below) is what actually distinguishes it.
sf::Color colorForZoneTile(char code) {
    switch (code) {
        case '.': return sf::Color(200, 190, 160); // open ground
        case '%': return sf::Color(55, 120, 55);   // vallenwood / dense growth
        case '#': return sf::Color(70, 70, 78);    // wall
        case '~': return sf::Color(60, 100, 180);  // water
        case '+': return sf::Color(120, 85, 50);   // doorway
        default: return sf::Color(200, 190, 160);  // POI/portal tile -- same as open ground
    }
}

// What a POI tile visually is, for icon selection -- computed at draw
// time from data already on world::PointOfInterest/world::Zone, not a
// stored field. Order matters only in the (currently never-hit, per a
// grep across every data/zones/*.txt) case where a POI is more than one
// of these at once.
enum class PoiKind { Door, Shop, Bed, Person, Landmark };

PoiKind poiKindFor(bool isPortal, const world::PointOfInterest& poi) {
    if (isPortal) return PoiKind::Door;
    if (poi.isShop) return PoiKind::Shop;
    if (poi.isBed) return PoiKind::Bed;
    if (!poi.dialogue.empty()) return PoiKind::Person;
    return PoiKind::Landmark;
}

// Zone landmark plates: a single static illustration shown full-window on
// entry to a zone, if one exists for it -- the Gold Box convention this
// project's own UI chrome already comes from (see docs/ARCHITECTURE.md's
// SFML section). Same "the file's mere presence is the opt-in" contract as
// loadCombatSprite (CombatSprite.h) -- no zone-file grammar, no
// ZoneLoader/Zone.h change, ships correctly with zero art since every zone
// simply has no matching file yet. Unlike CombatSprite there's no pure
// frame-splitting math worth a separate file/unit test here, just a
// straight image load, so this stays a small free function alongside the
// other small rendering helpers in this file. Never throws -- optional
// presentation, same as loadCombatSprite.
std::optional<sf::Texture> loadZonePlateTexture(const std::string& zoneId) {
    sf::Texture texture;
    if (!texture.loadFromFile("assets/plates/" + zoneId + ".png")) return std::nullopt;
    texture.setSmooth(true);  // a painted illustration, not pixel art --
                              // unlike CombatSprite's setSmooth(false)
    return texture;
}

// Dialogue portrait (Milestone 208) -- same "file's mere presence is the
// opt-in" contract as loadZonePlateTexture just above, keyed instead by
// DialogueCandidate::id (either "<zoneId>:<poiChar>" for an ordinary zone
// POI, or a canon Hero's own character id for a chance encounter -- see
// where DialogueCandidate::id is assigned in dialogueBegin). ':' isn't
// filesystem-safe, so it's swapped for '_' before the path is built:
// Otik (solace_inn:O) -> assets/portraits/solace_inn_O.png.
std::optional<sf::Texture> loadDialoguePortraitTexture(const std::string& candidateId) {
    std::string filename = candidateId;
    std::replace(filename.begin(), filename.end(), ':', '_');
    sf::Texture texture;
    if (!texture.loadFromFile("assets/portraits/" + filename + ".png")) return std::nullopt;
    texture.setSmooth(true);
    return texture;
}

// Gold-Box town menu (Milestone 206, docs/ZONE_NOTES.md's "Town menus"):
// one row per actionable POI in a TOWN_MENU zone, auto-derived from data
// the zone file already has -- see buildTownMenuItems below.
struct TownMenuItem {
    char hotkeyChar = '?';
    std::string label;
    bool isPortal = false;
    std::string portalTarget;  // only meaningful if isPortal
    int x = 0;
    int y = 0;                 // only meaningful if !isPortal
};

// Same "lookup table, not enum arithmetic" idiom as the character-creation
// wizard's own Gold-Box letter picker further down this file -- kept as
// its own small, self-contained copy (rather than sharing that one's local
// declaration) so this stays fully isolated from that large, separately-
// verified block. Returns 0-25 for A-Z, -1 otherwise.
int letterIndexForKey(sf::Keyboard::Key key) {
    static constexpr std::array<sf::Keyboard::Key, 26> kLetterKeys = {
        sf::Keyboard::Key::A, sf::Keyboard::Key::B, sf::Keyboard::Key::C, sf::Keyboard::Key::D,
        sf::Keyboard::Key::E, sf::Keyboard::Key::F, sf::Keyboard::Key::G, sf::Keyboard::Key::H,
        sf::Keyboard::Key::I, sf::Keyboard::Key::J, sf::Keyboard::Key::K, sf::Keyboard::Key::L,
        sf::Keyboard::Key::M, sf::Keyboard::Key::N, sf::Keyboard::Key::O, sf::Keyboard::Key::P,
        sf::Keyboard::Key::Q, sf::Keyboard::Key::R, sf::Keyboard::Key::S, sf::Keyboard::Key::T,
        sf::Keyboard::Key::U, sf::Keyboard::Key::V, sf::Keyboard::Key::W, sf::Keyboard::Key::X,
        sf::Keyboard::Key::Y, sf::Keyboard::Key::Z,
    };
    for (int i = 0; i < static_cast<int>(kLetterKeys.size()); ++i) {
        if (kLetterKeys[static_cast<size_t>(i)] == key) return i;
    }
    return -1;
}

// Scans the zone grid once, collecting one row per actionable POI -- a
// PORTAL target, SHOP, BED, anything with a TALK line, or the zone's own
// TIMELINE_ANCHOR POI (Milestone 210) -- deduped by POI char. Pure scenery
// (none of the above) is silently omitted, same as a real Gold Box town
// menu only ever listing actual destinations. A synthetic final row,
// hotkey 'L', is always appended (its label passed in by the caller --
// see townMenuLeaveLabel; "Leave town" isn't accurate for a zone reached
// via a parent's own menu, e.g. Solace's Inn) -- ZoneLoader guarantees
// no real POI uses that letter for a zone explicitly flagged TOWN_MENU,
// but a zone that's only *effectively* menu-town because it has art
// (isEffectiveMenuTown, Milestone 210) was never data-validated this way,
// since ZoneLoader has no filesystem/art awareness to check against; this
// is now a documented authoring rule instead (docs/ZONE_NOTES.md) --
// avoid 'L'/'l' on any actionable POI in any zone, not just ones flagged
// TOWN_MENU today, since any zone could get art later. Two real
// violations existed and were fixed as data when this shipped: Palanthas'
// Astinus and High Clerist's Tower's Knight of the Circle both used to be
// 'L'. Rebuilt fresh on demand (a handful of POIs per zone) rather than
// cached, same "no premature caching" restraint as loadZonePlateTexture
// above.
std::vector<TownMenuItem> buildTownMenuItems(const world::Zone& zone, const std::string& leaveLabel) {
    std::vector<TownMenuItem> items;
    std::vector<char> seen;
    for (int y = 0; y < zone.height(); ++y) {
        for (int x = 0; x < zone.width(); ++x) {
            const world::PointOfInterest* poi = zone.poiAt(x, y);
            if (poi == nullptr) continue;
            if (std::find(seen.begin(), seen.end(), poi->code) != seen.end()) continue;
            seen.push_back(poi->code);
            const std::string* portalTarget = zone.portalAt(x, y);
            // Milestone 210: a zone's TIMELINE_ANCHOR POI (where a canon
            // Hero can be found -- docs/TIMELINE_NOTES.md) is actionable
            // even with no TALK line of its own (e.g. solace_inn's Great
            // Fireplace) -- without this, that POI would silently get no
            // menu row at all, making the encounter unreachable the moment
            // a zone becomes menu-town. The row's label is just the POI's
            // own declared name; selecting it falls into the generic
            // dialogueBegin() branch below like any other talk-only row,
            // which already checks the anchor independently of the POI's
            // own dialogue (gatherTalkCandidates above).
            const bool actionable = poi->isShop || poi->isBed || !poi->dialogue.empty() ||
                                     portalTarget != nullptr || poi->code == zone.timelineAnchorPoi();
            if (!actionable) continue;
            TownMenuItem item;
            item.hotkeyChar = static_cast<char>(std::toupper(static_cast<unsigned char>(poi->code)));
            item.label = poi->name;
            item.x = x;
            item.y = y;
            if (portalTarget != nullptr) {
                item.isPortal = true;
                item.portalTarget = *portalTarget;
            }
            items.push_back(std::move(item));
        }
    }
    std::sort(items.begin(), items.end(),
              [](const TownMenuItem& a, const TownMenuItem& b) { return a.hotkeyChar < b.hotkeyChar; });
    TownMenuItem leave;
    leave.hotkeyChar = 'L';
    leave.label = leaveLabel;
    items.push_back(std::move(leave));
    return items;
}

// Small reusable shape pool for drawPoiIcon -- declared once and mutated
// per cell, same "declare outside the loop" convention as zoneTileShape/
// locationMarker below. Each icon is 1-2 primitives so the placeholder
// reads as a distinct shape, not just a distinct color.
struct PoiIconShapes {
    sf::RectangleShape rectA;
    sf::RectangleShape rectB;
    sf::ConvexShape triangle;
    sf::ConvexShape diamond;
    sf::CircleShape circle;

    explicit PoiIconShapes(float tilePx) : circle(tilePx * 0.22f) {
        triangle.setPointCount(3);
        diamond.setPointCount(4);
        circle.setOrigin(sf::Vector2f(tilePx * 0.22f, tilePx * 0.22f));
    }
};

// Draws a 1-2 primitive placeholder icon for `kind`, centered on
// (centerX, centerY). No image assets exist for zone interiors yet (see
// docs/ARCHITECTURE.md's SFML section) -- this is placeholder shape/color
// vocabulary, ready to swap for real sprite art later.
void drawPoiIcon(sf::RenderWindow& window, PoiIconShapes& shapes, PoiKind kind, float tilePx, float centerX,
                  float centerY) {
    switch (kind) {
        case PoiKind::Door: {
            const float frameW = tilePx * 0.5f;
            const float frameH = tilePx * 0.8f;
            shapes.rectA.setSize(sf::Vector2f(frameW, frameH));
            shapes.rectA.setOrigin(sf::Vector2f(frameW / 2.f, frameH / 2.f));
            shapes.rectA.setPosition(sf::Vector2f(centerX, centerY));
            shapes.rectA.setFillColor(sf::Color(101, 67, 33)); // wood frame
            window.draw(shapes.rectA);

            const float openW = frameW * 0.55f;
            const float openH = frameH * 0.7f;
            shapes.rectB.setSize(sf::Vector2f(openW, openH));
            shapes.rectB.setOrigin(sf::Vector2f(openW / 2.f, openH / 2.f));
            shapes.rectB.setPosition(sf::Vector2f(centerX, centerY + frameH * 0.06f));
            shapes.rectB.setFillColor(sf::Color(235, 205, 150)); // lit opening
            window.draw(shapes.rectB);
            break;
        }
        case PoiKind::Shop: {
            const float counterW = tilePx * 0.7f;
            const float counterH = tilePx * 0.35f;
            shapes.rectA.setSize(sf::Vector2f(counterW, counterH));
            shapes.rectA.setOrigin(sf::Vector2f(counterW / 2.f, counterH / 2.f));
            shapes.rectA.setPosition(sf::Vector2f(centerX, centerY + tilePx * 0.2f));
            shapes.rectA.setFillColor(sf::Color(150, 130, 100)); // counter
            window.draw(shapes.rectA);

            const float awningW = tilePx * 0.8f;
            const float awningH = tilePx * 0.3f;
            shapes.triangle.setPoint(0, sf::Vector2f(-awningW / 2.f, 0.f));
            shapes.triangle.setPoint(1, sf::Vector2f(awningW / 2.f, 0.f));
            shapes.triangle.setPoint(2, sf::Vector2f(0.f, -awningH));
            shapes.triangle.setPosition(sf::Vector2f(centerX, centerY - tilePx * 0.05f));
            shapes.triangle.setFillColor(sf::Color(70, 150, 150)); // awning
            window.draw(shapes.triangle);
            break;
        }
        case PoiKind::Bed: {
            const float mattressW = tilePx * 0.75f;
            const float mattressH = tilePx * 0.4f;
            shapes.rectA.setSize(sf::Vector2f(mattressW, mattressH));
            shapes.rectA.setOrigin(sf::Vector2f(mattressW / 2.f, mattressH / 2.f));
            shapes.rectA.setPosition(sf::Vector2f(centerX, centerY));
            shapes.rectA.setFillColor(sf::Color(180, 160, 210)); // mattress
            window.draw(shapes.rectA);

            const float pillowW = mattressW * 0.3f;
            const float pillowH = mattressH * 0.7f;
            shapes.rectB.setSize(sf::Vector2f(pillowW, pillowH));
            shapes.rectB.setOrigin(sf::Vector2f(pillowW / 2.f, pillowH / 2.f));
            shapes.rectB.setPosition(sf::Vector2f(centerX - mattressW / 2.f + pillowW / 2.f + 2.f, centerY));
            shapes.rectB.setFillColor(sf::Color(230, 225, 240)); // pillow
            window.draw(shapes.rectB);
            break;
        }
        case PoiKind::Person: {
            shapes.circle.setPosition(sf::Vector2f(centerX, centerY - tilePx * 0.2f));
            shapes.circle.setFillColor(sf::Color(230, 180, 90)); // head
            window.draw(shapes.circle);

            const float bodyBottomW = tilePx * 0.32f;
            const float bodyH = tilePx * 0.4f;
            shapes.triangle.setPoint(0, sf::Vector2f(0.f, 0.f));
            shapes.triangle.setPoint(1, sf::Vector2f(-bodyBottomW, bodyH));
            shapes.triangle.setPoint(2, sf::Vector2f(bodyBottomW, bodyH));
            shapes.triangle.setPosition(sf::Vector2f(centerX, centerY));
            shapes.triangle.setFillColor(sf::Color(190, 130, 60)); // body/robe
            window.draw(shapes.triangle);
            break;
        }
        case PoiKind::Landmark: {
            const float r = tilePx * 0.28f;
            shapes.diamond.setPoint(0, sf::Vector2f(0.f, -r));
            shapes.diamond.setPoint(1, sf::Vector2f(r, 0.f));
            shapes.diamond.setPoint(2, sf::Vector2f(0.f, r));
            shapes.diamond.setPoint(3, sf::Vector2f(-r, 0.f));
            shapes.diamond.setPosition(sf::Vector2f(centerX, centerY));
            shapes.diamond.setFillColor(sf::Color(110, 130, 90)); // quiet scenery marker
            window.draw(shapes.diamond);
            break;
        }
    }
}

// Drives the non-blocking combat state machine -- the console version's
// GameLoop::runCombat is one big blocking `for(;;) { draw(); readKey(); }`
// loop with nested blocking pickers; this SFML build instead advances one
// KeyPressed event at a time, so "what's the screen waiting on right now"
// has to be explicit state instead of a call stack. AwaitContinue covers
// both the opening "X appears!" beat and the closing Won/Lost/Fled beats --
// only Enter dismisses any of them, same "don't let a held movement key
// silently act" discipline docs/GOTCHAS.md documents for the console
// version. PickingTarget is the only *mid-round* pause: this phase commits
// to choosing one target per round (see CombatSession::pendingTargetIsFirst
// below) rather than porting GameLoop::runCombat's mid-round retarget-on-
// kill loop, which would need real cross-frame resumable state to redo
// non-blockingly -- a deliberate scope cut, not an oversight. PickingSpell
// and PickingItem are further pre-round pauses (which of 2+ memorized
// spells to cast, or 2+ usable items to use) -- like the console's own
// blocking spell-choice and USE-menu loops (GameLoop.cpp:2906-2928 and
// 2955-2977), each resolves entirely before initiative is rolled, so
// cancelling it (Escape/Q) costs nothing; PickingTarget itself is reused
// for a spell's or Webnet's target choice once one is picked (see
// CombatSession::pickReason/TargetPickReason below) -- a physical Attack
// used to be PickingTarget's third reason too, but Milestone 204 moved
// attack targeting to its own free-look Aiming cursor/combatConfirmAim
// instead (see Aiming's own doc comment below), so PickingTarget is Spell/
// Webnet only now.
enum class CombatUiState {
    AwaitContinue,
    Idle,
    // Free-look targeting cursor for a physical Attack (Milestone 204,
    // real DQoK "AIM" semantics -- see combatBeginAim's own doc comment):
    // pans anywhere on the battlefield with zero round cost, only
    // committing (and rolling initiative) once Enter confirms a legal
    // target. Distinct from PickingTarget below, which stays a list-cycle
    // over already-known-legal candidates for Spell/Webnet.
    Aiming,
    PickingTarget,
    PickingSpell,
    PickingItem,
    // Milestone 186: 'v' while Idle -- choosing who to inspect
    // (ViewPicking), then the resulting read-only stat card
    // (ViewingCard). Costs no round, same "pure info window" treatment as
    // Help/Journal/the character sheet -- see combatBeginView's own doc
    // comment.
    ViewPicking,
    ViewingCard,
    Won,
    Lost,
    Fled
};

// Which of the two real reasons PickingTarget is ever open for -- a spell
// that needs a target, or Webnet's own "tangle which enemy?"
// (character::useWebnet succeeded, still needs to know who) -- since
// combatConfirmTarget and the footer text both need to know which action to
// actually resolve once a target is chosen. A physical Attack was a third
// case here before Milestone 204 gave it its own free-look Aiming cursor
// instead (see CombatUiState::Aiming's own doc comment) -- removed rather
// than left unreachable.
enum class TargetPickReason { Spell, Webnet };

struct CombatInstance {
    int hp = 0;
    int maxHp = 0;
};

// All state for one fight, local to this file -- deliberately NOT part of
// game::GameState (combat isn't saved there either; GameLoop::runCombat's
// own instances/positions/log are just as local to its one call). See this
// file's top-of-file comment and docs/CURRENT_WORK.md for why this stays
// its own struct rather than a third game::Mode value.
struct CombatSession {
    bool active = false;
    combat::Monster monster;
    bool useLetters = false; // group of 2+ -- see GameLoop::runCombat's own useLetters
    std::vector<CombatInstance> instances;
    std::vector<combat::GridPos> instancePositions;
    std::vector<combat::GridPos> companionPositions;
    combat::GridPos playerPos;
    char floorTerrainCode = '.';
    // Milestone 188: this fight's wall layout (owned by the startup-loaded
    // world::BattleMapCatalog, so a raw pointer is safe for the catalog's
    // whole lifetime -- same "pointer into a catalog" precedent currentZone
    // already establishes) and every wall cell inside it, flattened once in
    // combatStartEncounter so movement/pathing/rendering don't rescan the
    // whole 50x25 grid on every single check. nullptr/empty means "no
    // battlemap for this terrain" (defensive only -- see
    // BattleMapCatalog::forTerrain's own doc comment), which combat code
    // below treats as an all-open floor, the pre-Milestone-188 behavior.
    const world::BattleMap* battleMap = nullptr;
    std::vector<combat::GridPos> wallPositions;
    std::vector<std::string> log;
    CombatUiState uiState = CombatUiState::AwaitContinue;

    // Free-look Aim cursor (Milestone 204) -- only meaningful while uiState
    // == Aiming. Position on the combat grid the player is currently
    // looking at; moves freely (no bounds beyond the grid itself, no wall/
    // occupancy check -- it's a viewfinder, not the character moving).
    combat::GridPos aimCursor;

    // Target-picker state -- only meaningful while uiState == PickingTarget.
    std::vector<int> pickCandidates;
    int pickSelected = 0;
    // Whether the player's side acts before the monsters this round
    // (combat::playerActsFirst(), rolled once at the top of the round and
    // remembered here so it's still known once the picker -- which may
    // span several frames -- finally confirms a target).
    bool pendingGoFirst = true;

    int roundNumber = 1;

    // Milestone 185's real per-round movement: how many grid squares the
    // player has left to spend this round (character::movementSquares,
    // DQoK.pdf p.51), reset at the top of every round in combatWrapUpRound.
    // Moving no longer ends the round by itself (see combatBeginPlayerMove)
    // -- only an attack, a cast, an item, or the new Space "hold action"
    // key does, via combatFinishPlayerAction.
    int movementRemaining = 0;

    // Whether combatRollGoFirstAndMaybeActMonsters has already resolved
    // initiative for the CURRENT round -- set true the first time it runs
    // each round (a move or an attack, whichever comes first), reset false
    // in combatWrapUpRound. Makes that function idempotent within a round,
    // so every action this round can call it unconditionally without
    // re-rolling initiative or re-running the monsters' whole turn a
    // second time.
    bool initiativeRolledThisRound = false;

    // Fight-start-only bonuses (Frostreaver's glacier edge, Weapon
    // Specialization) plus Aurak's blind-on-failed-save debuff, which
    // further adjusts playerThac0Bonus mid-fight -- same "this-fight-only
    // local, never written to the real Character" precedent
    // GameLoop::runCombat's own identically-named locals establish.
    int playerThac0Bonus = 0;
    int playerDamageBonus = 0;

    // The rest of GameLoop::runCombat's own this-fight-only spell buff/
    // debuff locals (GameLoop.cpp:1955-1978) -- Bless/Prayer/Protection
    // from Evil/Strength/Slow/Haste/Web/... (character/Spellcasting.h's
    // SpellEffect). Sized to instances.size() fresh in combatStartEncounter,
    // same as the console's own per-fight vectors.
    int playerAcBonus = 0;
    int hasteAttackMultiplier = 1;
    std::vector<int> monsterThac0Penalty;
    std::vector<int> monsterDamagePenalty;
    std::vector<int> monsterAcPenalty;
    std::vector<int> blockedAttacksRemaining;
    std::vector<bool> incapacitatedRestOfFight;

    // Spell-choice picker state -- only meaningful while uiState ==
    // PickingSpell. spellPickIds/spellPickLabels are parallel (distinct
    // memorized spell ids in memorization order, and their display
    // labels -- "Name" or "Name (xN)" if memorized more than once).
    std::vector<std::string> spellPickIds;
    std::vector<std::string> spellPickLabels;
    int spellPickSelected = 0;

    // Distinguishes PickingTarget's reused picker (see CombatUiState's own
    // doc comment and TargetPickReason above): Spell means this round's
    // target choice resolves pendingSpellResult (character::castSpell has
    // already run, consuming the memorized slot -- see
    // combatCommitSpellChoice); Webnet means it resolves a successful
    // character::useWebnet call instead (see combatCommitItemChoice). The
    // default value here is never actually read before being explicitly
    // set (both entry points set it before opening PickingTarget) --
    // arbitrary, just needs to be a valid enumerator.
    TargetPickReason pickReason = TargetPickReason::Spell;
    character::SpellCastResult pendingSpellResult;

    // Item-choice picker state -- only meaningful while uiState ==
    // PickingItem. Parallel to spellPickIds/spellPickLabels/
    // spellPickSelected above: itemPickKinds/itemPickLabels are parallel
    // (character::availableCombatItems's own kind + display label, in its
    // fixed Potion/Webnet/Brooch/StaffCure order).
    std::vector<character::CombatItemKind> itemPickKinds;
    std::vector<std::string> itemPickLabels;
    int itemPickSelected = 0;

    // Brooch of Imog's globe of invulnerability (character::activateBrooch,
    // Dragonlance Adventures p.92) -- player-only, absorbs one Bozak Magic
    // Missile/Aurak breath weapon/melee hit per attempt for the rest of the
    // fight once activated (see combatMonstersAct/
    // combatTriggerOpportunityAttacks's own globeActive checks). Mirrors
    // GameLoop::runCombat's identically-named local (GameLoop.cpp:1995),
    // reset fresh each fight the same way every other CombatSession field
    // above is (combatStartEncounter's wholesale `CombatSession{}` reset).
    bool globeActive = false;

    // Thief backstab (Milestone 119 in the console build, DQoK.pdf's own
    // positional version): WHO first attacked each instance, for this
    // fight's whole lifetime -- NOT reset per round, unlike most of the
    // fields above. -1 = no one yet, 0 = the player, N = state.companions
    // [N-1]. Sized fresh in combatStartEncounter, same as the other
    // per-instance vectors above. See combatBackstabBonus's own doc
    // comment for how this is used.
    std::vector<int> firstAttackerId;

    // View-picker state (Milestone 186) -- only meaningful while uiState ==
    // ViewPicking/ViewingCard. A richer identity than firstAttackerId's
    // plain int above needs: VIEW can target the player, a companion, OR a
    // monster instance, three categories rather than the attacker
    // encoding's two.
    struct ViewCandidate {
        enum class Kind { Player, Companion, Monster } kind;
        int index = 0; // meaningful only for Companion/Monster
    };
    std::vector<ViewCandidate> viewCandidates;
    int viewSelected = 0;
    // Built by combatConfirmView once a candidate is chosen; drawn as a
    // read-only drawPickerOverlay card (selectedIndex -1) while
    // uiState == ViewingCard.
    std::vector<std::string> viewCardLines;

    // Turn-follow camera/card (Milestone 200): parallel to `log` above --
    // logActorForLine[i] records who produced log[i] (the player, a
    // companion, or a monster instance), so AwaitContinue's paced replay
    // can center the camera and swap the persistent stat card onto
    // whoever's turn each currently-displayed line is actually about,
    // instead of always defaulting to the player. currentTurnActor is the
    // "who's producing messages right now" cursor: reset to Player at the
    // top of every player action function (and again right after any
    // combatRollGoFirstAndMaybeActMonsters() call, since that can run the
    // monsters' whole turn first and leave this pointing at one of them),
    // and to the relevant Companion/Monster at the top of each iteration
    // of combatCompanionActs'/combatMonstersAct's own loops. See
    // combatPushLog below, which stamps every pushed line with whatever
    // this is currently set to.
    std::vector<ViewCandidate> logActorForLine;
    ViewCandidate currentTurnActor{ViewCandidate::Kind::Player, 0};

    // Message-pacing queue (DQoK-style combat HUD, Milestone 199). A single
    // keypress can synchronously push several log lines below (the player's
    // own action, each companion's, each monster's, a death/knockout) before
    // the round-ending function decides Idle/Won/Lost/Fled -- actionLogStart/
    // roundJustConcluded let that internal resolution run completely
    // unchanged, while the input dispatch intercepts the result afterward
    // and replays [actionLogStart, log.size()), one line at a time (Milestone
    // 204: automatically for an ordinary round conclusion, see
    // pendingMessageAutoAdvance below; still one Enter press at a time for
    // the encounter-opening/closing beats), before ever applying the real
    // target state. actionLogStart is re-armed (to log.size()) and
    // roundJustConcluded reset false as the first statement of every entry
    // point that can end a round (see combatConfirmAim/combatConfirmTarget/
    // combatCommitSpellChoice/combatCommitItemChoice/combatEndTurn/
    // combatBeginFlee/combatBeginPlayerMove); roundJustConcluded is set true by
    // combatWrapUpRound/combatKnockedOutBy/combatBeginFlee right after their
    // own (unchanged) uiState assignment.
    size_t actionLogStart = 0;
    bool roundJustConcluded = false;
    // Set once roundJustConcluded fires (by the wrapper just after the input
    // switch): which log lines to replay, and which real CombatUiState to
    // apply once pendingMessageIndex reaches pendingMessageEnd.
    // combatStartEncounter sets these three by hand too, for the same "X
    // appears!" beat this used to hardcode as AwaitContinue's struct default.
    size_t pendingMessageIndex = 0;
    size_t pendingMessageEnd = 0;
    CombatUiState pendingTargetState = CombatUiState::Idle;
    // Milestone 204: whether the CURRENT pendingMessageIndex..pendingMessageEnd
    // queue should advance on its own (a short timer, see kMessageAutoAdvanceMs/
    // combatAdvancePendingMessage near the main loop) instead of waiting for an
    // Enter press each line -- real Gold Box games don't stop for a keypress on
    // routine round narration ("the enemies just keep going until it's your
    // turn"). Only ever set true for an ordinary round conclusion that lands
    // back on Idle (see the wrapper right after the input-dispatch switch,
    // where this is set alongside pendingTargetState) -- defaults false so the
    // encounter-opening "X appears!" beat (set up by hand in
    // combatStartEncounter, never touching this field) and the closing
    // Won/Lost/Fled beats both stay a deliberate, manual Enter-to-continue.
    // Enter still works as a manual skip-ahead even while auto-advancing.
    bool pendingMessageAutoAdvance = false;
};

// Dialogue's own UI states -- non-blocking port of GameLoop::talkTo's
// conversation loop (see docs/CURRENT_WORK.md's scope writeup). Quest
// turn-in, boat voyages, and companion recruiting are all wired up -- see
// the Quest*/Wayreth* values further below, and BoatOffer/RecruitOffer
// here. PickingCandidate mirrors GameLoop::pickAndTalk's
// "more than one candidate here" picker; Greeting/TopicText/TopicPicker
// mirror talkTo's own greeting-then-topic-menu flow. BoatOffer mirrors
// talkTo's own Board/Not yet picker (GameLoop.cpp:1061-1115), reached from
// Greeting's dismissal whenever the candidate carries a boatDestinationId,
// ahead of the topic menu; RecruitOffer mirrors talkTo's own Join me/Not yet
// picker (GameLoop.cpp:1116-1153), reached right after BoatOffer is resolved
// (accepted or declined) whenever the candidate carries a
// recruitCompanionId not already in the party -- same precedence order
// talkTo itself uses (boat, then recruit, then topics), via the shared
// dialogueOfferRecruitOrTopics tail. Unlike boarding a boat (which ends the
// conversation outright), joining or declining a recruit offer falls
// through to the topic picker either way, matching talkTo exactly.
// AskInput/AskResponse are the free-text "ask about something else..."
// flow -- AskInput is the typing box, AskResponse shows the resulting
// response (and, when today's ask limit is reached or extended, a queued
// 2nd message). The Quest*/Wayreth* values mirror GameLoop::
// offerOrTurnInQuest (GameLoop.cpp:1308-1528), reached from Greeting's
// dismissal ahead of BoatOffer (see questBegin/dialogueAfterGreeting) --
// QuestOfferText/QuestAcceptDecline are the not-yet-started path,
// QuestProgressText the Active-but-unfinished path, QuestCompleteText the
// ReadyToTurnIn path (reward flags applied once it's dismissed), and
// WayrethIntro/WayrethChoice the Test of High Sorcery scene that
// rewardWayrethRobe stages on top of QuestCompleteText.
enum class DialogueUiState {
    PickingCandidate,
    Greeting,
    TopicText,
    TopicPicker,
    BoatOffer,
    RecruitOffer,
    AskInput,
    AskResponse,
    QuestOfferText,
    QuestAcceptDecline,
    QuestAcceptText,
    QuestProgressText,
    QuestCompleteText,
    WayrethIntro,
    WayrethChoice
};

// All state for one conversation, local to this file -- same "not part of
// game::GameState" reasoning as CombatSession above (a conversation is
// transient UI state, not world state).
struct DialogueSession {
    bool active = false;
    std::vector<DialogueCandidate> candidates;
    int candidateSelected = 0;
    DialogueUiState uiState = DialogueUiState::PickingCandidate;
    DialogueCandidate current;             // the candidate actually being talked to
    std::string bodyText;                  // text currently on screen
    int bodyTextPage = 0;                  // which page of bodyText is showing -- see paginateBodyText
    std::string displaySpeaker;            // usually current.name; overridden by askLimitLocked's speaker
    std::vector<std::string> topicLabels;  // topics [+ "Ask about..."] + "Nothing, thanks"
    int topicSelected = 0;
    int boatOfferSelected = 0;             // BoatOffer's Board(0)/Not yet(1) picker cursor
    int recruitOfferSelected = 0;          // RecruitOffer's Join me(0)/Not yet(1) picker cursor
    int questOfferSelected = 0;            // QuestAcceptDecline's Accept(0)/Decline(1) picker cursor
    int wayrethChoiceSelected = 0;         // WayrethChoice's 3-option ethical picker cursor
    int askAnythingIndex = -1;             // index into topicLabels, or -1 if not offered this visit
    std::string askInputBuffer;            // free-typed text, built up from TextEntered events
    std::vector<std::string> askInputHints;
    std::vector<std::string> askMessages;  // 1 or 2 queued response lines to show in AskResponse
    int askMessageIndex = 0;
    bool askQueueEndsConversation = false; // true for SUBJECT_ENDS or a reached (non-extended) ask limit
};

// All state for one shop visit, local to this file -- same "transient UI
// state, not part of game::GameState" reasoning as CombatSession/
// DialogueSession above. Unlike those two, a shop has exactly one screen
// shape throughout (a buy or sell list, toggled by sellMode), so there's no
// UiState enum here -- matches game::GameLoop::handleShop's own single
// for(;;) loop with no phase distinction beyond the sellMode toggle.
struct ShopSession {
    bool active = false;
    std::string shopName;
    character::ShopCatalog catalog = character::ShopCatalog::General;
    bool sellMode = false;
    int selected = 0;
    std::string message; // post-transaction feedback, cleared on mode toggle
};

// All state for one inventory visit -- same "transient UI state" reasoning
// as ShopSession above, and just as flat: a single picker list, no UiState
// enum. Unlike Shop this isn't POI-gated -- works from Overworld or Zone,
// matching Character Sheet's sheetOpen convention instead.
struct InventorySession {
    bool active = false;
    int selected = 0;
    std::string message; // post-action feedback (equip/drink/no-op), cleared on open
};

// State for the full event log overlay ('v') -- same "transient UI state"
// reasoning as ShopSession/InventorySession above. Unlike those two this
// needs no picker cursor, just a scroll position: scrollOffset uses the
// same -1 "start at the bottom" sentinel render::MapRenderer::drawLogFrame
// does (MapRenderer.cpp:1220), so opening the log always starts at the
// most recent entry.
struct LogSession {
    bool active = false;
    int scrollOffset = -1;
};

// State for the Look ('l') command -- mirrors GameLoop::pickAndLook's own
// shape (GameLoop.cpp:937-969): a single candidate skips straight to
// showingDetail (no picker step at all, matching the console's
// candidates.size()==1 fast path); 2+ candidates start in the picker and
// move to showingDetail once one is chosen via Enter. Either way,
// showingDetail dismisses on any key and ends the whole session -- Look
// never loops back to its own picker the way Talk's pickAndTalk does.
struct LookSession {
    bool active = false;
    std::vector<LookCandidate> candidates;
    int selected = 0;
    bool showingDetail = false;
};

// One slot in Rest/Bed Rest's spell-loadout wizard -- one entry per slot
// still needing a pick, flattened across every accessible level, mirroring
// GameLoop::chooseSpellLoadout's nested level/slot loop (GameLoop.cpp:
// 590-620). slotNumber/totalSlotsAtLevel are 1-based/per-level (not a
// position in this flattened queue) purely so the picker's title can match
// the console's own "Level N spell (X/Y)" wording exactly.
struct RestSpellPick {
    int level = 0;
    int slotNumber = 0;
    int totalSlotsAtLevel = 0;
    std::vector<const character::SpellInfo*> choices;
};

// State for the Rest ('r') / Bed Rest ('z') spell-loadout wizard -- same
// "transient UI state" reasoning as the sessions above. Time/HP/
// lastRestDay are committed the instant Rest/Bed Rest fires (see
// restBegin below), matching GameLoop::handleRest/handleBedRest's own
// ordering (both mutate state before ever calling performSpellMemorization)
// -- this struct only drives the spell-memorization half that follows,
// ported from GameLoop::performSpellMemorization/chooseSpellLoadout
// (GameLoop.cpp:527-624). A non-caster, or a caster who's never memorized
// anything before, skips straight to the queue below (or finishes with no
// picker at all if the queue turns out empty); an existing caster sees
// keepSamePrompt first. Unlike every other picker in this file, there is
// deliberately no cancel here -- chooseSpellLoadout's own console loop
// only ever responds to North/South/Enter, never Escape, so Q/Escape (and
// everything else) are simply swallowed while this session is active, not
// bound to a cancel.
struct RestSession {
    bool active = false;
    bool keepSamePrompt = false;
    int keepSameSelected = 0;
    std::vector<RestSpellPick> queue;
    size_t queueIndex = 0;
    int pickSelected = 0;
    std::vector<std::string> loadout;
    std::string baseMessage;   // "You settle in..."/"You spend the night..." + healed/full-health wording
    long long dayAfterRest = 0;
};

int runPhase1(const std::string& savePath) {
    unsigned windowW = 1280;
    unsigned windowH = 800;
    const float sidebarWidth = 320.f;

    std::cout << "step 0: starting, save = "
              << (savePath.empty() ? "(none -- will show the save-slot menu)" : savePath) << std::endl;

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(windowW, windowH)),
                             "Ansalon SFML Phase 1+2+3 -- Real Overworld + Zones + Combat (WIP)");
    window.setFramerateLimit(60);

#ifdef _WIN32
    // Launch maximized. SFML 3's sf::State only distinguishes Windowed/
    // Fullscreen (the latter is exclusive borderless, not what "maximized"
    // means -- title bar + working minimize/restore/close, taskbar
    // respected), so this reaches through to the real OS handle instead.
    // Done before any drawing happens, so the loading screen itself already
    // renders at the maximized size, not a small window that then grows.
    ShowWindow(window.getNativeHandle(), SW_MAXIMIZE);
    // window.getSize() here is unreliable -- confirmed live (diagnostic
    // std::cout, since removed) that it still returns the pre-maximize
    // 1280x800 construction size immediately after ShowWindow returns, on
    // at least one real environment (SFML 3's WindowImplWin32 apparently
    // caches its size and doesn't refresh it just because the native
    // handle was resized out from under it via a raw ShowWindow call
    // rather than SFML's own resize path). GetClientRect queries the
    // actual OS window geometry directly, with no SFML-side caching to go
    // stale.
    RECT clientRect;
    GetClientRect(window.getNativeHandle(), &clientRect);
    windowW = static_cast<unsigned>(clientRect.right - clientRect.left);
    windowH = static_cast<unsigned>(clientRect.bottom - clientRect.top);
#endif
    const float mapWidth = static_cast<float>(windowW) - sidebarWidth;

    // Relocated here (originally declared much later, alongside the map
    // texture) for the same reason drawPickerOverlay itself was moved
    // earlier, just below -- the save-slot menu and character-creation
    // wizard draw before that later point too, and without this they ran
    // under SFML's default view, which is only computed once at
    // window-creation time (1280x800) and never recomputed on resize/
    // maximize. drawPickerOverlay's text-wrap budget is computed from the
    // real (larger) windowW/windowH, so a long line would wrap "correctly"
    // against a budget the stale default view couldn't actually show,
    // overflowing off the real visible edge (confirmed live at 2048x1152:
    // the Weapon Specialization prompt and the slot-menu's overwrite/
    // delete confirmation titles all clipped mid-word). windowW/windowH
    // are never reassigned again after the maximize block above, so this
    // stays correct for the rest of the run.
    const sf::View uiView(sf::FloatRect({0.f, 0.f},
                                         sf::Vector2f(static_cast<float>(windowW), static_cast<float>(windowH))));
    window.setView(uiView);

    sf::Font font;
    if (!font.openFromFile("References/Gold Box Games.ttf")) {
        std::cerr << "Failed to load References/Gold Box Games.ttf\n";
        return 1;
    }

    // Relocated here (originally declared much later, alongside the
    // character sheet overlay) so the save-slot menu and character
    // creation wizard below -- which run before game::GameState even
    // exists -- can reuse it too. Pure move, no logic change: every
    // existing caller further down in this function keeps working
    // unchanged. Only the subset of kSheet* constants drawPickerOverlay
    // itself actually reads comes with it; kSheetRightX/kSheetColumnWidth/
    // kSheetMaxChars stay with the character sheet overlay that still
    // needs them.
    constexpr unsigned kSheetTitleCharSize = 26;
    constexpr unsigned kSheetHeaderCharSize = 16;
    constexpr unsigned kSheetBodyCharSize = 15;
    const sf::Color kSheetSectionColor(230, 220, 160);
    const sf::Color kSheetBodyColor(210, 210, 210);
    const float kSheetMarginX = 40.f;

    // Gold-Box-style panel chrome, used directly by drawPickerOverlay, the
    // character-creation wizard's Name step (the one step that hand-rolls
    // the same flat-bg-then-text pattern locally instead of calling
    // drawPickerOverlay, since it needs a live-editable text line rather
    // than a selectable list), and, as of the full-overlay chrome pass,
    // drawCharacterSheetOverlay/drawWorldMapOverlay/drawDialogueOverlay
    // too -- every full-window overlay now shares this look. Deliberately
    // a separate palette from kSheetSectionColor/kSheetBodyColor above,
    // not a repurposing of them. The one remaining exception is the
    // startup loading screen, which keeps its own flat-black look --
    // it's a one-shot screen the player never returns to, not worth
    // matching.
    const sf::Color kPanelBg(52, 46, 40);
    const sf::Color kPanelBorderOuter(150, 125, 85);
    const sf::Color kPanelBorderInner(90, 75, 55);
    const sf::Color kPanelHeaderColor(230, 95, 70);
    auto drawPanelChrome = [&]() {
        sf::RectangleShape bg(sf::Vector2f(static_cast<float>(windowW), static_cast<float>(windowH)));
        bg.setFillColor(kPanelBg);
        window.draw(bg);

        sf::RectangleShape outerBorder(
            sf::Vector2f(static_cast<float>(windowW) - 12.f, static_cast<float>(windowH) - 12.f));
        outerBorder.setPosition(sf::Vector2f(6.f, 6.f));
        outerBorder.setFillColor(sf::Color::Transparent);
        outerBorder.setOutlineColor(kPanelBorderOuter);
        outerBorder.setOutlineThickness(3.f);
        window.draw(outerBorder);

        sf::RectangleShape innerBorder(
            sf::Vector2f(static_cast<float>(windowW) - 24.f, static_cast<float>(windowH) - 24.f));
        innerBorder.setPosition(sf::Vector2f(12.f, 12.f));
        innerBorder.setFillColor(sf::Color::Transparent);
        innerBorder.setOutlineColor(kPanelBorderInner);
        innerBorder.setOutlineThickness(2.f);
        window.draw(innerBorder);
    };

    // "Title + selectable list + footer + optional message" frame -- this
    // project's single most-reused screen primitive (see docs/CURRENT_WORK.md's
    // "Generic picker overlay" writeup). A Yes/No prompt is just a 2-item
    // list; a pure recap/info screen is a list with selectedIndex -1 (the
    // same idiom Help/Journal/Spellbook already use).
    auto drawPickerOverlay = [&](const std::string& title, const std::vector<std::string>& items,
                                  int selectedIndex, const std::string& footer, const std::string& message = "") {
        drawPanelChrome();

        // Wrapped against the real rendered pixel width (wrapToPixelWidth
        // measures via the actual font/size) so a long title/message wraps
        // instead of running off the right edge, same helper the dialogue
        // overlay's own body text already uses.
        const float maxWidthPx = static_cast<float>(windowW) - 2.f * kSheetMarginX;
        // "> "/"  " is only ever drawn ahead of an item's first visual
        // line (below), so items are wrapped a bit narrower to leave room
        // for it -- measured directly rather than assumed, since it's
        // drawn at the item's own body size, not the title's.
        const float prefixWidthPx = sf::Text(font, "> ", kSheetBodyCharSize).getLocalBounds().size.x;

        float y = 40.f;
        auto drawLine = [&](const std::string& text, sf::Color color, unsigned size) {
            sf::Text sfText(font, text, size);
            sfText.setFillColor(color);
            sfText.setPosition(sf::Vector2f(kSheetMarginX, y));
            window.draw(sfText);
            y += static_cast<float>(size) + 10.f;
        };

        for (const std::string& line : wrapToPixelWidth(font, kSheetTitleCharSize, title, maxWidthPx)) {
            drawLine(line, kPanelHeaderColor, kSheetTitleCharSize);
        }
        y += 10.f;
        // Each item can itself run long (a race/class name plus an
        // ineligibility annotation, a saves line, etc.) -- wrapped the same
        // way title/message are, with the "> "/"  " selection prefix only
        // on an item's first visual line so multi-line items don't read as
        // several separate selectable rows.
        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            const bool isSelected = i == selectedIndex;
            std::vector<std::string> wrapped = wrapToPixelWidth(font, kSheetBodyCharSize,
                                                                  items[static_cast<size_t>(i)],
                                                                  maxWidthPx - prefixWidthPx);
            if (wrapped.empty()) wrapped.push_back("");
            for (size_t lineIdx = 0; lineIdx < wrapped.size(); ++lineIdx) {
                const std::string prefix = lineIdx == 0 ? (isSelected ? "> " : "  ") : "  ";
                drawLine(prefix + wrapped[lineIdx], isSelected ? sf::Color::White : kSheetBodyColor,
                          kSheetBodyCharSize);
            }
        }
        if (!message.empty()) {
            y += 10.f;
            for (const std::string& line : wrapToPixelWidth(font, kSheetBodyCharSize, message, maxWidthPx)) {
                drawLine(line, sf::Color::White, kSheetBodyCharSize);
            }
        }
        y += 10.f;
        drawLine(footer, sf::Color(150, 150, 160), kSheetHeaderCharSize);
    };

    // Loading screen: a real drawn frame instead of a blank/possibly
    // "Not Responding" window while the steps below run -- the map image
    // decode especially (the packaged demo's ~76MB is dominated by that
    // one file). Local constants rather than the kSheet* ones used by
    // every other overlay below, since those are declared much later in
    // this function, after everything here already runs. Returns false
    // (caller should bail out) if the user closes the window mid-load.
    constexpr unsigned kLoadingTitleSize = 28;
    constexpr unsigned kLoadingStatusSize = 18;
    constexpr float kLoadingMarginX = 60.f;
    auto drawLoadingScreen = [&](const std::string& status) -> bool {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
        }
        if (!window.isOpen()) return false;
        window.clear(sf::Color(12, 12, 16));
        sf::Text title(font, "Ansalon: Age of Despair", kLoadingTitleSize);
        title.setFillColor(sf::Color(230, 220, 160));
        title.setPosition(sf::Vector2f(kLoadingMarginX, static_cast<float>(windowH) / 2.f - 40.f));
        window.draw(title);
        sf::Text statusText(font, status, kLoadingStatusSize);
        statusText.setFillColor(sf::Color(180, 180, 190));
        statusText.setPosition(sf::Vector2f(kLoadingMarginX, static_cast<float>(windowH) / 2.f + 10.f));
        window.draw(statusText);
        window.display();
        return true;
    };

    if (!drawLoadingScreen("Loading overworld grid...")) return 0;
    world::OverworldGrid grid =
        world::OverworldGrid::loadFromFile("data/overworld.grid", "data/overworld_regions.grid");
    std::cout << "step 1: grid loaded " << grid.width() << "x" << grid.height() << std::endl;

    if (!drawLoadingScreen("Loading world data...")) return 0;
    world::World world;
    world::WorldLoader::loadFromFile("data/locations.txt", world);
    std::cout << "step 2: world loaded, " << world.allLocations().size() << " locations" << std::endl;

    if (!drawLoadingScreen("Loading zones...")) return 0;
    world::ZoneCatalog zones = world::ZoneCatalog::loadForWorld(world, "data/zones");
    std::cout << "step 2b: zones loaded, " << zones.allZones().size() << " zones" << std::endl;

    if (!drawLoadingScreen("Loading monster catalog...")) return 0;
    combat::MonsterCatalog monsterCatalog;
    combat::MonsterLoader::loadFromFile("data/monsters.txt", monsterCatalog);
    std::cout << "step 2c: monsters loaded, " << monsterCatalog.size() << " entries" << std::endl;

    // Milestone 188: one hand-authored wall layout per encounter-capable
    // overworld terrain -- see docs/COMBAT_NOTES.md. ansalon_rpg has no
    // equivalent load, and never will (its 15x9 console combat grid has no
    // wall concept) -- this catalog and kCombatGridWidth/Height are both
    // ansalon_sfml_phase1-only.
    if (!drawLoadingScreen("Loading battlemaps...")) return 0;
    world::BattleMapCatalog battleMaps =
        world::BattleMapCatalog::loadAll("data/battlemaps", kCombatGridWidth, kCombatGridHeight);
    std::cout << "step 2c2: battlemaps loaded" << std::endl;

    if (!drawLoadingScreen("Loading timeline...")) return 0;
    timeline::Timeline timeline;
    timeline::TimelineLoader::loadFromFile("data/timeline.txt", timeline);
    std::cout << "step 2d: timeline loaded" << std::endl;

    // Static content too, loaded after zones so every zone's QUEST/
    // SHOP_LOCKED ids can be cross-checked against it below -- mirrors
    // src/main.cpp's own loading order (quest::QuestLoader itself can't see
    // zones, see docs/QUEST_NOTES.md).
    if (!drawLoadingScreen("Loading quests...")) return 0;
    quest::QuestCatalog quests;
    quest::QuestLoader::loadFromFile("data/quests.txt", quests);
    std::cout << "step 2e: quests loaded, " << quests.size() << " entries" << std::endl;

    // A zone's QUEST <char> <quest-id> / SHOP_LOCKED <char> <quest-id> line
    // is validated by ZoneLoader only against its own POI/TALK/SHOP grammar
    // (it can't see quest::QuestCatalog) -- so a bad quest id would
    // otherwise fail silently at play time. Cross-checked here, the same
    // place and same fail-fast shape src/main.cpp's own copy of this check
    // uses.
    for (const auto& [zoneId, zone] : zones.allZones()) {
        for (const auto& [code, questId] : zone.quests()) {
            if (quests.find(questId) == nullptr) {
                std::cerr << "Zone '" << zoneId << "' offers quest '" << questId << "' at POI '" << code
                          << "', but no such quest is defined in data/quests.txt.\n";
                return 1;
            }
        }
        for (const auto& [code, questId] : zone.shopLocks()) {
            if (quests.find(questId) == nullptr) {
                std::cerr << "Zone '" << zoneId << "' locks the shop at POI '" << code << "' behind quest '"
                          << questId << "', but no such quest is defined in data/quests.txt.\n";
                return 1;
            }
        }
    }

    // ------------------------------------------------------------------
    // No save path given (argc < 2 in main() -- see its own comment) means
    // "show the save-slot menu." Both this and the character creation
    // wizard below run as their own blocking event loops using the window/
    // font/drawPickerOverlay already set up above, before game::GameState
    // exists at all -- the same shape character::CharacterCreator::run()
    // itself is (one big self-contained blocking function in the console
    // build), just driven by real sf::Event polling instead of std::cin.
    // Ports src/main.cpp's save-slot menu (describeSlot/slotLabel/the
    // slot-picker loop, lines 107-380) and CharacterCreator.cpp step by
    // step -- see docs/CURRENT_WORK.md for the full writeup.
    // ------------------------------------------------------------------
    std::string activeSavePath = savePath;
    game::GameState state;
    if (activeSavePath.empty()) {
        struct SlotInfo {
            std::string path;
            bool exists = false;
            bool valid = false;
            game::GameState state;
            std::string summary;
            std::string error;
        };
        auto describeSlot = [&](std::string path) {
            SlotInfo info;
            info.exists = game::SaveGame::exists(path);
            info.path = std::move(path);
            if (!info.exists) return info;
            try {
                game::GameState loaded = game::SaveGame::load(info.path);
                if (loaded.mode == game::Mode::Zone && !zones.hasZone(loaded.currentZoneId)) {
                    throw std::runtime_error("references a zone ('" + loaded.currentZoneId +
                                              "') that no longer exists");
                }
                const character::Character& c = loaded.character;
                info.summary = c.name + ", level " + std::to_string(c.level) + " " +
                               std::string(character::raceInfo(c.race).name) + " " +
                               std::string(character::classInfo(c.charClass).name) + " (Day " +
                               std::to_string(loaded.hoursElapsed / 24) + ")";
                info.state = std::move(loaded);
                info.valid = true;
            } catch (const std::exception& ex) {
                info.error = ex.what();
            }
            return info;
        };
        auto slotLabel = [](const SlotInfo& slot) -> std::string {
            if (!slot.exists) return "(empty)";
            if (slot.valid) return slot.summary;
            return "(unreadable save: " + slot.error + ")";
        };

        std::array<SlotInfo, 3> slots = {describeSlot("save1.txt"), describeSlot("save2.txt"),
                                          describeSlot("save3.txt")};

        enum class SlotStep { List, ConfirmContinue, ConfirmOverwrite, ConfirmDelete };
        SlotStep slotStep = SlotStep::List;
        int slotCursor = 0;
        int confirmCursor = 0;
        bool creatingNew = false;
        bool slotChosen = false;

        while (!slotChosen) {
            while (const std::optional<sf::Event> event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                    const sf::Keyboard::Key key = keyPressed->code;
                    const bool wantsUp = key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::W;
                    const bool wantsDown = key == sf::Keyboard::Key::Down || key == sf::Keyboard::Key::S;
                    const bool wantsBack = key == sf::Keyboard::Key::Escape || key == sf::Keyboard::Key::Q;
                    const bool wantsEnter = key == sf::Keyboard::Key::Enter;
                    if (slotStep == SlotStep::List) {
                        if (wantsUp) {
                            slotCursor = (slotCursor + 2) % 3;
                        } else if (wantsDown) {
                            slotCursor = (slotCursor + 1) % 3;
                        } else if (key == sf::Keyboard::Key::D) {
                            confirmCursor = 1;
                            slotStep = SlotStep::ConfirmDelete;
                        } else if (wantsBack) {
                            window.close();
                        } else if (wantsEnter) {
                            SlotInfo& slot = slots[static_cast<size_t>(slotCursor)];
                            if (!slot.exists) {
                                creatingNew = true;
                                slotChosen = true;
                            } else if (slot.valid) {
                                confirmCursor = 0;
                                slotStep = SlotStep::ConfirmContinue;
                            } else {
                                confirmCursor = 1;
                                slotStep = SlotStep::ConfirmOverwrite;
                            }
                        }
                    } else if (slotStep == SlotStep::ConfirmContinue) {
                        if (wantsUp || wantsDown) {
                            confirmCursor = 1 - confirmCursor;
                        } else if (wantsBack) {
                            slotStep = SlotStep::List;
                        } else if (wantsEnter) {
                            if (confirmCursor == 0) {
                                state = std::move(slots[static_cast<size_t>(slotCursor)].state);
                                creatingNew = false;
                                slotChosen = true;
                            } else {
                                confirmCursor = 1;
                                slotStep = SlotStep::ConfirmOverwrite;
                            }
                        }
                    } else if (slotStep == SlotStep::ConfirmOverwrite) {
                        if (wantsUp || wantsDown) {
                            confirmCursor = 1 - confirmCursor;
                        } else if (wantsBack) {
                            slotStep = SlotStep::List;
                        } else if (wantsEnter) {
                            if (confirmCursor == 0) {
                                creatingNew = true;
                                slotChosen = true;
                            } else {
                                slotStep = SlotStep::List;
                            }
                        }
                    } else if (slotStep == SlotStep::ConfirmDelete) {
                        if (wantsUp || wantsDown) {
                            confirmCursor = 1 - confirmCursor;
                        } else if (wantsBack) {
                            slotStep = SlotStep::List;
                        } else if (wantsEnter) {
                            if (confirmCursor == 0) {
                                game::SaveGame::remove(slots[static_cast<size_t>(slotCursor)].path);
                                slots[static_cast<size_t>(slotCursor)] =
                                    describeSlot(slots[static_cast<size_t>(slotCursor)].path);
                            }
                            slotStep = SlotStep::List;
                        }
                    }
                }
            }
            if (!window.isOpen()) return 0;

            window.clear();
            switch (slotStep) {
                case SlotStep::List: {
                    std::vector<std::string> items;
                    for (int i = 0; i < 3; ++i) {
                        items.push_back("Slot " + std::to_string(i + 1) + ": " +
                                         slotLabel(slots[static_cast<size_t>(i)]));
                    }
                    drawPickerOverlay("Ansalon: Age of Despair -- Save Slots", items, slotCursor,
                                       "up/down=select   Enter=choose   d=delete highlighted   q=quit");
                    break;
                }
                case SlotStep::ConfirmContinue:
                    drawPickerOverlay("Continue " + slots[static_cast<size_t>(slotCursor)].state.character.name +
                                           "?",
                                       {"Yes", "No"}, confirmCursor, "Enter=confirm   Esc=back");
                    break;
                case SlotStep::ConfirmOverwrite:
                    drawPickerOverlay("Start a new character in Slot " + std::to_string(slotCursor + 1) +
                                           "? This will overwrite any character already there.",
                                       {"Yes", "No"}, confirmCursor, "Enter=confirm   Esc=back");
                    break;
                case SlotStep::ConfirmDelete:
                    drawPickerOverlay("Delete Slot " + std::to_string(slotCursor + 1) + " -- " +
                                           slotLabel(slots[static_cast<size_t>(slotCursor)]) +
                                           "? This cannot be undone.",
                                       {"Yes", "No"}, confirmCursor, "Enter=confirm   Esc=back");
                    break;
            }
            window.display();
        }

        activeSavePath = slots[static_cast<size_t>(slotCursor)].path;
        character::Character newCharacter;

        if (creatingNew) {
            // ---- Character creation wizard --------------------------------
            // Ports character::CharacterCreator::run() (CharacterCreator.cpp,
            // 521 lines) step by step: every character:: rule function it
            // calls (eligibility checks, adjustments, HP/AC/THAC0/saves/steel
            // formulas) is reused completely unchanged, only the presentation
            // (std::cin/std::cout -> real sf::Event-driven pickers) differs.
            constexpr std::array<character::Ability, 6> kCreationAbilityOrder = {
                character::Ability::Strength,     character::Ability::Dexterity,
                character::Ability::Constitution, character::Ability::Intelligence,
                character::Ability::Wisdom,       character::Ability::Charisma,
            };
            auto creationRaceIsSelectable = [](character::RaceId race, const character::AbilityScores& sc) {
                if (race == character::RaceId::Elf) {
                    for (character::SubraceId sub : character::kElfSubraces) {
                        if (character::meetsSubraceAbilityRange(sub, sc)) return true;
                    }
                    return false;
                }
                if (race == character::RaceId::Dwarf) {
                    for (character::SubraceId sub : character::kDwarfSubraces) {
                        if (character::meetsSubraceAbilityRange(sub, sc)) return true;
                    }
                    return false;
                }
                return character::meetsAbilityRange(race, sc);
            };
            // Verbatim port of CharacterCreator.cpp:446-473 -- the pure-logic
            // block with no UI of its own, called once the class/race/scores
            // are all final, right before the Summary step.
            auto computeDerivedStats = [](character::Character& ch) {
                const character::ClassInfo& chosenClass = character::classInfo(ch.charClass);
                bool isWarrior = ch.charClass == character::ClassId::Fighter;
                ch.maxHp = std::max(1, chosenClass.hitDieSides +
                                            character::hpAdjustmentForConstitution(ch.scores.constitution, isWarrior));
                ch.currentHp = ch.maxHp;
                if (ch.race == character::RaceId::Kender) {
                    ch.weaponName = character::kHoopakName;
                    ch.weaponDamageSides = character::kHoopakDamageSides;
                    ch.weaponDamageBonus = character::kHoopakDamageBonus;
                } else {
                    ch.weaponName = chosenClass.weaponName;
                    ch.weaponDamageSides = chosenClass.weaponDamageSides;
                    ch.weaponDamageBonus = 0;
                }
                character::recomputeArmorClass(ch);
                ch.thac0 = 20;
                ch.saves = chosenClass.level1Saves;
                character::applyRacialSavingThrowBonus(ch.race, ch.scores.constitution, ch.saves);
                ch.steelPieces = (character::roll(chosenClass.steelDiceCount, chosenClass.steelDiceSides) +
                                   chosenClass.steelFlatBonus) *
                                  chosenClass.steelMultiplier;
            };

            enum class CreationStep {
                Name,
                RollPool,
                AssignAbility,
                PickRace,
                PickSubrace,
                RaceAdjustments,
                PickClass,
                ClassForcedTinker,
                PickAlignment,
                KnightOffer,
                Specialization,
                Summary,
            };

            // Gold-Box-style letter selection (DQoK-inspired pass, see
            // docs/MILESTONES.md): a lookup table + linear search rather
            // than arithmetic on the sf::Keyboard::Key enum, matching this
            // file's existing per-key switch idiom elsewhere (e.g. the
            // movement-key switch). Returns 0-25 for A-Z, -1 otherwise.
            constexpr std::array<sf::Keyboard::Key, 26> kLetterKeys = {
                sf::Keyboard::Key::A, sf::Keyboard::Key::B, sf::Keyboard::Key::C, sf::Keyboard::Key::D,
                sf::Keyboard::Key::E, sf::Keyboard::Key::F, sf::Keyboard::Key::G, sf::Keyboard::Key::H,
                sf::Keyboard::Key::I, sf::Keyboard::Key::J, sf::Keyboard::Key::K, sf::Keyboard::Key::L,
                sf::Keyboard::Key::M, sf::Keyboard::Key::N, sf::Keyboard::Key::O, sf::Keyboard::Key::P,
                sf::Keyboard::Key::Q, sf::Keyboard::Key::R, sf::Keyboard::Key::S, sf::Keyboard::Key::T,
                sf::Keyboard::Key::U, sf::Keyboard::Key::V, sf::Keyboard::Key::W, sf::Keyboard::Key::X,
                sf::Keyboard::Key::Y, sf::Keyboard::Key::Z,
            };
            auto letterIndexForKey = [&](sf::Keyboard::Key k) -> int {
                for (int i = 0; i < static_cast<int>(kLetterKeys.size()); ++i) {
                    if (kLetterKeys[static_cast<size_t>(i)] == k) return i;
                }
                return -1;
            };

            bool creationComplete = false;
            while (!creationComplete) {
                CreationStep step = CreationStep::Name;
                std::string nameBuffer;
                std::vector<int> pool;
                character::AbilityScores scores;
                character::AbilityScores beforeRaceAdjustments;
                int assignIndex = 0;
                int listCursor = 0;
                int confirmCursor2 = 0;
                std::string stepMessage;
                character::Character character; // this attempt's in-progress character

                // Gold-Box-style persistent stat sidebar (DQoK-inspired
                // pass). Built fresh each frame from the in-progress
                // character/scores/step locals above, so it live-updates
                // as choices are made without duplicating any state. Race
                // shows the base race name while still on PickSubrace,
                // upgrading to the specific subrace once past it; Class is
                // gated on ClassForcedTinker's ordinal rather than
                // PickClass's so it correctly covers both the normal and
                // Gnome-forced-Tinker paths (both land on PickAlignment
                // next). CreationStep's declaration order matches the
                // wizard's flow, so these relational comparisons hold even
                // for paths that skip a step (e.g. non-Elf/Dwarf skipping
                // PickSubrace).
                auto buildSidebarLines = [&]() {
                    std::vector<std::string> lines;
                    lines.push_back(character.name);
                    lines.push_back("STR " + std::to_string(scores.strength));
                    lines.push_back("DEX " + std::to_string(scores.dexterity));
                    lines.push_back("CON " + std::to_string(scores.constitution));
                    lines.push_back("INT " + std::to_string(scores.intelligence));
                    lines.push_back("WIS " + std::to_string(scores.wisdom));
                    lines.push_back("CHA " + std::to_string(scores.charisma));
                    if (step > CreationStep::PickRace) {
                        const character::SubraceInfo* sub = character::subraceInfo(character.subrace);
                        lines.push_back(std::string("Race: ") +
                                         (sub != nullptr && step > CreationStep::PickSubrace
                                              ? sub->name
                                              : character::raceInfo(character.race).name));
                    }
                    if (step > CreationStep::ClassForcedTinker) {
                        lines.push_back(std::string("Class: ") + character::classInfo(character.charClass).name);
                    }
                    if (step > CreationStep::PickAlignment) {
                        lines.push_back(std::string("Alignment: ") + character::alignmentName(character.alignment));
                    }
                    if (character.knightOrder != character::KnightOrder::None) {
                        lines.push_back(std::string(character::knightOrderName(character.knightOrder)));
                    }
                    return lines;
                };

                // Wizard-local overlay: the same title/items/footer/message
                // frame drawPickerOverlay draws (shared with 26 other call
                // sites, deliberately left untouched here), plus two
                // additions only this wizard needs: A)/B)/C)... letters on
                // the selectable range [letterStart, items.size()) -- -1
                // means no lettering -- and an optional persistent stat
                // sidebar. Kept as a separate lambda rather than extending
                // drawPickerOverlay itself so no other screen's layout is
                // affected. Letters assume at most 26 items and, for
                // legible A-Z single-letter labels, realistically far fewer
                // -- true of every wizard list (Alignment, the largest, has
                // 9).
                constexpr float kCreationSidebarWidth = 280.f;
                auto drawCreationOverlay = [&](const std::string& title, const std::vector<std::string>& items,
                                                int selectedIndex, const std::string& footer,
                                                const std::string& message, int letterStart, bool showSidebar) {
                    drawPanelChrome();

                    float contentX = kSheetMarginX;
                    float contentWidthPx = static_cast<float>(windowW) - 2.f * kSheetMarginX;
                    if (showSidebar) {
                        contentX = kCreationSidebarWidth + kSheetMarginX;
                        contentWidthPx = static_cast<float>(windowW) - contentX - kSheetMarginX;

                        sf::RectangleShape divider(sf::Vector2f(2.f, static_cast<float>(windowH) - 24.f));
                        divider.setPosition(sf::Vector2f(kCreationSidebarWidth, 12.f));
                        divider.setFillColor(kPanelBorderInner);
                        window.draw(divider);

                        // Wrapped to fit inside the sidebar column, not drawn
                        // raw -- a long line ("Alignment: Lawful Good",
                        // "Knight of the Crown") at the previous unwrapped
                        // width ran straight past the divider and overlapped
                        // the main panel's own text on the same row (seen
                        // live on the Summary screen, a Knight character).
                        // Same wrapToPixelWidth primitive drawLine below
                        // already uses for the main content.
                        float sy = 40.f;
                        const float kSidebarTextWidthPx = kCreationSidebarWidth - kSheetMarginX - 12.f;
                        auto drawSideLine = [&](const std::string& text, sf::Color color, unsigned size) {
                            for (const std::string& wrappedLine :
                                 wrapToPixelWidth(font, size, text, kSidebarTextWidthPx)) {
                                sf::Text sfText(font, wrappedLine, size);
                                sfText.setFillColor(color);
                                sfText.setPosition(sf::Vector2f(kSheetMarginX, sy));
                                window.draw(sfText);
                                sy += static_cast<float>(size) + 8.f;
                            }
                        };
                        drawSideLine("Character", kPanelHeaderColor, kSheetHeaderCharSize);
                        sy += 6.f;
                        for (const std::string& line : buildSidebarLines()) {
                            drawSideLine(line, kSheetBodyColor, kSheetBodyCharSize);
                        }
                    }

                    // "A) " is wider than the plain "> " prefix
                    // drawPickerOverlay uses, so lettered items wrap a bit
                    // narrower -- measured directly, same idiom
                    // drawPickerOverlay itself uses for its own prefix.
                    const float letterPrefixWidthPx = sf::Text(font, "A) ", kSheetBodyCharSize).getLocalBounds().size.x;

                    float y = 40.f;
                    auto drawLine = [&](const std::string& text, sf::Color color, unsigned size) {
                        sf::Text sfText(font, text, size);
                        sfText.setFillColor(color);
                        sfText.setPosition(sf::Vector2f(contentX, y));
                        window.draw(sfText);
                        y += static_cast<float>(size) + 10.f;
                    };

                    for (const std::string& line : wrapToPixelWidth(font, kSheetTitleCharSize, title, contentWidthPx)) {
                        drawLine(line, kPanelHeaderColor, kSheetTitleCharSize);
                    }
                    y += 10.f;
                    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
                        const bool lettered = letterStart >= 0 && i >= letterStart;
                        const bool isSelected = i == selectedIndex;
                        const float wrapWidth = lettered ? contentWidthPx - letterPrefixWidthPx : contentWidthPx;
                        std::vector<std::string> wrapped =
                            wrapToPixelWidth(font, kSheetBodyCharSize, items[static_cast<size_t>(i)], wrapWidth);
                        if (wrapped.empty()) wrapped.push_back("");
                        for (size_t lineIdx = 0; lineIdx < wrapped.size(); ++lineIdx) {
                            std::string prefix;
                            if (lineIdx == 0 && lettered) {
                                prefix = std::string(1, static_cast<char>('A' + (i - letterStart))) + ") ";
                            } else if (lettered) {
                                prefix = "   ";
                            } else {
                                prefix = "  ";
                            }
                            drawLine(prefix + wrapped[lineIdx], isSelected ? sf::Color::White : kSheetBodyColor,
                                      kSheetBodyCharSize);
                        }
                    }
                    if (!message.empty()) {
                        y += 10.f;
                        for (const std::string& line : wrapToPixelWidth(font, kSheetBodyCharSize, message, contentWidthPx)) {
                            drawLine(line, sf::Color::White, kSheetBodyCharSize);
                        }
                    }
                    y += 10.f;
                    drawLine(footer, sf::Color(150, 150, 160), kSheetHeaderCharSize);
                };

                bool stepDone = false;
                while (!stepDone) {
                    while (const std::optional<sf::Event> event = window.pollEvent()) {
                        if (event->is<sf::Event::Closed>()) {
                            window.close();
                        }
                        if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
                            if (step == CreationStep::Name && textEntered->unicode >= 0x20 &&
                                textEntered->unicode < 0x7F && nameBuffer.size() < 20) {
                                nameBuffer.push_back(static_cast<char>(textEntered->unicode));
                            }
                        }
                        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                            const sf::Keyboard::Key key = keyPressed->code;
                            const bool wantsUp = key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::W;
                            const bool wantsDown = key == sf::Keyboard::Key::Down || key == sf::Keyboard::Key::S;
                            const bool wantsBack = key == sf::Keyboard::Key::Escape || key == sf::Keyboard::Key::Q;
                            const bool wantsEnter = key == sf::Keyboard::Key::Enter;
                            // Gold-Box-style direct letter selection (DQoK-
                            // inspired pass) -- -1 when the key isn't A-Z.
                            // Never collides with wantsBack (Q, index 16,
                            // is handled below before any per-step letter
                            // check runs) or with the W/S Up/Down bindings
                            // (indices 22/18), since every wizard list here
                            // has at most 9 items (Alignment, the largest).
                            const int letterIdx = letterIndexForKey(key);

                            // Escape/Q abandons creation entirely (closes the
                            // window) at every step -- there's no "cancel back
                            // to the slot menu" in this wizard, same as the
                            // console build's CharacterCreator::run() has no
                            // mid-wizard cancel either. Backspace during Name
                            // is a distinct, ordinary edit key, not this.
                            if (wantsBack) {
                                window.close();
                            } else {
                                switch (step) {
                                case CreationStep::Name:
                                    if (key == sf::Keyboard::Key::Backspace && !nameBuffer.empty()) {
                                        nameBuffer.pop_back();
                                    } else if (wantsEnter) {
                                        if (nameBuffer.empty()) {
                                            stepMessage = "Please enter a name before continuing.";
                                        } else {
                                            stepMessage.clear();
                                            character.name = nameBuffer;
                                            pool.clear();
                                            for (int i = 0; i < 6; ++i) pool.push_back(character::roll4d6DropLowest());
                                            confirmCursor2 = 0;
                                            step = CreationStep::RollPool;
                                        }
                                    }
                                    break;
                                case CreationStep::RollPool:
                                    if (wantsUp || wantsDown) {
                                        confirmCursor2 = 1 - confirmCursor2;
                                    } else if (wantsEnter || (letterIdx >= 0 && letterIdx < 2)) {
                                        if (letterIdx >= 0 && letterIdx < 2) confirmCursor2 = letterIdx;
                                        if (confirmCursor2 == 0) {
                                            scores = character::AbilityScores{};
                                            assignIndex = 0;
                                            listCursor = 0;
                                            step = CreationStep::AssignAbility;
                                        } else {
                                            pool.clear();
                                            for (int i = 0; i < 6; ++i) pool.push_back(character::roll4d6DropLowest());
                                            confirmCursor2 = 0;
                                        }
                                    }
                                    break;
                                case CreationStep::AssignAbility:
                                    if (wantsUp) {
                                        listCursor = (listCursor + static_cast<int>(pool.size()) - 1) %
                                                     static_cast<int>(pool.size());
                                    } else if (wantsDown) {
                                        listCursor = (listCursor + 1) % static_cast<int>(pool.size());
                                    } else if (wantsEnter ||
                                               (letterIdx >= 0 && letterIdx < static_cast<int>(pool.size()))) {
                                        if (letterIdx >= 0 && letterIdx < static_cast<int>(pool.size())) {
                                            listCursor = letterIdx;
                                        }
                                        character::Ability ability =
                                            kCreationAbilityOrder[static_cast<size_t>(assignIndex)];
                                        scores.adjust(ability, pool[static_cast<size_t>(listCursor)]);
                                        pool.erase(pool.begin() + listCursor);
                                        ++assignIndex;
                                        listCursor = 0;
                                        if (assignIndex >= 6) step = CreationStep::PickRace;
                                    }
                                    break;
                                case CreationStep::PickRace:
                                    if (wantsUp) {
                                        listCursor = (listCursor + 5) % 6;
                                    } else if (wantsDown) {
                                        listCursor = (listCursor + 1) % 6;
                                    } else if (wantsEnter || (letterIdx >= 0 && letterIdx < 6)) {
                                        if (letterIdx >= 0 && letterIdx < 6) listCursor = letterIdx;
                                        character::RaceId candidate =
                                            character::kAllRaces[static_cast<size_t>(listCursor)];
                                        if (!creationRaceIsSelectable(candidate, scores)) {
                                            stepMessage = "Your rolled ability scores don't meet " +
                                                          std::string(character::raceInfo(candidate).name) +
                                                          "'s requirements. Choose a different race.";
                                        } else {
                                            character.race = candidate;
                                            stepMessage.clear();
                                            listCursor = 0;
                                            if (candidate == character::RaceId::Elf ||
                                                candidate == character::RaceId::Dwarf) {
                                                step = CreationStep::PickSubrace;
                                            } else {
                                                beforeRaceAdjustments = scores;
                                                character::applyRacialOrSubracialAdjustments(
                                                    character.race, character.subrace, scores);
                                                character.scores = scores;
                                                step = CreationStep::RaceAdjustments;
                                            }
                                        }
                                    }
                                    break;
                                case CreationStep::PickSubrace: {
                                    const std::vector<character::SubraceId> subraceList =
                                        character.race == character::RaceId::Elf
                                            ? std::vector<character::SubraceId>(character::kElfSubraces.begin(),
                                                                                  character::kElfSubraces.end())
                                            : std::vector<character::SubraceId>(character::kDwarfSubraces.begin(),
                                                                                  character::kDwarfSubraces.end());
                                    int count = static_cast<int>(subraceList.size());
                                    if (wantsUp) {
                                        listCursor = (listCursor + count - 1) % count;
                                    } else if (wantsDown) {
                                        listCursor = (listCursor + 1) % count;
                                    } else if (wantsEnter || (letterIdx >= 0 && letterIdx < count)) {
                                        if (letterIdx >= 0 && letterIdx < count) listCursor = letterIdx;
                                        character::SubraceId candidate = subraceList[static_cast<size_t>(listCursor)];
                                        if (!character::meetsSubraceAbilityRange(candidate, scores)) {
                                            stepMessage = "Your rolled ability scores don't meet " +
                                                          std::string(character::subraceInfo(candidate)->name) +
                                                          "'s requirements. Choose a different one.";
                                        } else {
                                            character.subrace = candidate;
                                            stepMessage.clear();
                                            beforeRaceAdjustments = scores;
                                            character::applyRacialOrSubracialAdjustments(character.race,
                                                                                           character.subrace, scores);
                                            character.scores = scores;
                                            step = CreationStep::RaceAdjustments;
                                        }
                                    }
                                    break;
                                }
                                case CreationStep::RaceAdjustments:
                                    if (character.race == character::RaceId::Gnome) {
                                        character.charClass = character::ClassId::Tinker;
                                        step = CreationStep::ClassForcedTinker;
                                    } else {
                                        listCursor = 0;
                                        step = CreationStep::PickClass;
                                    }
                                    break;
                                case CreationStep::PickClass:
                                    if (wantsUp) {
                                        listCursor = (listCursor + 3) % 4;
                                    } else if (wantsDown) {
                                        listCursor = (listCursor + 1) % 4;
                                    } else if (wantsEnter || (letterIdx >= 0 && letterIdx < 4)) {
                                        if (letterIdx >= 0 && letterIdx < 4) listCursor = letterIdx;
                                        character::ClassId candidate =
                                            character::kAllClasses[static_cast<size_t>(listCursor)];
                                        if (character::classLevelCap(character.race, character.subrace, candidate) ==
                                            0) {
                                            const char* who = character::subraceInfo(character.subrace) != nullptr
                                                                  ? character::subraceInfo(character.subrace)->name
                                                                  : character::raceInfo(character.race).name;
                                            stepMessage = std::string(character::classInfo(candidate).name) +
                                                          " is not open to a " + who +
                                                          " character. Choose a different class.";
                                        } else {
                                            character.charClass = candidate;
                                            stepMessage.clear();
                                            if (character.charClass == character::ClassId::Fighter &&
                                                scores.strength == 18) {
                                                character.exceptionalStrengthPercentile = character::roll(1, 100);
                                            }
                                            listCursor = 0;
                                            step = CreationStep::PickAlignment;
                                        }
                                    }
                                    break;
                                case CreationStep::ClassForcedTinker:
                                    listCursor = 0;
                                    step = CreationStep::PickAlignment;
                                    break;
                                case CreationStep::PickAlignment:
                                    if (wantsUp) {
                                        listCursor = (listCursor + 8) % 9;
                                    } else if (wantsDown) {
                                        listCursor = (listCursor + 1) % 9;
                                    } else if (wantsEnter || (letterIdx >= 0 && letterIdx < 9)) {
                                        if (letterIdx >= 0 && letterIdx < 9) listCursor = letterIdx;
                                        character::Alignment candidate =
                                            static_cast<character::Alignment>(listCursor);
                                        if (!character::meetsAlignmentRestriction(character.race, candidate)) {
                                            stepMessage =
                                                "Kender cannot be of evil alignment. Choose a different alignment.";
                                        } else {
                                            character.alignment = candidate;
                                            stepMessage.clear();
                                            confirmCursor2 = 0;
                                            if (character.charClass == character::ClassId::Fighter &&
                                                character::meetsKnightOfCrownRequirements(
                                                    character.race, character.subrace, scores, character.alignment)) {
                                                step = CreationStep::KnightOffer;
                                            } else if (character.charClass == character::ClassId::Fighter) {
                                                step = CreationStep::Specialization;
                                            } else {
                                                computeDerivedStats(character);
                                                step = CreationStep::Summary;
                                            }
                                        }
                                    }
                                    break;
                                case CreationStep::KnightOffer:
                                    if (wantsUp || wantsDown) {
                                        confirmCursor2 = 1 - confirmCursor2;
                                    } else if (wantsEnter || (letterIdx >= 0 && letterIdx < 2)) {
                                        if (letterIdx >= 0 && letterIdx < 2) confirmCursor2 = letterIdx;
                                        if (confirmCursor2 == 0) character.knightOrder = character::KnightOrder::Crown;
                                        confirmCursor2 = 0;
                                        step = CreationStep::Specialization;
                                    }
                                    break;
                                case CreationStep::Specialization:
                                    if (wantsUp || wantsDown) {
                                        confirmCursor2 = 1 - confirmCursor2;
                                    } else if (wantsEnter || (letterIdx >= 0 && letterIdx < 2)) {
                                        if (letterIdx >= 0 && letterIdx < 2) confirmCursor2 = letterIdx;
                                        if (confirmCursor2 == 0) character.specializedWeapon = true;
                                        computeDerivedStats(character);
                                        confirmCursor2 = 0;
                                        step = CreationStep::Summary;
                                    }
                                    break;
                                case CreationStep::Summary:
                                    if (wantsUp || wantsDown) {
                                        confirmCursor2 = 1 - confirmCursor2;
                                    } else if (wantsEnter || (letterIdx >= 0 && letterIdx < 2)) {
                                        if (letterIdx >= 0 && letterIdx < 2) confirmCursor2 = letterIdx;
                                        if (confirmCursor2 == 0) {
                                            newCharacter = character;
                                            creationComplete = true;
                                        }
                                        stepDone = true;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    if (!window.isOpen()) return 0;
                    if (stepDone) break;

                    window.clear();
                    if (step == CreationStep::Name) {
                        drawPanelChrome();
                        float y = 40.f;
                        auto drawLine = [&](const std::string& text, sf::Color color, unsigned size) {
                            sf::Text sfText(font, text, size);
                            sfText.setFillColor(color);
                            sfText.setPosition(sf::Vector2f(kSheetMarginX, y));
                            window.draw(sfText);
                            y += static_cast<float>(size) + 10.f;
                        };
                        drawLine("=== Character Creation (2nd Edition AD&D) ===", kPanelHeaderColor,
                                 kSheetTitleCharSize);
                        y += 10.f;
                        drawLine("What is your name, traveler?", kSheetBodyColor, kSheetBodyCharSize);
                        drawLine("> " + nameBuffer + "_", sf::Color::White, kSheetBodyCharSize);
                        if (!stepMessage.empty()) {
                            drawLine(stepMessage, sf::Color::White, kSheetBodyCharSize);
                        }
                        y += 10.f;
                        drawLine("(Enter=confirm, Backspace=edit, up to 20 characters)",
                                 sf::Color(150, 150, 160), kSheetHeaderCharSize);
                    } else {
                        std::vector<std::string> items;
                        std::string title;
                        std::string footer = "up/down or letter=select   Enter=choose";
                        int selectedIndex = listCursor;
                        // -1 = no letters (RaceAdjustments/ClassForcedTinker's
                        // "press any key" info screens); set per-case below
                        // to match each list's selectable range.
                        int letterStart = -1;
                        // Persistent stat sidebar (DQoK-inspired pass) --
                        // shown from race selection onward, once ability
                        // scores are finalized; RollPool/AssignAbility's own
                        // main content already *is* the stat display, so a
                        // sidebar there would just repeat it.
                        bool showSidebar = true;
                        switch (step) {
                        case CreationStep::RollPool: {
                            // Gold Box-style roll screen: each roll on its
                            // own line plus a running total, rather than one
                            // comma-joined line -- the DQoK-inspired ask.
                            showSidebar = false;
                            for (size_t i = 0; i < pool.size(); ++i) {
                                items.push_back("Roll " + std::to_string(i + 1) + ": " + std::to_string(pool[i]));
                            }
                            int total = 0;
                            for (int v : pool) total += v;
                            items.push_back("Total: " + std::to_string(total));
                            items.push_back("Keep these rolls?");
                            items.push_back("Yes");
                            items.push_back("No");
                            title = "Ability Scores";
                            letterStart = static_cast<int>(items.size()) - 2;
                            selectedIndex = letterStart + confirmCursor2;
                            footer = "Enter=confirm (or A/B)";
                            break;
                        }
                        case CreationStep::AssignAbility: {
                            showSidebar = false;
                            title = std::string("Assigning: ") +
                                    character::abilityName(kCreationAbilityOrder[static_cast<size_t>(assignIndex)]);
                            for (int i = 0; i < 6; ++i) {
                                character::Ability shown = kCreationAbilityOrder[static_cast<size_t>(i)];
                                std::string label = std::string(character::abilityName(shown)) + ": " +
                                                     (i < assignIndex ? std::to_string(scores.get(shown)) : "--");
                                items.push_back(label);
                            }
                            letterStart = static_cast<int>(items.size());
                            for (int v : pool) items.push_back(std::to_string(v));
                            selectedIndex = letterStart + listCursor;
                            footer = "up/down or letter=select   Enter=assign";
                            break;
                        }
                        case CreationStep::PickRace:
                            letterStart = 0;
                            title = "Choose a race:";
                            for (character::RaceId r : character::kAllRaces) {
                                std::string label = character::raceInfo(r).name;
                                if (!creationRaceIsSelectable(r, scores)) label += "  (your ability scores don't qualify)";
                                items.push_back(label);
                            }
                            break;
                        case CreationStep::PickSubrace: {
                            letterStart = 0;
                            const std::vector<character::SubraceId> subraceList =
                                character.race == character::RaceId::Elf
                                    ? std::vector<character::SubraceId>(character::kElfSubraces.begin(),
                                                                          character::kElfSubraces.end())
                                    : std::vector<character::SubraceId>(character::kDwarfSubraces.begin(),
                                                                          character::kDwarfSubraces.end());
                            title = character.race == character::RaceId::Elf ? "Choose an elven heritage:"
                                                                              : "Choose a dwarven clan:";
                            for (character::SubraceId sub : subraceList) {
                                std::string label = character::subraceInfo(sub)->name;
                                if (!character::meetsSubraceAbilityRange(sub, scores)) {
                                    label += "  (your ability scores don't qualify)";
                                }
                                items.push_back(label);
                            }
                            break;
                        }
                        case CreationStep::RaceAdjustments: {
                            title = "Race Adjustments";
                            bool any = false;
                            for (character::Ability a : kCreationAbilityOrder) {
                                int before = beforeRaceAdjustments.get(a);
                                int after = character.scores.get(a);
                                if (before == after) continue;
                                any = true;
                                std::string delta = (after > before ? "+" : "") + std::to_string(after - before);
                                items.push_back(std::string(character::abilityName(a)) + ": " +
                                                 std::to_string(before) + " -> " + std::to_string(after) + " (" +
                                                 delta + ")");
                            }
                            if (!any) items.push_back("No adjustments for this race.");
                            selectedIndex = -1;
                            footer = "(press any key to continue)";
                            break;
                        }
                        case CreationStep::PickClass:
                            letterStart = 0;
                            title = "Choose a class:";
                            for (character::ClassId c : character::kAllClasses) {
                                const character::ClassInfo& info = character::classInfo(c);
                                bool qualifies = scores.get(info.primeRequisite) >= info.primeRequisiteMinimum;
                                bool blockedByRace =
                                    character::classLevelCap(character.race, character.subrace, c) == 0;
                                bool mageDexOk = scores.dexterity >= 6;
                                std::string label = info.name;
                                if (blockedByRace) {
                                    label += c == character::ClassId::Mage ? "  (cannot learn arcane magic)"
                                                                            : "  (not eligible for this race)";
                                } else if (c == character::ClassId::Mage && !mageDexOk) {
                                    label += "  (wizardry on Krynn also requires Dexterity 6+)";
                                } else if (!qualifies) {
                                    label += "  (does not meet prime requisite)";
                                }
                                items.push_back(label);
                            }
                            break;
                        case CreationStep::ClassForcedTinker:
                            title = "Class";
                            items = {"As a Gnome, you are a Tinker -- Krynn's gnomes know no other calling."};
                            selectedIndex = -1;
                            footer = "(press any key to continue)";
                            break;
                        case CreationStep::PickAlignment:
                            letterStart = 0;
                            title = "Choose an alignment:";
                            for (int i = 0; i < 9; ++i) {
                                character::Alignment a = static_cast<character::Alignment>(i);
                                std::string label = character::alignmentName(a);
                                if (!character::meetsAlignmentRestriction(character.race, a)) {
                                    label += "  (kender cannot be evil)";
                                }
                                items.push_back(label);
                            }
                            break;
                        case CreationStep::KnightOffer: {
                            // Long descriptive text goes into a
                            // non-selectable leading item instead of the
                            // title -- same idiom Summary already uses --
                            // since drawCreationOverlay's title line is
                            // meant to stay a short heading, not a full
                            // sentence. drawCreationOverlay wraps this item
                            // itself (measured against the real font/window
                            // width), so it isn't pre-wrapped here too.
                            title = "Knights of Solamnia";
                            items.push_back(
                                "You meet the qualifications to be sponsored into the Knights "
                                "of Solamnia as a Knight of the Crown. Swear the oath and join?");
                            items.push_back("Yes");
                            items.push_back("No");
                            letterStart = static_cast<int>(items.size()) - 2;
                            selectedIndex = letterStart + confirmCursor2;
                            footer = "Enter=confirm (or A/B)";
                            break;
                        }
                        case CreationStep::Specialization: {
                            title = "Weapon Specialization";
                            items.push_back(
                                "You may specialize in your weapon, gaining +1 to hit and +2 "
                                "damage with it, plus faster extra attacks as you level. "
                                "Specialize?");
                            items.push_back("Yes");
                            items.push_back("No");
                            letterStart = static_cast<int>(items.size()) - 2;
                            selectedIndex = letterStart + confirmCursor2;
                            footer = "Enter=confirm (or A/B)";
                            break;
                        }
                        case CreationStep::Summary: {
                            title = character.name;
                            const character::SubraceInfo* sub = character::subraceInfo(character.subrace);
                            std::ostringstream line1;
                            line1 << (sub != nullptr ? sub->name : character::raceInfo(character.race).name) << " "
                                  << character::classInfo(character.charClass).name << ", "
                                  << character::alignmentName(character.alignment);
                            items.push_back(line1.str());
                            if (character.knightOrder != character::KnightOrder::None) {
                                items.push_back(character::knightOrderName(character.knightOrder));
                            }
                            if (character.charClass == character::ClassId::Mage) {
                                items.push_back("An unaffiliated student of the arcane -- a Robe and Order of "
                                                 "High Sorcery await at higher levels.");
                            }
                            std::ostringstream scoresLine;
                            scoresLine << "STR " << character.scores.strength << "  DEX " << character.scores.dexterity
                                       << "  CON " << character.scores.constitution << "  INT "
                                       << character.scores.intelligence << "  WIS " << character.scores.wisdom
                                       << "  CHA " << character.scores.charisma;
                            items.push_back(scoresLine.str());
                            if (character.exceptionalStrengthPercentile > 0) {
                                std::ostringstream ex;
                                ex << "(exceptional Strength: 18/"
                                   << (character.exceptionalStrengthPercentile == 100
                                           ? "00"
                                           : (character.exceptionalStrengthPercentile < 10 ? "0" : "") +
                                                 std::to_string(character.exceptionalStrengthPercentile))
                                   << ")";
                                items.push_back(ex.str());
                            }
                            std::ostringstream statLine;
                            statLine << "HP " << character.maxHp << "   AC " << character.armorClass << "   THAC0 "
                                     << character.thac0;
                            items.push_back(statLine.str());
                            items.push_back("Weapon: " + character.weaponName);
                            std::ostringstream savesLine;
                            savesLine << "Saves -- "
                                      << character::saveCategoryName(character::SaveCategory::ParalyzationPoisonDeath)
                                      << ": "
                                      << character.saves.at(character::SaveCategory::ParalyzationPoisonDeath) << "  "
                                      << character::saveCategoryName(character::SaveCategory::RodStaffWand) << ": "
                                      << character.saves.at(character::SaveCategory::RodStaffWand) << "  "
                                      << character::saveCategoryName(character::SaveCategory::PetrificationPolymorph)
                                      << ": "
                                      << character.saves.at(character::SaveCategory::PetrificationPolymorph) << "  "
                                      << character::saveCategoryName(character::SaveCategory::BreathWeapon) << ": "
                                      << character.saves.at(character::SaveCategory::BreathWeapon) << "  "
                                      << character::saveCategoryName(character::SaveCategory::Spell) << ": "
                                      << character.saves.at(character::SaveCategory::Spell);
                            items.push_back(savesLine.str());
                            items.push_back("Steel: " + std::to_string(character.steelPieces) + " stl");
                            items.push_back("Begin your journey as this character?");
                            items.push_back("Yes");
                            items.push_back("No");
                            letterStart = static_cast<int>(items.size()) - 2;
                            selectedIndex = letterStart + confirmCursor2;
                            footer = "Enter=confirm (or A/B)";
                            break;
                        }
                        default:
                            break;
                        }
                        drawCreationOverlay(title, items, selectedIndex, footer, stepMessage, letterStart, showSidebar);
                    }
                    window.display();
                }
            }
        }

        if (creatingNew) {
            // Fresh character -> position at the starting location, same as
            // src/main.cpp:386-391 does for the console build.
            constexpr const char* kCreationStartingLocationId = "solace";
            const world::Location* start = world.getLocation(kCreationStartingLocationId);
            if (!start) {
                std::cerr << "World data does not define the starting location '" << kCreationStartingLocationId
                          << "'.\n";
                return 1;
            }
            state = game::GameState{};
            state.character = newCharacter;
            state.x = start->x;
            state.y = start->y;
            state.visitedLocations.insert(start->id);
        }
        // Written immediately so the save file exists on disk right away,
        // rather than relying on the later in-loop autosave triggers for
        // this first write.
        game::SaveGame::save(state, activeSavePath);
    } else {
        state = game::SaveGame::load(activeSavePath);
    }
    std::cout << "step 3: save loaded -- " << state.character.name << ", level "
              << state.character.level << " " << character::raceInfo(state.character.race).name << " "
              << character::classInfo(state.character.charClass).name << ", at (" << state.x << ", "
              << state.y << ")" << std::endl;

    if (!drawLoadingScreen("Loading world map image...")) return 0;
    sf::Texture mapTexture;
    if (!mapTexture.loadFromFile("References/dragonlancemap2.png")) {
        std::cerr << "Failed to load References/dragonlancemap2.png\n";
        return 1;
    }
    const sf::Vector2u mapSize = mapTexture.getSize();
    std::cout << "step 4: map texture loaded, size " << mapSize.x << "x" << mapSize.y << std::endl;

    const float pxPerTileX = static_cast<float>(mapSize.x) / static_cast<float>(grid.width());
    const float pxPerTileY = static_cast<float>(mapSize.y) / static_cast<float>(grid.height());
    const float worldW = static_cast<float>(mapSize.x);
    const float worldH = static_cast<float>(mapSize.y);

    // Real GameState fields drive position in both modes (state.x/y for
    // Overworld, state.zoneX/zoneY for Zone) -- mutated in memory exactly
    // like game::GameLoop does, and autosaved back to savePath the same
    // way too (see the autosave call sites in the event loop below).
    const world::Zone* currentZone = nullptr;
    if (state.mode == game::Mode::Zone) {
        currentZone = zones.getZone(state.currentZoneId);
        if (!currentZone) {
            std::cerr << "Warning: save's zone '" << state.currentZoneId
                      << "' not found -- falling back to overworld.\n";
            state.mode = game::Mode::Overworld;
        }
    }

    // Zone landmark plate -- same transient-full-window-overlay reasoning
    // as helpOpen/worldMapOpen/journalOpen (declared further below,
    // alongside those), dismissed by any key, but set from the zone-entry
    // code (enterZone/leaveCurrentZone below) rather than a dedicated
    // keybind. Declared here, right after currentZone, rather than beside
    // helpOpen/worldMapOpen, because enterZone/leaveCurrentZone -- defined
    // well before those -- need to capture it. zonePlateTexture/
    // zonePlateZoneName only matter while zonePlateOpen is true (see
    // loadZonePlateTexture above and drawZonePlateOverlay below).
    // zonePlateLoaded (Milestone 206) is independent of zonePlateOpen: it
    // tracks whether the *current* zone has a plate texture loaded at all,
    // needed because a TOWN_MENU zone (see enterZone/drawTownMenuOverlay)
    // never sets zonePlateOpen -- it draws the same texture persistently
    // instead of as a one-shot full-window flourish.
    bool zonePlateOpen = false;
    bool zonePlateLoaded = false;
    sf::Texture zonePlateTexture;
    std::string zonePlateZoneName;

    // Milestone 210: any zone with real plate art now behaves as a
    // Gold-Box menu town, not just one explicitly flagged TOWN_MENU --
    // confirmed with the user rather than adding a per-zone flag to every
    // new art drop. zonePlateLoaded already tracks "does the *current*
    // zone have art" (set once per zone at enterZone/leaveCurrentZone/the
    // Milestone 209 startup-load fix), so this needs no new I/O.
    // Zone::isMenuTown() itself deliberately stays untouched -- Zone/
    // ZoneLoader are shared with the ansalon_rpg console target, which
    // has no concept of assets/, so all art-awareness stays local to this
    // file (see docs/ARCHITECTURE.md's SFML section).
    auto isEffectiveMenuTown = [&]() {
        return currentZone != nullptr && (currentZone->isMenuTown() || zonePlateLoaded);
    };

    // Dialogue portrait (Milestone 208) -- same "screen transition, not
    // world state" reasoning as the zone-plate state just above. Set once
    // per conversation in dialogueStartTalk, stays valid (no reload) for
    // however many topic/ask exchanges that same conversation has.
    bool dialoguePortraitLoaded = false;
    sf::Texture dialoguePortraitTexture;

    // A save can load directly into a zone (MODE ZONE) without ever
    // passing through enterZone -- the only other place zonePlateTexture/
    // zonePlateLoaded get set. Without this, a save that starts (or is
    // continued from the slot menu) already standing inside, say, Solace
    // would show a blank image area on launch until the player physically
    // left and re-entered. Never sets zonePlateOpen -- same "resuming
    // isn't a fresh arrival" reasoning leaveCurrentZone already uses when
    // backing out of a child zone, so an ordinary (non-TOWN_MENU) zone's
    // one-shot full-window flourish doesn't re-fire just because the
    // process restarted.
    if (currentZone != nullptr) {
        if (auto plate = loadZonePlateTexture(state.currentZoneId)) {
            zonePlateTexture = std::move(*plate);
            zonePlateZoneName = currentZone->name();
            zonePlateLoaded = true;
        }
    }

    sf::View mapView(sf::Vector2f(0.f, 0.f), sf::Vector2f(mapWidth, static_cast<float>(windowH)));
    mapView.setViewport(sf::FloatRect({0.f, 0.f}, {mapWidth / static_cast<float>(windowW), 1.f}));

    // Combat-only counterpart to mapView, reserving kCombatBottomBarHeight
    // along the bottom for the new DQoK-style command/message bar (Milestone
    // 199) -- a separate view rather than resizing mapView itself, since
    // mapView is shared by Overworld/Zone too and neither of those gets a
    // bottom bar. The camera-clamp math in combatAnimateAiStep and the main
    // combat draw branch both read this view's own size, so a focused token
    // near the bottom of the 50x25 grid clamps against the real unobstructed
    // height instead of the bar's dead zone.
    sf::View combatMapView(sf::Vector2f(0.f, 0.f),
                            sf::Vector2f(mapWidth, static_cast<float>(windowH) - kCombatBottomBarHeight));
    combatMapView.setViewport(sf::FloatRect(
        {0.f, 0.f}, {mapWidth / static_cast<float>(windowW),
                     (static_cast<float>(windowH) - kCombatBottomBarHeight) / static_cast<float>(windowH)}));

    // uiView itself is declared much earlier now (right after the
    // SW_MAXIMIZE block) so the save-slot menu and character-creation
    // wizard can use it too -- see that declaration for why.

    sf::Sprite mapSprite(mapTexture);

    sf::CircleShape locationMarker(6.f);
    locationMarker.setOrigin(sf::Vector2f(6.f, 6.f));

    sf::CircleShape playerMarker(9.f);
    playerMarker.setOrigin(sf::Vector2f(9.f, 9.f));
    playerMarker.setFillColor(sf::Color(255, 215, 0));

    sf::RectangleShape zoneTileShape(sf::Vector2f(kZoneTilePx, kZoneTilePx));

    PoiIconShapes poiIconShapes(kZoneTilePx);

    sf::CircleShape entryMarker(kZoneTilePx * 0.35f);
    entryMarker.setOrigin(sf::Vector2f(kZoneTilePx * 0.35f, kZoneTilePx * 0.35f));
    entryMarker.setFillColor(sf::Color(60, 200, 220));

    // Combat-only bottom command/message bar background (Milestone 199) --
    // spans the full window width, so it deliberately overlaps the bottom
    // kCombatBottomBarHeight strip of the sidebar column (same fill color,
    // harmless) and the bottom strip combatMapView's shrunk viewport
    // already excludes the map from drawing into.
    sf::RectangleShape combatBottomBarBg(sf::Vector2f(static_cast<float>(windowW), kCombatBottomBarHeight));
    combatBottomBarBg.setPosition(sf::Vector2f(0.f, static_cast<float>(windowH) - kCombatBottomBarHeight));
    combatBottomBarBg.setFillColor(sf::Color(20, 20, 28));

    // Combat grid + marker shapes -- same "declare once, mutate per cell/
    // entity" convention as zoneTileShape/locationMarker above. These stay
    // the fallback presentation for any combatant with no sprite art (see
    // drawCombatSpriteToken/combatSpriteCache below, and
    // docs/ARCHITECTURE.md's SFML section, "Combat sprite art") -- flat
    // color plus a letter label, still used for every companion/monster id
    // besides "bren_alder" and every combatant besides "player".
    sf::RectangleShape combatTileShape(sf::Vector2f(kCombatTilePx - 2.f, kCombatTilePx - 2.f));
    combatTileShape.setFillColor(sf::Color(70, 65, 55));
    // Milestone 188: the one other tile kind this screen now draws -- a
    // wall, from data/battlemaps/*.txt (see CombatSession::battleMap).
    // Same "flat color, no sprite art yet" placeholder philosophy as
    // combatTileShape itself, just visually distinct (darker, cooler) so a
    // wall reads clearly against open floor.
    sf::RectangleShape combatWallTileShape(sf::Vector2f(kCombatTilePx - 2.f, kCombatTilePx - 2.f));
    combatWallTileShape.setFillColor(sf::Color(35, 33, 38));
    sf::RectangleShape combatGridBorder;
    combatGridBorder.setFillColor(sf::Color::Transparent);
    combatGridBorder.setOutlineColor(sf::Color(150, 140, 110));
    combatGridBorder.setOutlineThickness(2.f);
    sf::CircleShape combatMonsterMarker(kCombatTilePx * 0.32f);
    combatMonsterMarker.setOrigin(sf::Vector2f(kCombatTilePx * 0.32f, kCombatTilePx * 0.32f));
    combatMonsterMarker.setFillColor(sf::Color(200, 60, 60));
    sf::CircleShape combatCompanionMarker(kCombatTilePx * 0.32f);
    combatCompanionMarker.setOrigin(sf::Vector2f(kCombatTilePx * 0.32f, kCombatTilePx * 0.32f));
    combatCompanionMarker.setFillColor(sf::Color(80, 150, 220));
    sf::RectangleShape combatPickHighlight(sf::Vector2f(kCombatTilePx - 6.f, kCombatTilePx - 6.f));
    combatPickHighlight.setFillColor(sf::Color::Transparent);
    combatPickHighlight.setOutlineColor(sf::Color(255, 230, 120));
    combatPickHighlight.setOutlineThickness(3.f);

    // Combat sprite art (see docs/ARCHITECTURE.md's SFML section, "Combat
    // sprite art"): an id-keyed lookup, so a combatant with a real
    // assets/sprites/<id>.png (idle | attack side by side) draws that
    // instead of its plain marker+glyph. Cached per id (including a
    // "no art for this id" miss) so a missing file is only ever attempted
    // once per run. "player" and companion id "bren_alder" have art today --
    // combat::Monster::id and the rest of game::RecruitedCompanion::id are
    // the natural future keys, needing no further plumbing here when that
    // art arrives.
    std::unordered_map<std::string, std::optional<sfml_phase1::CombatSpriteFrames>> combatSpriteCache;
    // Returns true if a real sprite was drawn -- false means "no art for
    // this id," and the caller should fall back to its own existing
    // marker+glyph drawing exactly as before (every monster/companion
    // today). `facingTarget` is the grid cell the sprite should mirror
    // toward -- purely cosmetic (see CombatSprite.h's own doc comment):
    // it never feeds combat::oppositeSide's backstab check.
    // neighborFree(dx, dy) tells this whether the adjacent cell one step in
    // that cardinal direction from the token's own anchor is free of any
    // other combatant *this frame* -- only ever queried with |dx|+|dy| == 1
    // (never a diagonal). Companion/player calls below always pass a
    // trivially-true checker since neither has -wide/-tall/-four art yet
    // (so it's never actually consulted); the two monster call sites pass a
    // real one built from combatCellOccupied + the player's own position.
    auto drawCombatSpriteToken = [&](const std::string& spriteId, int footprintWidth, int footprintHeight, float cx,
                                      float cy, combat::GridPos selfGridPos,
                                      const std::optional<combat::GridPos>& facingTarget, bool useAttackPose,
                                      const std::function<bool(int, int)>& neighborFree) -> bool {
        auto it = combatSpriteCache.find(spriteId);
        if (it == combatSpriteCache.end()) {
            it = combatSpriteCache.emplace(spriteId, sfml_phase1::loadCombatSprite(spriteId)).first;
        }
        if (!it->second.has_value()) return false;
        const sfml_phase1::CombatSpriteFrames& frames = *it->second;
        const sf::IntRect& rect = useAttackPose ? frames.attackRect : frames.idleRect;
        // Effective render size in cells: the real gameplay footprint, or
        // the loaded art's own bigger -wide/-tall/-four size when the
        // neighbor cell(s) that extra size would spill into are actually
        // free this frame -- found live: two adjacent wolves' "-wide" art
        // fully overlapping when both were always drawn centered on their
        // own single cell regardless of what stood next to them
        // (2026-09-12). One free side biases the sprite fully toward it
        // (shiftX/shiftY) instead of the old always-centered split. A cell
        // blocked on both sides can't expand at all -- widthBoxedIn/
        // heightBoxedIn below crop the source art itself for that case,
        // rather than squishing the whole oversized frame into a normal
        // 1-cell box (found live: a "-wide" wolf boxed in between two
        // packmates rendered as a visibly tiny wolf, since its native
        // ~2:1 frame squeezed into a square box shrinks far more than a
        // normal creature's roughly-square frame would).
        int effWidth = footprintWidth;
        float shiftX = 0.f;
        bool widthBoxedIn = false;
        if (frames.renderWidth > footprintWidth) {
            const bool leftFree = neighborFree(-1, 0);
            const bool rightFree = neighborFree(1, 0);
            const float extra = kCombatTilePx * static_cast<float>(frames.renderWidth - footprintWidth);
            if (leftFree && rightFree) {
                effWidth = frames.renderWidth;  // centered, same as before this fix
            } else if (rightFree) {
                effWidth = frames.renderWidth;
                shiftX = extra / 2.f;
            } else if (leftFree) {
                effWidth = frames.renderWidth;
                shiftX = -extra / 2.f;
            } else {
                widthBoxedIn = true;
            }
        }
        int effHeight = footprintHeight;
        float shiftY = 0.f;
        bool heightBoxedIn = false;
        if (frames.renderHeight > footprintHeight) {
            const bool upFree = neighborFree(0, -1);
            const bool downFree = neighborFree(0, 1);
            const float extra = kCombatTilePx * static_cast<float>(frames.renderHeight - footprintHeight);
            if (upFree && downFree) {
                effHeight = frames.renderHeight;
            } else if (downFree) {
                effHeight = frames.renderHeight;
                shiftY = extra / 2.f;
            } else if (upFree) {
                effHeight = frames.renderHeight;
                shiftY = -extra / 2.f;
            } else {
                heightBoxedIn = true;
            }
        }
        // Crop a boxed-in axis down to a centered square-ish sub-region
        // (matching a normal creature's roughly-square frame) instead of
        // displaying the full oversized frame -- computed from the
        // original frame's own dimensions on both axes at once so a
        // "-four" sprite boxed in on both axes simultaneously crops
        // correctly rather than compounding.
        sf::IntRect displayRect = rect;
        if (widthBoxedIn) {
            const int squareWidth = std::min(rect.size.x, rect.size.y);
            displayRect.position.x = rect.position.x + (rect.size.x - squareWidth) / 2;
            displayRect.size.x = squareWidth;
        }
        if (heightBoxedIn) {
            const int squareHeight = std::min(rect.size.x, rect.size.y);
            displayRect.position.y = rect.position.y + (rect.size.y - squareHeight) / 2;
            displayRect.size.y = squareHeight;
        }
        sf::Sprite sprite(frames.texture);
        sprite.setTextureRect(displayRect);
        const float frameW = static_cast<float>(displayRect.size.x);
        const float frameH = static_cast<float>(displayRect.size.y);
        sprite.setOrigin(sf::Vector2f(frameW / 2.f, frameH / 2.f));
        // Contain-fit within that bounding box (preserving aspect ratio,
        // 90% fill so it doesn't touch the tile edges) -- same
        // footprint-bounding-box generalization the Milestone 190 marker
        // radius already uses below, just for a rectangular sprite instead
        // of a circle.
        const float boundW = kCombatTilePx * static_cast<float>(effWidth);
        const float boundH = kCombatTilePx * static_cast<float>(effHeight);
        const float scale = 0.9f * std::min(boundW / frameW, boundH / frameH);
        const bool faceLeft = facingTarget.has_value() && sfml_phase1::spriteShouldFaceLeft(selfGridPos, *facingTarget);
        sprite.setScale(sf::Vector2f(faceLeft ? -scale : scale, scale));
        sprite.setPosition(sf::Vector2f(cx + shiftX, cy + shiftY));
        window.draw(sprite);
        return true;
    };

    std::deque<std::string> log;
    auto pushLog = [&log](const std::string& text) {
        log.push_back(text);
        while (log.size() > kMaxLogLines) {
            log.pop_front();
        }
    };

    pushLog("Loaded " + state.character.name + ".");
    if (currentZone) {
        pushLog("Resumed inside " + currentZone->name() + ".");
    }
    pushLog("Ansalon: Age of Despair. Press / for the full command list.");

    // --- Dialogue (core conversation): all state is DialogueSession above;
    // every lambda below is a non-blocking port of the matching piece of
    // GameLoop::handleTalk/talkTo (src/game/GameLoop.cpp:828-1306) -- see
    // this file's top-of-file comment and docs/CURRENT_WORK.md for the
    // scope this covers: greeting/again/aftermath/anticipation/conditional-
    // greeting resolution, the topic picker, GRANTS_ITEM, boat voyage
    // accept/decline, companion recruit accept/decline, free-text ask
    // (AskInput/AskResponse below), and quest offer/turn-in (questBegin and
    // the Quest*/Wayreth* DialogueUiState values below).
    DialogueSession dialogueSession;
    ShopSession shopSession;
    InventorySession inventorySession;
    LogSession logSession;
    RestSession restSession;
    LookSession lookSession;

    // Mirrors GameLoop::handleTalk exactly -- see that function's own
    // comments for why the Overworld/Zone branches differ and what
    // TIMELINE_ANCHOR does.
    auto gatherTalkCandidates = [&]() -> std::vector<DialogueCandidate> {
        std::vector<DialogueCandidate> candidates;
        const long long dayNow = state.hoursElapsed / 24;
        if (state.mode == game::Mode::Overworld) {
            const world::Location* here = world.locationAt(state.x, state.y);
            if (here != nullptr) {
                for (const timeline::Presence& presence : timeline.presentAt(here->id, static_cast<int>(dayNow))) {
                    if (presence.window->dialogue.empty()) continue;
                    DialogueCandidate candidate;
                    candidate.id = presence.character->id;
                    candidate.name = presence.character->name;
                    candidate.speech =
                        speechFromWindow(timeline, *presence.character, *presence.window, static_cast<int>(dayNow));
                    candidates.push_back(std::move(candidate));
                }
            }
        } else if (currentZone != nullptr) {
            const world::PointOfInterest* poi = currentZone->poiAt(state.zoneX, state.zoneY);
            const std::string& effectiveId = currentZone->timelineLocationId().empty()
                                                  ? state.currentZoneId
                                                  : currentZone->timelineLocationId();
            if (poi != nullptr && !poi->dialogue.empty()) {
                DialogueCandidate candidate;
                candidate.id = state.currentZoneId + ":" + std::string(1, poi->code);
                candidate.name = poi->name;
                candidate.speech = speechFromPoi(*poi, static_cast<int>(dayNow));
                candidate.grantsItemId = poi->grantsItemId;
                candidate.grantsItemName = poi->grantsItemName;
                if (const std::string* questId = currentZone->questAt(state.zoneX, state.zoneY)) {
                    candidate.questId = *questId;
                }
                if (const world::BoatVoyage* boat = currentZone->boatAt(state.zoneX, state.zoneY)) {
                    candidate.boatDestinationId = boat->destinationLocationId;
                    candidate.boatHours = boat->hours;
                }
                candidate.recruitCompanionId = poi->recruitCompanionId;
                if (!poi->dialogueAfter.empty()) {
                    int latestDayEnd = timeline.latestDayEnd(effectiveId);
                    if (latestDayEnd >= 0 && dayNow > latestDayEnd) candidate.dialogueAfter = poi->dialogueAfter;
                }
                if (!poi->dialogueBefore.empty()) {
                    int earliestDayStart = timeline.earliestDayStart(effectiveId);
                    if (earliestDayStart >= 0 && dayNow < earliestDayStart) {
                        candidate.dialogueBefore = poi->dialogueBefore;
                    }
                }
                candidates.push_back(std::move(candidate));
            }
            if (poi != nullptr && poi->code == currentZone->timelineAnchorPoi()) {
                for (const timeline::Presence& presence : timeline.presentAt(effectiveId, static_cast<int>(dayNow))) {
                    if (presence.window->dialogue.empty()) continue;
                    DialogueCandidate candidate;
                    candidate.id = presence.character->id;
                    candidate.name = presence.character->name;
                    candidate.speech =
                        speechFromWindow(timeline, *presence.character, *presence.window, static_cast<int>(dayNow));
                    candidates.push_back(std::move(candidate));
                }
            }
        }
        return candidates;
    };

    // Mirrors GameLoop::lookOverworld/lookZone's own candidate gathering
    // (GameLoop.cpp:753-826). Deliberately NOT filtered by
    // presence.window->dialogue.empty() the way gatherTalkCandidates above
    // is -- Look should reveal a presence window's flavor text even if that
    // window has no SAY line authored yet, matching
    // announceOverworldTile/announceZoneTile's own unfiltered iteration (see
    // GameLoop.cpp's Milestone 43 comment on lookOverworld). The POI-itself
    // candidate in the zone branch keeps the dialogue-emptiness filter,
    // same as Talk -- only the presence-window loops (overworld, and the
    // zone's TIMELINE_ANCHOR) drop it.
    auto gatherLookCandidates = [&]() -> std::vector<LookCandidate> {
        std::vector<LookCandidate> candidates;
        const long long dayNow = state.hoursElapsed / 24;
        if (state.mode == game::Mode::Overworld) {
            const world::Location* here = world.locationAt(state.x, state.y);
            if (here != nullptr) {
                for (const timeline::Presence& presence : timeline.presentAt(here->id, static_cast<int>(dayNow))) {
                    candidates.push_back({presence.character->name, presence.window->flavorText});
                }
            }
        } else if (currentZone != nullptr) {
            const world::PointOfInterest* poi = currentZone->poiAt(state.zoneX, state.zoneY);
            if (poi != nullptr && !poi->dialogue.empty()) {
                candidates.push_back({poi->name, poi->description});
            }
            if (poi != nullptr && poi->code == currentZone->timelineAnchorPoi()) {
                const std::string& effectiveId = currentZone->timelineLocationId().empty()
                                                      ? state.currentZoneId
                                                      : currentZone->timelineLocationId();
                for (const timeline::Presence& presence :
                     timeline.presentAt(effectiveId, static_cast<int>(dayNow))) {
                    candidates.push_back({presence.character->name, presence.window->flavorText});
                }
            }
        }
        return candidates;
    };

    // 'L' -- mirrors GameLoop::lookOverworld/lookZone's own dispatch
    // (GameLoop.cpp:753-826): no lookable candidates falls back to a
    // one-line pushLog (nearest-other-location + compass direction on the
    // overworld, a fixed "nothing else" line in a zone, since a zone is
    // always rendered in full already); one or more candidates opens
    // lookSession, skipping straight to showingDetail for the single-
    // candidate case exactly like pickAndLook's own fast path.
    auto lookBegin = [&]() {
        lookSession.candidates = gatherLookCandidates();
        if (lookSession.candidates.empty()) {
            if (state.mode == game::Mode::Overworld) {
                const world::Location* nearest = nullptr;
                long long nearestDistSq = -1;
                for (const world::Location& loc : world.allLocations()) {
                    if (loc.x == state.x && loc.y == state.y) continue; // already described by the status line
                    const long long ddx = loc.x - state.x;
                    const long long ddy = loc.y - state.y;
                    const long long distSq = ddx * ddx + ddy * ddy;
                    if (nearest == nullptr || distSq < nearestDistSq) {
                        nearest = &loc;
                        nearestDistSq = distSq;
                    }
                }
                if (nearest == nullptr) {
                    pushLog("Nothing notable stands out on the horizon.");
                } else {
                    const char* dir = compassDirection(nearest->x - state.x, nearest->y - state.y);
                    pushLog("You reckon " + nearest->name + " lies to the " + dir + ".");
                }
            } else {
                pushLog("Nothing else catches your eye here.");
            }
            return;
        }
        lookSession.selected = 0;
        lookSession.showingDetail = lookSession.candidates.size() == 1;
        lookSession.active = true;
    };

    // Ends the current conversation -- loops back to the "talk to whom?"
    // picker if there was one (mirroring GameLoop::pickAndTalk's own
    // deliberate non-return after talkTo finishes, GameLoop.cpp:924-929),
    // otherwise closes the session outright.
    auto dialogueEnd = [&]() {
        if (dialogueSession.candidates.size() > 1) {
            dialogueSession.uiState = DialogueUiState::PickingCandidate;
        } else {
            dialogueSession.active = false;
        }
    };

    // Whether `companionId` (a RECRUIT POI's recruitCompanionId) is already
    // in the party. Also used by the zone-render loop below to stop drawing
    // a recruited companion's own POI tile -- see its own comment.
    auto companionAlreadyRecruited = [&](const std::string& companionId) {
        return std::any_of(state.companions.begin(), state.companions.end(),
                            [&](const game::RecruitedCompanion& c) { return c.id == companionId; });
    };

    // Whether `candidate`'s recruitCompanionId (if any) is already in the
    // party -- mirrors GameLoop.cpp:1124-1126.
    auto alreadyRecruited = [&](const DialogueCandidate& candidate) {
        return companionAlreadyRecruited(candidate.recruitCompanionId);
    };

    // Whether today's free-text ask limit is used up for `speech` -- same
    // formula as GameLoop.cpp:990-995. A pure query over state.character +
    // speech, safe to call every frame from multiple places (rebuilding the
    // topic picker, the askLimitLocked greeting override).
    auto askLimitExhaustedFor = [&](const DialogueSpeech& speech) -> bool {
        const long long today = state.hoursElapsed / 24;
        const bool sameDay = state.character.lastAstinusAskDay == today;
        const int effectiveLimitToday =
            (sameDay && state.character.astinusDailyLimit > 0) ? state.character.astinusDailyLimit : speech.askLimit;
        return speech.askLimit > 0 && sameDay && state.character.astinusQuestionsToday >= effectiveLimitToday;
    };

    // Builds the "ask <name> about..." picker's labels -- topics, then
    // "Ask about something else..." (only when this candidate still has
    // SUBJECT content left to ask about today), then "Nothing, thanks".
    // Mirrors GameLoop.cpp:1160-1176. Called both when a conversation
    // starts and whenever AskResponse returns to TopicPicker, since a
    // just-answered question may have changed askLimitExhaustedFor's
    // result.
    auto rebuildTopicLabels = [&](const DialogueCandidate& candidate) {
        dialogueSession.topicLabels.clear();
        for (const auto& [label, topicText] : candidate.speech.topics) dialogueSession.topicLabels.push_back(label);
        const bool canAskAnything = !candidate.speech.subjects.empty() && !askLimitExhaustedFor(candidate.speech);
        if (canAskAnything) {
            dialogueSession.askAnythingIndex = static_cast<int>(dialogueSession.topicLabels.size());
            dialogueSession.topicLabels.push_back("Ask about something else...");
        } else {
            dialogueSession.askAnythingIndex = -1;
        }
        dialogueSession.topicLabels.push_back("Nothing, thanks");
        dialogueSession.topicSelected = 0;
    };

    // Copied verbatim from GameLoop::checkQuestReadiness (GameLoop.cpp:
    // 1531-1540) -- promotes every Active quest whose objectives are now all
    // met to ReadyToTurnIn, logging a "ready to turn in" notice for each.
    // Called after every mutation that could satisfy an objective (Visit/
    // Talk/Slay -- see this function's own callers) plus once more after
    // accepting an already-satisfied quest, the same four-call-site shape
    // GameLoop.cpp uses.
    auto checkQuestReadiness = [&]() {
        for (auto& [questId, status] : state.quests) {
            if (status != game::QuestStatus::Active) continue;
            const quest::Quest* q = quests.find(questId);
            if (q == nullptr) continue; // defensive -- startup already cross-validated every zone QUEST id
            if (!allObjectivesMet(*q, state)) continue;
            status = game::QuestStatus::ReadyToTurnIn;
            pushLog(q->name + " is ready to turn in -- return to " + q->giver + " to collect your reward.");
        }
    };

    // Dialogue body-text pagination (feedback after Milestone 210's
    // portrait redesign): the user didn't want a long response (Otik's
    // TALK_AFTER, ~20 wrapped lines) shrinking the portrait to make room
    // for all of it at once -- the portrait should stay the same large
    // size every time, and long text should page instead, "press Enter to
    // see more" the same way AskResponse's own multi-message queue
    // already works, just applied within a single response too.
    // kBodyLinesPerPage is also what fixes the portrait's own size (see
    // drawPortraitFixed in drawDialogueOverlay) -- reserving a constant
    // amount of text space regardless of the current response's actual
    // length is what makes the portrait stop fluctuating.
    constexpr int kBodyLinesPerPage = 6;
    auto paginateBodyText = [&](const std::string& text) {
        const float maxWidthPx = static_cast<float>(windowW) - 2.f * kSheetMarginX;
        const std::vector<std::string> wrapped = wrapToPixelWidth(font, kSheetBodyCharSize, text, maxWidthPx);
        std::vector<std::vector<std::string>> pages;
        for (std::size_t i = 0; i < wrapped.size(); i += static_cast<std::size_t>(kBodyLinesPerPage)) {
            const std::size_t end = std::min(wrapped.size(), i + static_cast<std::size_t>(kBodyLinesPerPage));
            pages.emplace_back(wrapped.begin() + static_cast<std::ptrdiff_t>(i),
                                wrapped.begin() + static_cast<std::ptrdiff_t>(end));
        }
        if (pages.empty()) pages.emplace_back();  // empty body text -- one (empty) page
        return pages;
    };
    // Every dialogueSession.bodyText assignment goes through this instead
    // of setting the field directly, so a fresh response always starts
    // back at page 0 -- easy to miss one of the several call sites
    // otherwise (Greeting/TopicText/the Quest* text frames/AskResponse's
    // own queue advance all set bodyText independently).
    auto setDialogueBodyText = [&](const std::string& text) {
        dialogueSession.bodyText = text;
        dialogueSession.bodyTextPage = 0;
    };

    // Mirrors GameLoop::talkTo's greeting-resolution precedence exactly
    // (GameLoop.cpp:1002-1046), including the askLimitLocked override.
    // Quest's real handling (checkQuestReadiness + questBegin), boat, and
    // recruit all happen later, in dialogueContinue/
    // dialogueOfferRecruitOrTopics -- not here, since talkTo itself doesn't
    // resolve any of the three until after the greeting is shown.
    auto dialogueStartTalk = [&](const DialogueCandidate& candidate) {
        dialogueSession.current = candidate;
        dialogueSession.displaySpeaker = candidate.name;
        if (auto portrait = loadDialoguePortraitTexture(candidate.id)) {
            dialoguePortraitTexture = std::move(*portrait);
            dialoguePortraitLoaded = true;
        } else {
            dialoguePortraitLoaded = false;
        }
        const std::string afterId = candidate.id + ":after";
        const bool showAfter = !candidate.dialogueAfter.empty() && state.metCharacters.count(afterId) == 0;
        const bool alreadyMet = state.metCharacters.count(candidate.id) > 0;
        const bool askLimitLocked =
            askLimitExhaustedFor(candidate.speech) && !candidate.speech.askLimitLockedText.empty();
        std::string text;
        if (showAfter) {
            text = candidate.dialogueAfter;
            state.metCharacters.insert(afterId);
        } else if (!candidate.dialogueBefore.empty()) {
            text = candidate.dialogueBefore;
        } else if (askLimitLocked) {
            text = candidate.speech.askLimitLockedText;
            dialogueSession.displaySpeaker = candidate.speech.askLimitLockedSpeaker;
        } else if (alreadyMet) {
            text = !candidate.speech.again.empty()
                       ? candidate.speech.again
                       : (candidate.name + " catches your eye and gives a small nod of recognition.");
        } else {
            text = candidate.speech.greeting;
            for (const auto& [condition, conditionalText] : candidate.speech.conditional) {
                if (conditionMatches(condition, state.character)) {
                    text = conditionalText;
                    break;
                }
            }
        }
        setDialogueBodyText(text);
        state.metCharacters.insert(candidate.id);
        checkQuestReadiness(); // a TALK objective may have just been satisfied

        if (!candidate.grantsItemId.empty() &&
            character::findQuestItemIndex(state.character, candidate.grantsItemId) < 0) {
            state.character.inventory.push_back(character::InventoryItem{
                character::ItemKind::QuestItem, character::ArmorId::None, "", 0, 0, 0, candidate.grantsItemId,
                candidate.grantsItemName});
            pushLog("You've picked up " + candidate.grantsItemName + ".");
        }

        rebuildTopicLabels(candidate);
        dialogueSession.uiState = DialogueUiState::Greeting;
    };

    // 'T' -- mirrors GameLoop::handleTalk/pickAndTalk's own 0/1/2+ shape.
    auto dialogueBegin = [&]() {
        dialogueSession.candidates = gatherTalkCandidates();
        if (dialogueSession.candidates.empty()) {
            pushLog("There's no one here to talk to.");
            return;
        }
        dialogueSession.active = true;
        if (dialogueSession.candidates.size() == 1) {
            dialogueStartTalk(dialogueSession.candidates.front());
            return;
        }
        dialogueSession.candidateSelected = 0;
        dialogueSession.uiState = DialogueUiState::PickingCandidate;
    };

    // 'P' -- mirrors GameLoop::handleShop's opening checks
    // (GameLoop.cpp:1570-1596) verbatim, including its pushLog messages and
    // its SHOP_LOCKED gate (a shop can require a quest to be Complete
    // before it opens at all, e.g. Flint's Smithy waiting on
    // ore_for_the_forge -- checked here rather than baked into isShop
    // itself so the POI's own TALK/TOPIC content stays fully reachable
    // regardless of lock state).
    auto shopBegin = [&]() {
        if (state.mode != game::Mode::Zone || currentZone == nullptr) {
            pushLog("There's nothing to buy here.");
            return;
        }
        const world::PointOfInterest* poi = currentZone->poiAt(state.zoneX, state.zoneY);
        if (poi == nullptr || !poi->isShop) {
            pushLog("There's nothing to buy here.");
            return;
        }
        if (const std::string* requiredQuestId = currentZone->shopLockAt(state.zoneX, state.zoneY)) {
            auto it = state.quests.find(*requiredQuestId);
            if (it == state.quests.end() || it->second != game::QuestStatus::Complete) {
                pushLog("There's nothing to buy here yet.");
                return;
            }
        }
        shopSession.active = true;
        shopSession.shopName = poi->name;
        shopSession.catalog = shopCatalogFor(poi->shopCatalog);
        shopSession.sellMode = false;
        shopSession.selected = 0;
        shopSession.message.clear();
    };

    // 'I' -- unlike shopBegin above, no opening gate: Inventory works from
    // Overworld or Zone, matching sheetOpen's convention rather than
    // Shop's POI-gated one. Mirrors GameLoop::handleInventory's own lack
    // of preconditions (GameLoop.cpp:1638).
    auto inventoryBegin = [&]() {
        inventorySession.active = true;
        inventorySession.selected = 0;
        inventorySession.message.clear();
    };

    // Rest/Bed Rest, final step -- mirrors performSpellMemorization's own
    // tail (GameLoop.cpp:569-581): actually memorize whatever loadout was
    // settled on (empty is fine -- matches a non-caster, or a caster whose
    // class has nothing implemented at any accessible level) and push the
    // combined message. The isCleric&&isMage branch performSpellMemorization
    // carries is NOT ported -- Character::charClass is a single value today,
    // so that branch is unreachable there too (see its own comment,
    // GameLoop.cpp:570-574).
    auto restFinishMemorization = [&]() {
        character::Character& c = state.character;
        character::memorizeSpells(c, restSession.dayAfterRest, c.preferredSpellIds);
        const std::string suffix = c.charClass == character::ClassId::Cleric
                                        ? " You rememorize your prayers."
                                        : " You memorize your incantations.";
        pushLog(restSession.baseMessage + suffix);
        restSession = RestSession{};
    };

    // Opens the level-by-level/slot-by-slot spell picker, or finishes
    // immediately if the queue restBeginSpellMemorization already built
    // turned out empty (nothing implemented at any accessible level for
    // this class -- chooseSpellLoadout's own roster.empty() guard,
    // GameLoop.cpp:598, has the same effect: nothing to choose, loadout
    // stays whatever it already was).
    auto restBeginPicking = [&]() {
        if (restSession.queue.empty()) {
            restFinishMemorization();
            return;
        }
        restSession.active = true;
        restSession.keepSamePrompt = false;
        restSession.queueIndex = 0;
        restSession.pickSelected = 0;
        restSession.loadout.clear();
    };

    // Enter on the "keep the same spells memorized?" prompt -- mirrors
    // GameLoop.cpp:536-566. "Yes" tops up any slots a level-up since the
    // last rest opened, silently, with the lowest-level roster spell (no
    // picker -- same as the console's own top-up branch); "No" goes to the
    // ordinary picker to choose an entirely new loadout.
    auto restConfirmKeepSame = [&]() {
        character::Character& c = state.character;
        if (restSession.keepSameSelected != 0) { // 0 = "Yes", 1 = "No -- choose new spells"
            restBeginPicking();
            return;
        }
        const int totalSlots = static_cast<int>(restSession.queue.size());
        const auto& roster = character::spellListFor(c.charClass);
        if (!roster.empty()) {
            while (static_cast<int>(c.preferredSpellIds.size()) < totalSlots) {
                c.preferredSpellIds.push_back(roster.front().id);
            }
        }
        restFinishMemorization();
    };

    // Enter on one spell pick -- mirrors chooseSpellLoadout's own inner
    // loop body (GameLoop.cpp:603-619): record the choice, advance to the
    // next queued slot, and once the whole queue is spent, commit the new
    // loadout and finish.
    auto restConfirmSpellPick = [&]() {
        const RestSpellPick& pick = restSession.queue[restSession.queueIndex];
        restSession.loadout.push_back(pick.choices[static_cast<size_t>(restSession.pickSelected)]->id);
        ++restSession.queueIndex;
        restSession.pickSelected = 0;
        if (restSession.queueIndex >= restSession.queue.size()) {
            state.character.preferredSpellIds = std::move(restSession.loadout);
            restFinishMemorization();
        }
    };

    // Rest/Bed Rest's spell-memorization half -- mirrors
    // performSpellMemorization (GameLoop.cpp:527-567). Called only after
    // restBegin has already committed time/HP/lastRestDay, matching the
    // console's own ordering exactly.
    auto restBeginSpellMemorization = [&](const std::string& baseMessage, long long dayAfterRest) {
        character::Character& c = state.character;
        restSession.baseMessage = baseMessage;
        restSession.dayAfterRest = dayAfterRest;

        if (character::maxAccessibleSpellLevel(c) == 0) {
            pushLog(baseMessage); // no caster suffix, no memorization at all -- matches console
            return;
        }

        restSession.queue.clear();
        const int maxLevel = character::maxAccessibleSpellLevel(c);
        const auto& roster = character::spellListFor(c.charClass);
        for (int lvl = 1; lvl <= maxLevel; ++lvl) {
            const int slots = character::spellSlotsPerDay(c, lvl);
            if (slots <= 0) continue;
            std::vector<const character::SpellInfo*> choices;
            for (const auto& spell : roster) {
                if (spell.level == lvl) choices.push_back(&spell);
            }
            if (choices.empty()) continue; // nothing implemented at this level yet
            for (int slot = 0; slot < slots; ++slot) {
                restSession.queue.push_back(RestSpellPick{lvl, slot + 1, slots, choices});
            }
        }

        if (c.preferredSpellIds.empty()) {
            // Never memorized anything before -- nothing to "keep the same"
            // as, so go straight to the picker (GameLoop.cpp:531-534).
            restBeginPicking();
        } else {
            restSession.active = true;
            restSession.keepSamePrompt = true;
            restSession.keepSameSelected = 0;
        }
    };

    // 'R' (anywhere) / 'Z' (standing on a bed inside a zone) -- mirrors
    // GameLoop::handleRest/handleBedRest (GameLoop.cpp:464-525). Time/HP/
    // lastRestDay are committed here, immediately, before the spell-loadout
    // half above ever runs -- see RestSession's own doc comment for why
    // that ordering matters (it's why there's no cancel once this fires).
    auto restBegin = [&](bool isBedRest) {
        character::Character& c = state.character;
        const long long currentDay = state.hoursElapsed / 24;
        if (c.lastRestDay == currentDay) {
            pushLog("You've already rested today.");
            return;
        }
        if (isBedRest) {
            const world::PointOfInterest* poi =
                state.mode == game::Mode::Zone && currentZone != nullptr
                    ? currentZone->poiAt(state.zoneX, state.zoneY)
                    : nullptr;
            if (poi == nullptr || !poi->isBed) {
                pushLog("There's no bed here.");
                return;
            }
        }

        state.hoursElapsed += 8; // an overnight rest -- may cross into a new day
        const long long dayAfterRest = state.hoursElapsed / 24;
        c.lastRestDay = dayAfterRest;

        std::string message;
        if (isBedRest) {
            const bool alreadyFull = c.currentHp >= c.maxHp;
            c.currentHp = c.maxHp;
            for (game::RecruitedCompanion& companion : state.companions) {
                companion.character.currentHp = companion.character.maxHp;
            }
            message = "You spend the night resting soundly in a real bed.";
            message += alreadyFull ? " You were already at full health." : " You wake fully healed.";
        } else {
            const int healed = std::min(1, c.maxHp - c.currentHp); // DMG p.74: 1 hp per day of rest
            c.currentHp += healed;
            for (game::RecruitedCompanion& companion : state.companions) {
                companion.character.currentHp = std::min(companion.character.maxHp, companion.character.currentHp + 1);
            }
            message = "You settle in and rest through the night.";
            message += healed > 0 ? " You recover 1 hit point." : " You were already at full health.";
        }

        restBeginSpellMemorization(message, dayAfterRest);
    };

    // Shared zone-entry sequence (Milestone 206) -- used by the overworld
    // Enter handler, a walked-into PORTAL tile, and a TOWN_MENU portal-type
    // menu row (a third caller doing the identical thing, which is what
    // justified pulling this out of two near-duplicate inline copies).
    // pushStack is true only for the PORTAL/menu-portal cases -- entering
    // straight from the Overworld has no zone to push onto zoneStack.
    // Returns false (does nothing else) if zoneId isn't a real zone --
    // callers decide what to log in that case, since the wording differs
    // (a portal with a dangling target vs. a location with no interior).
    auto enterZone = [&](const std::string& zoneId, bool pushStack) -> bool {
        const world::Zone* target = zones.getZone(zoneId);
        if (target == nullptr) return false;
        if (pushStack) {
            state.zoneStack.push_back({state.currentZoneId, state.zoneX, state.zoneY});
        }
        state.mode = game::Mode::Zone;
        state.currentZoneId = zoneId;
        state.zoneX = target->entryX();
        state.zoneY = target->entryY();
        currentZone = target;
        pushLog("You step into " + target->name() + ".");
        if (auto plate = loadZonePlateTexture(zoneId)) {
            zonePlateTexture = std::move(*plate);
            zonePlateZoneName = target->name();
            zonePlateLoaded = true;
            // Milestone 210: having art now always means menu-town (see
            // isEffectiveMenuTown above), which draws this same texture
            // persistently inside drawTownMenuOverlay instead -- so the
            // one-shot full-window flourish (zonePlateOpen, Milestone 205)
            // can never fire for a zone that has art, and never could for
            // one that doesn't. Left unconditionally false rather than
            // deleting drawZonePlateOverlay/zonePlateOpen outright -- see
            // docs/ARCHITECTURE.md's SFML section for why.
            zonePlateOpen = false;
        } else {
            zonePlateLoaded = false;
            zonePlateOpen = false;
        }
        return true;
    };

    // Shared zone-exit sequence (Milestone 206) -- the walkable-zone
    // "stepped onto ENTRY and pressed Enter" logic, factored out so a
    // TOWN_MENU zone's synthetic "Leave town" menu row can call it directly
    // without requiring the player to actually stand on the ENTRY tile.
    auto leaveCurrentZone = [&]() {
        if (!state.zoneStack.empty()) {
            const game::ZoneReturnPoint back = state.zoneStack.back();
            state.zoneStack.pop_back();
            state.currentZoneId = back.zoneId;
            state.zoneX = back.x;
            state.zoneY = back.y;
            currentZone = zones.getZone(state.currentZoneId);
            pushLog("You step back out into " + (currentZone ? currentZone->name() : back.zoneId) + ".");
        } else {
            pushLog("You step back outside.");
            state.mode = game::Mode::Overworld;
            currentZone = nullptr;
        }
        // Refresh the plate texture for whichever zone is now current --
        // needed so a TOWN_MENU zone's persistent menu image
        // (drawTownMenuOverlay) stays correct after a round trip through a
        // child zone (e.g. Solace -> its Inn -> back to Solace), since
        // zonePlateTexture/zonePlateLoaded otherwise still reflect
        // whichever zone was most recently *entered* forward. Never sets
        // zonePlateOpen -- backing out is not a fresh arrival, so the
        // one-shot full-window flourish never re-fires here.
        if (currentZone != nullptr) {
            if (auto plate = loadZonePlateTexture(state.currentZoneId)) {
                zonePlateTexture = std::move(*plate);
                zonePlateZoneName = currentZone->name();
                zonePlateLoaded = true;
            } else {
                zonePlateLoaded = false;
            }
        }
    };

    // The town menu's synthetic Leave row's label -- reported wrong live
    // (a user playtest, 2026-09-17): a zone reached via a *parent's own*
    // menu (Solace's Inn, entered via Solace's own "[I]" row) said "Leave
    // Town" too, which is misleading -- leaveCurrentZone above doesn't
    // exit to the overworld there, it just pops back to the parent zone's
    // menu. "Leave town" is only accurate when zoneStack is empty (a
    // top-level menu town, e.g. Solace, Palanthas -- leaving really does
    // go to the overworld); otherwise the label names the actual parent
    // zone leaveCurrentZone is about to return to.
    auto townMenuLeaveLabel = [&]() -> std::string {
        if (state.zoneStack.empty()) return "Leave town";
        const world::Zone* parent = zones.getZone(state.zoneStack.back().zoneId);
        return "Back to " + (parent != nullptr ? parent->name() : std::string("town"));
    };

    // Enter on the "talk to whom?" picker.
    auto dialogueConfirmCandidate = [&]() {
        if (dialogueSession.candidateSelected < 0 ||
            dialogueSession.candidateSelected >= static_cast<int>(dialogueSession.candidates.size())) {
            return;
        }
        dialogueStartTalk(dialogueSession.candidates[static_cast<size_t>(dialogueSession.candidateSelected)]);
    };

    // "Ask about something else..." selected in TopicPicker -- mirrors
    // GameLoop.cpp:1187-1214's hint-building: skip SUBJECT_ENDS entries
    // (meant to be discovered, not nudged toward) and skip the whole list
    // when suppressAskHints is set (ASK_ANYTHING POIs, see
    // docs/ZONE_NOTES.md's "Ask about anything").
    auto dialogueBeginAsk = [&]() {
        dialogueSession.askInputHints.clear();
        if (!dialogueSession.current.speech.suppressAskHints) {
            for (const auto& subject : dialogueSession.current.speech.subjects) {
                if (subject.endsConversation) continue;
                std::string hint = subject.keywords.front().front();
                if (!hint.empty()) hint[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(hint[0])));
                dialogueSession.askInputHints.push_back(hint);
            }
        }
        dialogueSession.askInputBuffer.clear();
        dialogueSession.uiState = DialogueUiState::AskInput;
    };

    // Enter on the "ask <name> about..." picker -- "Nothing, thanks" is
    // always the last label; askAnythingIndex (see rebuildTopicLabels) is
    // only valid when >= 0.
    auto dialogueConfirmTopic = [&]() {
        const int nothingThanksIndex = static_cast<int>(dialogueSession.topicLabels.size()) - 1;
        if (dialogueSession.topicSelected == nothingThanksIndex) {
            dialogueEnd();
            return;
        }
        if (dialogueSession.topicSelected == dialogueSession.askAnythingIndex) {
            dialogueBeginAsk();
            return;
        }
        setDialogueBodyText(
            dialogueSession.current.speech.topics[static_cast<size_t>(dialogueSession.topicSelected)].second);
        dialogueSession.uiState = DialogueUiState::TopicText;
    };

    // Enter on the free-text ask-input box -- mirrors GameLoop.cpp:1215-
    // 1296. Empty input silently cancels back to the topic picker with no
    // bookkeeping, the same net effect Console::readLine's Esc produces in
    // the console build (both return "", and the caller does nothing with
    // an empty result) -- this build has no per-screen cancel key (Escape
    // always closes the whole window, see the event loop below), so this
    // is the only cancel path, and it's a faithful one.
    auto dialogueSubmitAsk = [&]() {
        if (dialogueSession.askInputBuffer.empty()) {
            dialogueSession.uiState = DialogueUiState::TopicPicker;
            return;
        }
        const DialogueSpeech& speech = dialogueSession.current.speech;
        const DialogueSpeech::SubjectEntry* match = matchSubject(speech.subjects, dialogueSession.askInputBuffer);
        std::string response;
        bool endsConversation = false;
        if (match != nullptr) {
            response = match->text;
            endsConversation = match->endsConversation;
        } else if (!speech.subjectUnknown.empty()) {
            response = speech.subjectUnknown;
        } else {
            response =
                dialogueSession.current.name + " gives you a blank look. \"I'm not sure what you mean by that.\"";
        }
        dialogueSession.askMessages.clear();
        dialogueSession.askMessages.push_back(response);
        dialogueSession.askQueueEndsConversation = false;

        const long long today = state.hoursElapsed / 24;
        if (endsConversation) {
            // SUBJECT_ENDS exhausts today's audience entirely (same as
            // running out the ordinary question count) rather than just
            // ending this one visit.
            if (speech.askLimit > 0) {
                state.character.lastAstinusAskDay = today;
                if (state.character.astinusDailyLimit <= 0) state.character.astinusDailyLimit = speech.askLimit;
                state.character.astinusQuestionsToday = state.character.astinusDailyLimit;
            }
            dialogueSession.askQueueEndsConversation = true;
        } else if (speech.askLimit > 0) {
            if (state.character.lastAstinusAskDay != today) {
                state.character.lastAstinusAskDay = today;
                state.character.astinusQuestionsToday = 0;
                state.character.astinusDailyLimit = speech.askLimit;
            } else if (state.character.astinusDailyLimit <= 0) {
                state.character.astinusDailyLimit = speech.askLimit;
            }
            ++state.character.astinusQuestionsToday;
            if (state.character.astinusQuestionsToday >= state.character.astinusDailyLimit) {
                bool extended = false;
                if (state.character.astinusDailyLimit < speech.askLimitHardCap) {
                    // Homebrew Int+Wis proficiency-check combination, not a
                    // printed 2e mechanic -- see docs/ZONE_NOTES.md's "Ask
                    // about anything" and GameLoop.cpp:1268-1287.
                    const int intRoll = character::roll(1, 20);
                    const int wisRoll = character::roll(1, 20);
                    const bool passedInt = intRoll <= state.character.scores.intelligence && intRoll != 20;
                    const bool passedWis = wisRoll <= state.character.scores.wisdom && wisRoll != 20;
                    if (passedInt && passedWis) {
                        state.character.astinusDailyLimit = speech.askLimitHardCap;
                        extended = true;
                        dialogueSession.askMessages.push_back(speech.askLimitExtendedText);
                    }
                }
                if (!extended) {
                    dialogueSession.askMessages.push_back(speech.askLimitReachedText);
                    dialogueSession.askQueueEndsConversation = true;
                }
            }
        }
        dialogueSession.askMessageIndex = 0;
        setDialogueBodyText(dialogueSession.askMessages.front());
        dialogueSession.uiState = DialogueUiState::AskResponse;
    };

    // Shared tail reached both directly from Greeting (no boat to offer) and
    // from a declined boat offer -- offers RecruitOffer next if this
    // candidate carries a recruitCompanionId not already in the party,
    // otherwise falls through to the ordinary topic picker (or ends the
    // conversation if there's nothing to ask about). Mirrors talkTo's own
    // linear fallthrough from the boat block into the recruit block into
    // the topic picker (GameLoop.cpp:1112-1176) -- boat, then recruit, then
    // topics, in that order, every time.
    auto dialogueOfferRecruitOrTopics = [&]() {
        const DialogueCandidate& candidate = dialogueSession.current;
        if (!candidate.recruitCompanionId.empty() && !alreadyRecruited(candidate)) {
            dialogueSession.recruitOfferSelected = 0;
            dialogueSession.uiState = DialogueUiState::RecruitOffer;
        } else if (!candidate.speech.topics.empty() || dialogueSession.askAnythingIndex >= 0) {
            dialogueSession.uiState = DialogueUiState::TopicPicker;
        } else {
            dialogueEnd();
        }
    };

    // "Not yet"/Q/Escape on the RecruitOffer picker -- falls through to the
    // ordinary topic picker (or ends the conversation), same shape as
    // dialogueOfferRecruitOrTopics's own topics-or-end tail. Mirrors talkTo's
    // own fallthrough (GameLoop.cpp:1151-1153: "falls through to the
    // ordinary topics/SUBJECT picker below, same as BOAT's decline path
    // above").
    auto dialogueDeclineRecruit = [&]() {
        if (!dialogueSession.current.speech.topics.empty() || dialogueSession.askAnythingIndex >= 0) {
            dialogueSession.uiState = DialogueUiState::TopicPicker;
        } else {
            dialogueEnd();
        }
    };

    // "Join me" on the RecruitOffer picker -- direct port of talkTo's own
    // recruit block (GameLoop.cpp:1144-1150). Unlike boarding a boat, this
    // does NOT end the conversation -- talkTo falls through to the topic
    // picker whether the offer was accepted or declined, so this shares
    // dialogueDeclineRecruit's own fallthrough rather than duplicating it.
    auto dialogueJoinRecruit = [&]() {
        const DialogueCandidate& candidate = dialogueSession.current;
        game::RecruitedCompanion recruited;
        recruited.id = candidate.recruitCompanionId;
        recruited.character = character::buildCompanionById(candidate.recruitCompanionId);
        state.companions.push_back(std::move(recruited));
        pushLog(candidate.name + " joins your party.");
        dialogueDeclineRecruit();
    };

    // Shared tail reached once quest and boat have both had their chance to
    // intercept a Greeting dismissal (see questBegin/dialogueContinue
    // below) -- offers BoatOffer if this candidate carries a
    // boatDestinationId, else falls to dialogueOfferRecruitOrTopics.
    // Extracted so every quest-resolution endpoint below (declining,
    // dismissing progress/accept text, finishing a turn-in with no Wayreth
    // reward, resolving the Wayreth choice) can resume the exact same
    // chain a plain Greeting dismissal would have gone to -- mirrors how
    // GameLoop::talkTo always falls through to its own boat block right
    // after offerOrTurnInQuest returns, no matter which path inside it was
    // taken (GameLoop.cpp:1047-1061).
    auto dialogueAfterGreeting = [&]() {
        if (!dialogueSession.current.boatDestinationId.empty()) {
            // Offered on every talk (including after boarding once -- see
            // boatDestinationId's own comment), ahead of the topic menu --
            // mirrors talkTo's own precedence exactly (GameLoop.cpp:
            // 1061-1115), independent of which greeting text was actually
            // shown and of whether a quest screen ran first.
            dialogueSession.boatOfferSelected = 0;
            dialogueSession.uiState = DialogueUiState::BoatOffer;
        } else {
            dialogueOfferRecruitOrTopics();
        }
    };

    // Mirrors GameLoop::offerOrTurnInQuest's own not-yet-started branch
    // (GameLoop.cpp:1308-1319) -- called from dialogueContinue right after
    // a Greeting is dismissed, the same point talkTo calls
    // offerOrTurnInQuest at (checkQuestReadiness already ran inside
    // dialogueStartTalk, matching GameLoop.cpp:1039 vs :1048's ordering).
    // Active/ReadyToTurnIn are resolved here too, one dialogue frame each;
    // Complete is silently skipped, same as offerOrTurnInQuest's own early
    // return. Returns true if a quest screen now owns uiState (caller must
    // stop, not fall through to dialogueAfterGreeting yet); false means
    // there's nothing to show (no quest here, an unmet REQUIRE, or already
    // Complete) and the caller should call dialogueAfterGreeting itself.
    auto questBegin = [&]() -> bool {
        const DialogueCandidate& candidate = dialogueSession.current;
        if (candidate.questId.empty()) return false;
        const quest::Quest* q = quests.find(candidate.questId);
        if (q == nullptr) return false; // defensive -- startup already cross-validated every zone QUEST id

        auto it = state.quests.find(candidate.questId);
        if (it == state.quests.end()) {
            if (!q->requirement.empty() && !conditionMatches(q->requirement, state.character)) return false;
            setDialogueBodyText(q->offerText);
            dialogueSession.uiState = DialogueUiState::QuestOfferText;
            return true;
        }
        if (it->second == game::QuestStatus::Complete) return false;
        if (it->second == game::QuestStatus::Active) {
            setDialogueBodyText(q->progressText);
            dialogueSession.uiState = DialogueUiState::QuestProgressText;
            return true;
        }
        // ReadyToTurnIn.
        setDialogueBodyText(q->completeText);
        dialogueSession.uiState = DialogueUiState::QuestCompleteText;
        return true;
    };

    // Enter/dismiss on the QuestOfferText frame -- opens the Accept/Decline
    // picker, mirroring offerOrTurnInQuest's own dialogue-then-picker shape
    // (GameLoop.cpp:1321-1327).
    auto questShowAcceptDecline = [&]() {
        dialogueSession.questOfferSelected = 0;
        dialogueSession.uiState = DialogueUiState::QuestAcceptDecline;
    };

    // "Accept" on the QuestAcceptDecline picker -- mirrors
    // offerOrTurnInQuest's own accept branch (GameLoop.cpp:1332-1342).
    // "Decline"/Quit on that same picker records nothing and just calls
    // dialogueAfterGreeting directly (see the key-dispatch wiring below),
    // matching offerOrTurnInQuest's bare `return;` on decline.
    auto questAccept = [&]() {
        const quest::Quest* q = quests.find(dialogueSession.current.questId);
        if (q == nullptr) return; // defensive, same as questBegin
        state.quests[dialogueSession.current.questId] = game::QuestStatus::Active;
        setDialogueBodyText(q->acceptText);
        dialogueSession.uiState = DialogueUiState::QuestAcceptText;
        pushLog("Quest accepted: " + q->name + ".");
        // Catches "already did it before being asked" -- without this, a
        // quest accepted in a state that already satisfies every objective
        // would sit at Active forever (GameLoop.cpp:1337-1342).
        checkQuestReadiness();
    };

    // Enter on the QuestCompleteText frame -- mirrors offerOrTurnInQuest's
    // own ReadyToTurnIn/turn-in branch (GameLoop.cpp:1363-1528): hands over
    // delivered items, awards steel/XP (with a mid-conversation level-up
    // check), marks Complete, logs the reward summary, then applies every
    // simple reward flag. rewardWayrethRobe is the one flag that can't
    // finish here -- it stages its own WayrethIntro/WayrethChoice scene
    // instead of calling dialogueAfterGreeting itself.
    auto questFinishTurnIn = [&]() {
        const DialogueCandidate& candidate = dialogueSession.current;
        const quest::Quest* q = quests.find(candidate.questId);
        if (q == nullptr) {
            dialogueAfterGreeting(); // defensive, same as questBegin
            return;
        }
        for (const auto& objective : q->objectives) {
            if (objective.kind != quest::ObjectiveKind::Deliver) continue;
            for (int i = 0; i < objective.count; ++i) {
                int index = character::findQuestItemIndex(state.character, objective.targetId);
                if (index >= 0) state.character.inventory.erase(state.character.inventory.begin() + index);
            }
        }
        state.character.steelPieces += q->rewardSteel;
        if (q->rewardXp > 0) {
            state.character.experience += q->rewardXp;
            // First call site of applyPendingLevelUps outside combat -- a
            // level-up can now happen mid-conversation, same as
            // GameLoop.cpp:1380-1383. Its messages go to the persistent
            // log, not the dialogue frame that just closed.
            std::vector<std::string> levelUpMessages;
            character::applyPendingLevelUps(state.character, levelUpMessages);
            for (const std::string& msg : levelUpMessages) pushLog(msg);
        }
        state.quests[candidate.questId] = game::QuestStatus::Complete;
        std::ostringstream rewardMsg;
        rewardMsg << "Quest complete: " << q->name << ".";
        if (q->rewardSteel > 0) rewardMsg << " +" << q->rewardSteel << " steel.";
        if (q->rewardXp > 0) rewardMsg << " +" << q->rewardXp << " XP.";
        pushLog(rewardMsg.str());
        if (q->rewardKnightSword) {
            state.character.knightOrder = character::KnightOrder::Sword;
            pushLog("You are named a Knight of the Sword.");
        }
        if (q->rewardSolamnicArmor) {
            // See character::ArmorId::SolamnicArmor and docs/CHARACTER_NOTES.md's
            // "Magic items" -- an ordinary Shield accompanies it, a deliberate
            // simplification of the book's separate "shield +1".
            state.character.inventory.push_back(
                character::InventoryItem{character::ItemKind::Armor, character::ArmorId::SolamnicArmor, "", 0, 0});
            state.character.inventory.push_back(
                character::InventoryItem{character::ItemKind::Shield, character::ArmorId::None, "", 0, 0});
            pushLog("You are granted Solamnic Armor and a Knight's shield. Press 'i' to equip them.");
        }
        if (q->rewardKnightRose) {
            state.character.knightOrder = character::KnightOrder::Rose;
            pushLog("You are named a Knight of the Rose.");
        }
        if (q->rewardStaffOfStrikingCuring) {
            state.character.inventory.push_back(character::InventoryItem{
                character::ItemKind::Weapon, character::ArmorId::None, character::kStaffOfStrikingCuringName,
                character::kStaffDamageSides, 0, character::kStaffMagicBonus});
            pushLog("You are granted the Staff of Striking/Curing. Press 'i' to equip it.");
        }
        if (q->rewardFrostreaver) {
            // weaponMagicBonus is 0 here on purpose -- the Frostreaver's +4
            // only applies while standing on glacier terrain, a this-fight-
            // only local bonus (see combat's own Frostreaver handling), not
            // baked into the item itself.
            state.character.inventory.push_back(character::InventoryItem{
                character::ItemKind::Weapon, character::ArmorId::None, character::kFrostreaverName,
                character::kFrostreaverDamageSides, 0, 0});
            pushLog("You are granted a Frostreaver. Press 'i' to equip it.");
        }
        if (q->rewardWayrethRobe) {
            // The actual Test of High Sorcery -- see GameLoop.cpp:1429-1528
            // for the full sourcing/design rationale (Dragonlance
            // Adventures pp.34-35, Players Guide pp.79-80: the Test grades
            // conduct during the Test, not a preset alignment).
            setDialogueBodyText(
                "The grave-cold thing you put down a moment ago doesn't dissipate the way a beaten "
                "illusion should. It holds, one heartbeat too long, and reshapes -- not into the "
                "stranger, not into anything Wayreth would claim as its own, but into a face you'd "
                "trust with your back turned. It doesn't attack. It doesn't need to. It just stands "
                "between you and the rest of your life, waiting to see what you do about that.");
            dialogueSession.uiState = DialogueUiState::WayrethIntro;
            return;
        }
        dialogueAfterGreeting();
    };

    // Enter on the WayrethIntro frame -- opens the 3-option ethical-choice
    // picker, mirroring offerOrTurnInQuest's own dialogue-then-picker shape
    // (GameLoop.cpp:1459-1469).
    auto questShowWayrethChoice = [&]() {
        dialogueSession.wayrethChoiceSelected = 0;
        dialogueSession.uiState = DialogueUiState::WayrethChoice;
    };

    // Enter on the WayrethChoice picker -- mirrors offerOrTurnInQuest's own
    // Test-of-High-Sorcery resolution (GameLoop.cpp:1485-1527): applies
    // withEthic, assigns robeColor by the resulting alignment, and logs one
    // of three outcome passages. Always ends the quest detour via
    // dialogueAfterGreeting -- there is no further quest state after this.
    // ethicLabels' authored order (see drawDialogueOverlay's WayrethChoice
    // case) matches EthicChoice's own declaration order (Good, Neutral,
    // Evil), same as GameLoop.cpp's own picker.
    auto questResolveWayreth = [&]() {
        const EthicChoice choice = static_cast<EthicChoice>(dialogueSession.wayrethChoiceSelected);
        character::Alignment newAlignment = withEthic(state.character.alignment, choice);
        if (newAlignment != state.character.alignment) {
            state.character.alignment = newAlignment;
            pushLog(
                "Your alignment shifts: the Test measured what you did, not what you meant to be. You "
                "are now considered " +
                std::string(character::alignmentName(newAlignment)) + ".");
        }
        state.character.robeColor = character::robeForAlignment(state.character.alignment);
        switch (state.character.robeColor) {
            case character::RobeColor::White:
                pushLog(
                    "You could spend this -- the shape, the moment, whatever's left of the working "
                    "underneath it -- for real power, and some clean part of you wants to. You don't. "
                    "The illusion falls apart at your feet, the way it should have from the start, and "
                    "every reflex you fought down to refuse that trade turns out to matter more than "
                    "the spells you cast to get here. You emerge a " +
                    std::string(character::robeColorName(state.character.robeColor)) + ", sworn to " +
                    character::robeMoonName(state.character.robeColor) +
                    ", having learned exactly what the good in you is worth when no one but the "
                    "Conclave is watching. The Conclave sees you home.");
                break;
            case character::RobeColor::Red:
                pushLog(
                    "Every trial the Conclave set you tonight resolved into the same shape underneath "
                    "-- a mercy that would cost you the working, a cruelty that would buy it outright "
                    "-- and this one is no different. You take neither. You emerge a " +
                    std::string(character::robeColorName(state.character.robeColor)) + ", sworn to " +
                    character::robeMoonName(state.character.robeColor) +
                    ", already fluent in a kind of balance most people spend a lifetime failing to "
                    "learn. The Conclave sees you home.");
                break;
            case character::RobeColor::Black:
                pushLog(
                    "The illusion puts someone you'd call a friend between you and the only way "
                    "through, and you don't hesitate nearly as long as you expected to. You emerge a " +
                    std::string(character::robeColorName(state.character.robeColor)) + ", sworn to " +
                    character::robeMoonName(state.character.robeColor) +
                    ", carrying home a certainty about yourself you didn't have when you left. The "
                    "Conclave sees you home.");
                break;
            case character::RobeColor::None:
                break; // unreachable -- robeForAlignment never returns None
        }
        dialogueAfterGreeting();
    };

    // Enter (or any key, per the console's "press any key to continue") on
    // a plain dialogue box. Greeting checks quest first (questBegin), then
    // goes to BoatOffer if there's a boat to offer, else to RecruitOffer/
    // the topic menu/ends the conversation, via dialogueAfterGreeting;
    // TopicText always returns to the topic menu (mirroring talkTo's own
    // topic loop); AskResponse advances to the next queued message if
    // there is one, else ends the conversation or returns to a freshly-
    // rebuilt topic menu, per dialogueSubmitAsk's bookkeeping. The Quest*/
    // Wayreth* text frames all continue the same quest detour they're
    // already in (see each's own dedicated handler above). Checks for a
    // remaining page of the *current* bodyText first -- only once that's
    // exhausted does it actually advance the conversation state.
    auto dialogueContinue = [&]() {
        const std::size_t pageCount = paginateBodyText(dialogueSession.bodyText).size();
        if (static_cast<std::size_t>(dialogueSession.bodyTextPage) + 1 < pageCount) {
            ++dialogueSession.bodyTextPage;
            return;
        }
        if (dialogueSession.uiState == DialogueUiState::Greeting) {
            if (!questBegin()) dialogueAfterGreeting();
        } else if (dialogueSession.uiState == DialogueUiState::QuestOfferText) {
            questShowAcceptDecline();
        } else if (dialogueSession.uiState == DialogueUiState::QuestAcceptText ||
                   dialogueSession.uiState == DialogueUiState::QuestProgressText) {
            dialogueAfterGreeting();
        } else if (dialogueSession.uiState == DialogueUiState::QuestCompleteText) {
            questFinishTurnIn();
        } else if (dialogueSession.uiState == DialogueUiState::WayrethIntro) {
            questShowWayrethChoice();
        } else if (dialogueSession.uiState == DialogueUiState::TopicText) {
            dialogueSession.uiState = DialogueUiState::TopicPicker;
        } else if (dialogueSession.uiState == DialogueUiState::AskResponse) {
            ++dialogueSession.askMessageIndex;
            if (dialogueSession.askMessageIndex < static_cast<int>(dialogueSession.askMessages.size())) {
                setDialogueBodyText(dialogueSession.askMessages[static_cast<size_t>(dialogueSession.askMessageIndex)]);
            } else if (dialogueSession.askQueueEndsConversation) {
                dialogueEnd();
            } else {
                rebuildTopicLabels(dialogueSession.current);
                dialogueSession.uiState = DialogueUiState::TopicPicker;
            }
        }
    };

    // "Not yet"/Q/Escape on the BoatOffer picker -- falls through to
    // dialogueOfferRecruitOrTopics, exactly what a plain Greeting dismissal
    // would have done had there been no boat to offer (recruit next, then
    // topics). Mirrors talkTo's own fallthrough (GameLoop.cpp:1112-1114:
    // "ends up here, falling through to the ordinary topics/SUBJECT picker
    // below, same as any other POI's declined offer" -- which itself falls
    // through the recruit block first).
    auto dialogueDeclineBoat = [&]() { dialogueOfferRecruitOrTopics(); };

    // "Board" on the BoatOffer picker -- mirrors talkTo's own boat block
    // (GameLoop.cpp:1080-1110) exactly: whisks the player straight to the
    // destination and ends the conversation outright (talkTo itself
    // `return`s from here, never reaching the topic picker below it).
    auto dialogueBoardBoat = [&]() {
        const DialogueCandidate& candidate = dialogueSession.current;
        const world::Location* destination = world.getLocation(candidate.boatDestinationId);
        if (destination == nullptr) {
            // Defensive -- ZoneCatalog::loadForWorld already validated this
            // id, same guard GameLoop.cpp:1063 carries for the same reason.
            dialogueDeclineBoat();
            return;
        }
        state.voyagesTaken.insert(candidate.id);
        // Computed before x/y are overwritten below -- see compassDirection's
        // own note on dy sign.
        const char* dir = compassDirection(destination->x - state.x, destination->y - state.y);
        state.mode = game::Mode::Overworld;
        state.currentZoneId.clear();
        state.zoneStack.clear();
        currentZone = nullptr;
        state.x = destination->x;
        state.y = destination->y;
        state.hoursElapsed += candidate.boatHours;
        state.visitedLocations.insert(destination->id);
        checkQuestReadiness(); // a VISIT objective may have just been satisfied
        // Two phrasings, split at a day -- matches GameLoop.cpp:1100-1107's
        // own split between the four original 48-96 hour open-water legs and
        // the Crossing/Port O'Call strait hop's much shorter one.
        if (candidate.boatHours < 24) {
            pushLog("You board the ferry, and it carries you " + std::string(dir) +
                    " across the water. Before long, " + destination->name + " comes into view.");
        } else {
            pushLog("You board the ship, and it carries you " + std::string(dir) +
                    " across open water. Days pass before " + destination->name + " finally rises out of the fog.");
        }
        dialogueSession.active = false;
    };

    // --- Combat (Phase 3): all state is CombatSession above; every lambda
    // below is a non-blocking port of the matching piece of
    // GameLoop::runCombat (see that function, src/game/GameLoop.cpp:1762-
    // 3050, and this file's top-of-file comment for the scope this phase
    // ports vs. defers). Declared in dependency order -- a lambda body can
    // only see names already declared earlier in the source, same
    // constraint GameLoop::runCombat's own lambdas are bound by (see
    // docs/ARCHITECTURE.md).
    CombatSession combatSession;

    // The player's cosmetic facing target for combat sprite art (see
    // drawCombatSpriteToken above): the nearest living monster instance's
    // nearest footprint cell (same combat::nearestFootprintCell/
    // chebyshevDistance helpers the sidebar's own "dist N" readout already
    // uses further below). std::nullopt only if every instance is down --
    // combat would already be over by then, but this is defensive rather
    // than assumed.
    auto combatNearestLivingEnemyPos = [&]() -> std::optional<combat::GridPos> {
        std::optional<combat::GridPos> nearest;
        int nearestDist = 0;
        for (size_t i = 0; i < combatSession.instances.size(); ++i) {
            if (combatSession.instances[i].hp <= 0) continue;
            const combat::GridPos cell =
                combat::nearestFootprintCell(combatSession.playerPos, combatSession.instancePositions[i],
                                              combatSession.monster.footprintWidth, combatSession.monster.footprintHeight);
            const int dist = combat::chebyshevDistance(combatSession.playerPos, cell);
            if (!nearest.has_value() || dist < nearestDist) {
                nearestDist = dist;
                nearest = cell;
            }
        }
        return nearest;
    };

    // Character sheet ('C'): a transient full-window UI overlay, not world
    // state -- same reasoning CombatSession's own comment above gives for
    // staying local rather than becoming a third game::Mode value. Dismissed
    // by any key, matching the console's blocking-readKey() convention for
    // this and every other console overlay (Sheet/Spellbook/Help/WorldMap).
    bool sheetOpen = false;

    // Spellbook ('s', only while the sheet is open and the character is a
    // caster): the sheet's own full-spellbook drill-down
    // (GameLoop::showSpellbook, GameLoop.cpp:644-647), same transient-UI
    // reasoning as sheetOpen above. Any key other than North/South returns
    // to the sheet (which stays open), matching the console's
    // showCharacterSheet loop -- except the console prints to a terminal
    // that can show/scroll arbitrarily many lines, while this window is a
    // fixed size for the run (whatever it launched maximized at), so a
    // high-level caster's full list (a 20th-level Mage
    // has 9 spell levels' worth) needs the same real scroll treatment
    // logSession already has, unlike the console original. scrollOffset
    // resets to 0 each time the spellbook is (re)opened.
    bool spellbookOpen = false;
    int spellbookScrollOffset = 0;

    // Help ('/', the pixel-space bind for the console's '?'), World Map
    // ('o'), and Journal ('g') -- same transient-full-window-overlay
    // reasoning as sheetOpen above, dismissed by any key. Journal ports
    // GameLoop::showJournal's own logic (see drawJournalOverlay).
    bool helpOpen = false;
    bool worldMapOpen = false;
    bool journalOpen = false;

    // Quit confirmation -- the outermost wantsQuit catch-all in the key
    // dispatch below used to call window.close() directly; it now opens
    // this instead, so an accidental Q/Escape (an ordinary WASD-adjacent
    // key) can't end the session outright. Reachable from every state
    // that doesn't already give Quit its own meaning (Overworld, a Zone,
    // and -- since combat had no wantsQuit override of its own -- Combat
    // too, which previously fell straight into the same catch-all).
    // selectedIndex always resets to 1 ("No") whenever this opens, so a
    // stray double-Enter can never quit by accident.
    bool quitConfirmOpen = false;
    int quitConfirmSelected = 1;

    auto combatCompanionAlive = [&](size_t i) { return state.companions[i].character.currentHp > 0; };

    // Shared by combatBeginPlayerMove below, called both before and after
    // monsters get a chance to act -- a monster that wasn't yet adjacent
    // when the move was declared can close in during its own turn and land
    // exactly on the cell the player is mid-step into, since nothing else
    // re-checks that cell once combatRollGoFirstAndMaybeActMonsters runs.
    // Left uncaught, that produces two combatants sharing one GridPos,
    // which combat::isAdjacent's own "never adjacent to itself" rule then
    // masks as both being permanently "too far away" to attack each other.
    // Moved up here (was originally right before its first use, much
    // further down) so drawCombatSpriteToken's -wide/-tall/-four
    // neighbor-occupancy check below can also call it -- doesn't reference
    // anything declared between the two spots.
    auto combatCellOccupied = [&](combat::GridPos cell) {
        for (size_t i = 0; i < combatSession.instances.size(); ++i) {
            if (combatSession.instances[i].hp <= 0) continue;
            // Milestone 190: `cell` must miss every cell of a footprint>1x1
            // instance, not just its anchor -- degenerates to the exact
            // bare equality check above for every ordinary 1x1 monster.
            for (const combat::GridPos& occupied : combat::footprintCells(
                     combatSession.instancePositions[i], combatSession.monster.footprintWidth,
                     combatSession.monster.footprintHeight)) {
                if (occupied.x == cell.x && occupied.y == cell.y) return true;
            }
        }
        for (size_t ci = 0; ci < state.companions.size(); ++ci) {
            if (combatCompanionAlive(ci) && combatSession.companionPositions[ci].x == cell.x &&
                combatSession.companionPositions[ci].y == cell.y) {
                return true;
            }
        }
        return false;
    };

    // Milestone 204: which monster instance (if any) occupies `cell`, for
    // the Aim cursor -- same whole-footprint check as combatCellOccupied's
    // monster loop just above (not a copy-paste divergence, just a
    // different return type: an index instead of a bool, since Aim needs
    // to know WHICH instance, not just whether one is there). Returns -1
    // for empty ground, a wall, the player, or a companion -- only a
    // living monster instance is ever a legal Aim target.
    auto combatInstanceAtCell = [&](combat::GridPos cell) -> int {
        for (size_t i = 0; i < combatSession.instances.size(); ++i) {
            if (combatSession.instances[i].hp <= 0) continue;
            for (const combat::GridPos& occupied : combat::footprintCells(
                     combatSession.instancePositions[i], combatSession.monster.footprintWidth,
                     combatSession.monster.footprintHeight)) {
                if (occupied.x == cell.x && occupied.y == cell.y) return static_cast<int>(i);
            }
        }
        return -1;
    };

    auto combatNearestRefuge = [&]() -> const world::Location* {
        const world::Location* best = nullptr;
        long long bestDistSq = 0;
        for (const world::Location& loc : world.allLocations()) {
            if (!loc.isTown && !loc.seaLocked) continue;
            long long ddx = loc.x - state.x;
            long long ddy = loc.y - state.y;
            long long distSq = ddx * ddx + ddy * ddy;
            if (best == nullptr || distSq < bestDistSq) {
                best = &loc;
                bestDistSq = distSq;
            }
        }
        return best != nullptr ? best : world.getLocation("solace");
    };

    auto combatMonsterLabel = [&](int idx) -> std::string {
        if (!combatSession.useLetters) return combatSession.monster.name;
        return combatSession.monster.name + " " + std::string(1, static_cast<char>('A' + idx));
    };

    auto combatAliveCount = [&]() {
        int count = 0;
        for (const CombatInstance& inst : combatSession.instances) {
            if (inst.hp > 0) ++count;
        }
        return count;
    };

    // Turn-follow camera/card (Milestone 200): every log line goes through
    // this instead of a bare push_back, so it's automatically tagged with
    // whoever's turn currentTurnActor says is producing messages right now
    // -- see CombatSession::logActorForLine/currentTurnActor's own doc
    // comment for the full mechanism. Declared this early since even
    // combatKnockedOutBy just below (a knockout can happen well before any
    // of the round/action lambdas further down this file) already needs it.
    auto combatPushLog = [&](const std::string& text) {
        combatSession.log.push_back(text);
        combatSession.logActorForLine.push_back(combatSession.currentTurnActor);
    };

    // Knockout ending -- not a real death (this project never permadeaths
    // the player, see docs/COMBAT_NOTES.md): full-heals the player and
    // every companion, then carries them to the nearest town/sea-locked
    // refuge, mirroring GameLoop::nearestRefuge/knockedOutBy exactly.
    auto combatKnockedOutBy = [&](const std::string& cause) {
        const world::Location* refuge = combatNearestRefuge();
        const std::string refugeName = refuge != nullptr ? refuge->name : "town";
        state.character.currentHp = state.character.maxHp;
        for (game::RecruitedCompanion& companion : state.companions) {
            companion.character.currentHp = companion.character.maxHp;
        }
        combatPushLog("You are struck down... and wake up back in " + refugeName +
                                     ", battered but alive.");
        if (refuge != nullptr) {
            state.x = refuge->x;
            state.y = refuge->y;
        }
        pushLog("You were knocked out by the " + cause + " and woke up back in " + refugeName + ".");
        combatSession.uiState = CombatUiState::Lost;
        combatSession.roundJustConcluded = true;
    };

    // Checks the player's HP after any step that could have dropped it
    // (an opportunity attack, a death-burst, a monster's own turn) and
    // applies the knockout ending exactly once if so -- the non-blocking
    // equivalent of GameLoop::runCombat's `fightAlreadyEnded` flag, which
    // exists there so a blocking call stack can unwind early; here it's
    // just "is uiState already Lost."
    auto combatCheckPlayerDown = [&](const std::string& cause) -> bool {
        if (state.character.currentHp > 0) return false;
        if (combatSession.uiState != CombatUiState::Lost) combatKnockedOutBy(cause);
        return true;
    };

    // Playtest feedback (2026-09-11, on Milestone 185's bigger battlefield):
    // a monster/companion's whole multi-step move used to resolve entirely
    // before the next real frame, so the token visibly teleported straight
    // to its final cell instead of appearing to walk there. Called once per
    // single step from combatMonstersAct/combatCompanionActs' own movement
    // loops below, this redraws just the combat map/tokens and pauses
    // briefly, so a multi-step move reads as walking one square at a time.
    //
    // Deliberately does NOT call window.clear() first, and deliberately
    // does NOT also redraw the sidebar (a separate viewport, only drawn by
    // the real per-frame draw at the bottom of the main loop): the tile
    // loop below already paints a fully opaque tile over all
    // kCombatGridWidth*kCombatGridHeight cells every single call, so the
    // map region is completely repainted regardless of whatever was left
    // in the back buffer, and the sidebar simply isn't touched -- cheaper
    // than duplicating its own draw logic here too, for an animation aid
    // that's only ever about the tokens' positions. A short, deliberate
    // amount of draw-code duplication against the real per-frame combat
    // draw further down (rather than a bigger refactor to share it) --
    // that draw's own PickingTarget-aware camera/highlight logic isn't
    // needed here, since no picker is ever open while AI is acting.
    // kAiStepAnimationMs is an invented, flagged pacing number (DQoK's own
    // manual specifies no animation speed) -- tuned up from an initial 70ms
    // per the user's own live playtest feedback ("slow them down a tick"),
    // still bounded enough that a full-budget Wraith/Spectre (MOVE 24/30)
    // doesn't stall the window for long.
    constexpr int kAiStepAnimationMs = 130;
    // Same invented-pacing precedent as kAiStepAnimationMs above, just for
    // the player's attack-pose flash (see combatFlashPlayerAttackPose
    // below) -- longer than a move step since it's meant to read as a
    // deliberate swing, not a walk cycle. Expect this needs the same kind
    // of live-feedback tuning pass once seen at a real keyboard.
    constexpr int kAttackPoseFlashMs = 200;
    // Milestone 204: how long each auto-advanced AwaitContinue message line
    // stays on screen before combatAdvancePendingMessage (near the main
    // loop) moves to the next one -- same "invented, flagged pacing
    // number, needs live tuning" idiom as kAiStepAnimationMs/
    // kAttackPoseFlashMs above. Text needs longer than a walk-step to
    // actually read, so this is well above kAiStepAnimationMs.
    constexpr int kMessageAutoAdvanceMs = 600;
    // Centers the camera on whichever cell is actually moving this step
    // (the same live-feedback pass: the camera used to stay fixed on the
    // player, so a companion or monster walking far from the player was
    // animating off-screen) rather than always the player's own position.
    // `playerAttackPose` (Combat sprite art, see docs/ARCHITECTURE.md's
    // SFML section) draws the player's attack-pose frame for this one
    // frame instead of idle, and sleeps the longer kAttackPoseFlashMs --
    // combatFlashPlayerAttackPose below is the only caller that passes
    // true, reusing this same draw-then-sleep animation frame rather than
    // duplicating it a third time.
    auto combatAnimateAiStep = [&](combat::GridPos focus, bool playerAttackPose = false) {
        const float combatPxW = static_cast<float>(kCombatGridWidth) * kCombatTilePx;
        const float combatPxH = static_cast<float>(kCombatGridHeight) * kCombatTilePx;
        const float focusPxX = (static_cast<float>(focus.x) + 0.5f) * kCombatTilePx;
        const float focusPxY = (static_cast<float>(focus.y) + 0.5f) * kCombatTilePx;
        const sf::Vector2f viewSize = combatMapView.getSize();
        const float halfW = viewSize.x / 2.f;
        const float halfH = viewSize.y / 2.f;
        const float camX = std::clamp(focusPxX, halfW, std::max(halfW, combatPxW - halfW));
        const float camY = std::clamp(focusPxY, halfH, std::max(halfH, combatPxH - halfH));
        combatMapView.setCenter(sf::Vector2f(camX, camY));
        window.setView(combatMapView);

        // Paints over exactly the currently-visible map region before
        // redrawing it, rather than window.clear() (which would also blank
        // the sidebar's own separate viewport for this one frame -- the
        // reason this function skips clear() at all, per its own doc
        // comment above). Without this, the tiles' own 1px inter-tile gaps
        // don't fully overwrite a marker/glyph pixel left there by a
        // previous animation step, showing up as small stray line
        // fragments trailing a moving token -- found live by the user
        // (2026-09-11) once several steps of the new per-square animation
        // were actually visible on screen.
        sf::RectangleShape visibleMapAreaFill(viewSize);
        visibleMapAreaFill.setOrigin(sf::Vector2f(viewSize.x / 2.f, viewSize.y / 2.f));
        visibleMapAreaFill.setPosition(sf::Vector2f(camX, camY));
        visibleMapAreaFill.setFillColor(sf::Color::Black);
        window.draw(visibleMapAreaFill);

        for (int gy = 0; gy < kCombatGridHeight; ++gy) {
            for (int gx = 0; gx < kCombatGridWidth; ++gx) {
                const float cx = (static_cast<float>(gx) + 0.5f) * kCombatTilePx;
                const float cy = (static_cast<float>(gy) + 0.5f) * kCombatTilePx;
                // Milestone 188: a wall cell draws combatWallTileShape
                // instead of the ordinary floor tile.
                sf::RectangleShape& tileShape =
                    (combatSession.battleMap != nullptr && combatSession.battleMap->isWall(gx, gy)) ? combatWallTileShape
                                                                                                      : combatTileShape;
                tileShape.setPosition(sf::Vector2f(cx - (kCombatTilePx - 2.f) / 2.f, cy - (kCombatTilePx - 2.f) / 2.f));
                window.draw(tileShape);
            }
        }
        combatGridBorder.setSize(sf::Vector2f(combatPxW, combatPxH));
        combatGridBorder.setPosition(sf::Vector2f(0.f, 0.f));
        window.draw(combatGridBorder);

        constexpr unsigned kAnimGlyphCharSize = 16;
        for (size_t i = 0; i < combatSession.instances.size(); ++i) {
            if (combatSession.instances[i].hp <= 0) continue;
            const combat::GridPos pos = combatSession.instancePositions[i];
            // Milestone 190: same footprint-centered/scaled marker as the
            // main combat draw branch below -- see its own comment.
            const float footprintRadius =
                kCombatTilePx * 0.32f *
                static_cast<float>(std::max(combatSession.monster.footprintWidth, combatSession.monster.footprintHeight));
            const float cx = (static_cast<float>(pos.x) + combatSession.monster.footprintWidth / 2.f) * kCombatTilePx;
            const float cy = (static_cast<float>(pos.y) + combatSession.monster.footprintHeight / 2.f) * kCombatTilePx;
            // -wide/-tall/-four art only ever renders bigger than the real
            // footprint into a neighbor cell confirmed empty this frame --
            // see drawCombatSpriteToken's own comment.
            auto monsterNeighborFree = [&](int dx, int dy) {
                const combat::GridPos neighbor{pos.x + dx, pos.y + dy};
                if (combatCellOccupied(neighbor)) return false;
                return !(neighbor.x == combatSession.playerPos.x && neighbor.y == combatSession.playerPos.y);
            };
            if (!drawCombatSpriteToken(combatSession.monster.id, combatSession.monster.footprintWidth,
                                        combatSession.monster.footprintHeight, cx, cy, pos, std::nullopt, false,
                                        monsterNeighborFree)) {
                combatMonsterMarker.setRadius(footprintRadius);
                combatMonsterMarker.setOrigin(sf::Vector2f(footprintRadius, footprintRadius));
                combatMonsterMarker.setPosition(sf::Vector2f(cx, cy));
                window.draw(combatMonsterMarker);
                sf::Text glyph(font, std::string(1, static_cast<char>('A' + i)), kAnimGlyphCharSize);
                glyph.setFillColor(sf::Color::White);
                glyph.setPosition(sf::Vector2f(cx - 5.f, cy - 10.f));
                window.draw(glyph);
            }
        }
        // Companions/player have no -wide/-tall/-four art yet, so the
        // neighbor check is never actually consulted for them.
        auto noNeighborCheckNeeded = [](int, int) { return true; };
        for (size_t i = 0; i < combatSession.companionPositions.size(); ++i) {
            if (!combatCompanionAlive(i)) continue;
            const combat::GridPos pos = combatSession.companionPositions[i];
            const float cx = (static_cast<float>(pos.x) + 0.5f) * kCombatTilePx;
            const float cy = (static_cast<float>(pos.y) + 0.5f) * kCombatTilePx;
            if (!drawCombatSpriteToken(state.companions[i].id, 1, 1, cx, cy, pos, std::nullopt, false,
                                        noNeighborCheckNeeded)) {
                combatCompanionMarker.setPosition(sf::Vector2f(cx, cy));
                window.draw(combatCompanionMarker);
                sf::Text glyph(font, std::string(1, static_cast<char>('c' + i)), kAnimGlyphCharSize);
                glyph.setFillColor(sf::Color::White);
                glyph.setPosition(sf::Vector2f(cx - 5.f, cy - 10.f));
                window.draw(glyph);
            }
        }
        {
            const float cx = (static_cast<float>(combatSession.playerPos.x) + 0.5f) * kCombatTilePx;
            const float cy = (static_cast<float>(combatSession.playerPos.y) + 0.5f) * kCombatTilePx;
            if (!drawCombatSpriteToken("player", 1, 1, cx, cy, combatSession.playerPos, combatNearestLivingEnemyPos(),
                                        playerAttackPose, noNeighborCheckNeeded)) {
                playerMarker.setPosition(sf::Vector2f(cx, cy));
                window.draw(playerMarker);
            }
        }
        window.display();
        sf::sleep(sf::milliseconds(playerAttackPose ? kAttackPoseFlashMs : kAiStepAnimationMs));
    };

    // Combat sprite art (see docs/ARCHITECTURE.md's SFML section): flashes
    // the player's attack-pose frame for one frame via combatAnimateAiStep
    // above (reusing its exact draw-then-sleep animation, just centered on
    // the player rather than a moving AI unit), then lets the caller's own
    // next real draw revert to idle. A no-op in effect when no player
    // sprite is loaded -- drawCombatSpriteToken's fallback path never
    // distinguishes idle/attack for the plain marker, so nothing visibly
    // changes until real art exists.
    auto combatFlashPlayerAttackPose = [&]() { combatAnimateAiStep(combatSession.playerPos, true); };

    // Milestone 186 (the VIEW command, and the sidebar's passive status
    // readout): pure presentation over state this project already tracks
    // -- no new mechanic, just surfacing what combatApplySpellEffect
    // (above) already sets. Companions have no equivalent (no per-
    // companion buff/debuff tracking exists anywhere in this engine yet),
    // so there's no combatCompanionStatusTags -- their card/roster line
    // simply carries no status text, an accurate reflection of what's
    // real rather than a gap to paper over.
    auto combatPlayerStatusTags = [&]() -> std::vector<std::string> {
        std::vector<std::string> tags;
        if (combatSession.hasteAttackMultiplier > 1) tags.push_back("Hasted");
        if (combatSession.playerThac0Bonus != 0) {
            tags.push_back((combatSession.playerThac0Bonus > 0 ? "+" : "") +
                           std::to_string(combatSession.playerThac0Bonus) + " THAC0");
        }
        if (combatSession.playerDamageBonus != 0) {
            tags.push_back((combatSession.playerDamageBonus > 0 ? "+" : "") +
                           std::to_string(combatSession.playerDamageBonus) + " damage");
        }
        if (combatSession.playerAcBonus != 0) {
            tags.push_back((combatSession.playerAcBonus > 0 ? "+" : "-") +
                           std::to_string(std::abs(combatSession.playerAcBonus)) + " AC");
        }
        if (combatSession.globeActive) tags.push_back("Globe of Invulnerability active");
        return tags;
    };
    auto combatMonsterStatusTags = [&](int idx) -> std::vector<std::string> {
        std::vector<std::string> tags;
        const size_t i = static_cast<size_t>(idx);
        if (combatSession.incapacitatedRestOfFight[i]) tags.push_back("Held");
        if (combatSession.blockedAttacksRemaining[i] > 0) {
            tags.push_back("Blocked (" + std::to_string(combatSession.blockedAttacksRemaining[i]) +
                            " more attack(s))");
        }
        // Both stored as a positive "penalty" (see combat::resolveMonsterAttack:
        // attackerThac0 = monster.thac0 + thac0Penalty, a HIGHER number is a
        // WORSE THAC0) -- shown as-is (with its sign), not negated, so a
        // positive value reads as the real direction: worse for the monster.
        if (combatSession.monsterThac0Penalty[i] != 0) {
            const int penalty = combatSession.monsterThac0Penalty[i];
            tags.push_back((penalty > 0 ? "+" : "") + std::to_string(penalty) + " THAC0 (harder for it to hit you)");
        }
        if (combatSession.monsterDamagePenalty[i] != 0) {
            tags.push_back("-" + std::to_string(combatSession.monsterDamagePenalty[i]) + " damage");
        }
        if (combatSession.monsterAcPenalty[i] != 0) {
            const int penalty = combatSession.monsterAcPenalty[i];
            tags.push_back((penalty > 0 ? "+" : "") + std::to_string(penalty) + " AC (easier to hit)");
        }
        return tags;
    };

    // Awards steel/XP (and applies any resulting level-up) the moment one
    // instance's HP reaches 0, then Sivak's real death-burst (Dragonlance
    // Adventures p.75) if this monster has one. Returns true if the burst
    // just knocked the player out, so callers stop swinging immediately --
    // mirrors GameLoop::handleInstanceDeath exactly.
    auto combatHandleInstanceDeath = [&](int idx) -> bool {
        state.monsterKills[combatSession.monster.id] += 1;
        checkQuestReadiness(); // a SLAY objective may have just been satisfied
        int steel = std::max(0, character::roll(combatSession.monster.steelDiceCount,
                                                  combatSession.monster.steelDiceSides) +
                                     combatSession.monster.steelFlatBonus);
        state.character.steelPieces += steel;
        std::string name = combatMonsterLabel(idx);
        if (combatSession.monster.id == "baaz") {
            combatPushLog("The " + name + " falls and its body crumbles to stone! You find " +
                                         std::to_string(steel) + " steel among the rubble.");
        } else {
            combatPushLog("The " + name + " falls! You find " + std::to_string(steel) + " steel.");
        }
        if (combatSession.monster.xpValue > 0) {
            state.character.experience += combatSession.monster.xpValue;
            combatPushLog("You gain " + std::to_string(combatSession.monster.xpValue) +
                                         " experience.");
            character::applyPendingLevelUps(state.character, combatSession.log);
        }
        if (combatSession.monster.burstsIntoFlameOnDeath) {
            int burstDamage = character::roll(2, 4);
            state.character.currentHp -= burstDamage;
            combatPushLog("As it falls, the " + name + " bursts into flame! You take " +
                                         std::to_string(burstDamage) + " damage.");
            if (state.character.currentHp <= 0) {
                combatKnockedOutBy(combatSession.monster.name);
                return true;
            }
        }
        return false;
    };

    // Real, sourced opportunity attack (DQoK.pdf's own manual): retreating
    // from an instance you were adjacent to gives it one free swing.
    auto combatTriggerOpportunityAttacks = [&](combat::GridPos destination) {
        for (size_t i = 0; i < combatSession.instances.size() && state.character.currentHp > 0; ++i) {
            if (combatSession.instances[i].hp <= 0) continue;
            bool leavingReach =
                combat::isAdjacentToFootprint(combatSession.playerPos, combatSession.instancePositions[i],
                                               combatSession.monster.footprintWidth,
                                               combatSession.monster.footprintHeight) &&
                !combat::isAdjacentToFootprint(destination, combatSession.instancePositions[i],
                                                combatSession.monster.footprintWidth,
                                                combatSession.monster.footprintHeight);
            if (!leavingReach) continue;
            // A Web/Hold-style blocked or incapacitated instance can't
            // swing at all, opportunity attack included; an active Brooch
            // globe absorbs it before it's even rolled -- mirrors
            // GameLoop.cpp:2182's own guard on this exact loop.
            if (combatSession.globeActive || combatSession.incapacitatedRestOfFight[i] ||
                combatSession.blockedAttacksRemaining[i] > 0) {
                continue;
            }
            std::string name = combatMonsterLabel(static_cast<int>(i));
            combat::AttackOutcome outcome = combat::resolveMonsterAttack(
                combatSession.monster, state.character, combatSession.playerAcBonus,
                combatSession.monsterThac0Penalty[i], combatSession.monsterDamagePenalty[i]);
            if (outcome.hit) {
                state.character.currentHp -= outcome.damage;
                combatPushLog("As you pull back, the " + name + " gets a free strike! It hits you for " +
                                             std::to_string(outcome.damage) + ".");
            } else {
                combatPushLog("The " + name + " lunges as you pull back, but misses.");
            }
        }
    };

    // Closes out a round: victory (every instance down), knockout (player
    // at 0 HP), or just advances to the next round -- mirrors the tail end
    // of GameLoop::runCombat's own `for(;;)` body.
    auto combatWrapUpRound = [&]() {
        if (combatAliveCount() == 0) {
            combatPushLog(combatSession.useLetters
                                             ? ("The " + pluralMonsterName(combatSession.monster.name) + " are defeated!")
                                             : ("You defeated the " + combatSession.monster.name + "."));
            pushLog(combatSession.useLetters
                        ? ("You defeated the " + pluralMonsterName(combatSession.monster.name) + ".")
                        : ("You defeated the " + combatSession.monster.name + "."));
            combatSession.uiState = CombatUiState::Won;
            combatSession.roundJustConcluded = true;
            return;
        }
        if (combatCheckPlayerDown(combatSession.monster.name)) return;
        ++combatSession.roundNumber;
        // Milestone 185: fresh movement budget and a fresh initiative roll
        // for the round about to start.
        combatSession.movementRemaining = character::movementSquares(state.character);
        combatSession.initiativeRolledThisRound = false;
        combatSession.uiState = CombatUiState::Idle;
        combatSession.roundJustConcluded = true;
    };

    // Milestone 204: paces combatSession.pendingMessageAutoAdvance's timer
    // (see that field's own doc comment) -- restarted every time a line
    // advances, by either path below, so each line gets its own full
    // kMessageAutoAdvanceMs on screen.
    sf::Clock combatMessageClock;

    // Advances the AwaitContinue message queue by one line, applying the
    // real target state once the queue drains -- shared by the manual
    // Enter-press handler (AwaitContinue's own key-dispatch case, below)
    // and the automatic timer-driven advance (the main loop, guarded on
    // pendingMessageAutoAdvance) so both paths use identical logic instead
    // of duplicating it.
    auto combatAdvancePendingMessage = [&]() {
        ++combatSession.pendingMessageIndex;
        if (combatSession.pendingMessageIndex >= combatSession.pendingMessageEnd) {
            combatSession.uiState = combatSession.pendingTargetState;
        }
        combatMessageClock.restart();
    };

    // id == 0 is the player; id == N (N >= 1) is state.companions[N-1] --
    // same identity encoding as CombatSession::firstAttackerId. Direct port
    // of GameLoop.cpp:2035-2037.
    auto combatPositionOfAttacker = [&](int id) -> combat::GridPos {
        return id == 0 ? combatSession.playerPos : combatSession.companionPositions[static_cast<size_t>(id - 1)];
    };

    // Thief backstab (Milestone 119 in the console build): DQoK.pdf's own
    // manual, "A thief 'back stabs' if he attacks a target from exactly
    // opposite the first character to attack the target." Records
    // `attackerId` as the first attacker on `targetIndex` if none is
    // recorded yet (returns {0, 1}, no bonus); otherwise returns the
    // backstab bonus (+4 to-hit, PHB Table 30's level-based damage
    // multiplier) when `attacker` is a Thief-type in light-or-no armor
    // (character::canBackstab) standing exactly opposite
    // (combat::oppositeSide) that first attacker's CURRENT position --
    // {0, 1} otherwise, including when the first attacker was a companion
    // who has since been knocked out (no living ally left to flank
    // around). Shared by the player's own attack loop and
    // combatCompanionActs below, direct port of GameLoop.cpp:2048-2065.
    auto combatBackstabBonus = [&](const character::Character& attacker, int attackerId,
                                    combat::GridPos attackerPos, int targetIndex) -> std::pair<int, int> {
        size_t idx = static_cast<size_t>(targetIndex);
        if (combatSession.firstAttackerId[idx] < 0) {
            combatSession.firstAttackerId[idx] = attackerId;
            return {0, 1};
        }
        if (combatSession.firstAttackerId[idx] == attackerId) return {0, 1}; // can't backstab around yourself
        if (combatSession.firstAttackerId[idx] > 0 &&
            !combatCompanionAlive(static_cast<size_t>(combatSession.firstAttackerId[idx] - 1))) {
            return {0, 1};
        }
        combat::GridPos anchor = combatPositionOfAttacker(combatSession.firstAttackerId[idx]);
        combat::GridPos opposite = combat::oppositeSide(combatSession.instancePositions[idx], anchor);
        if (attackerPos.x == opposite.x && attackerPos.y == opposite.y && character::canBackstab(attacker)) {
            return {4, character::backstabDamageMultiplier(attacker.level)};
        }
        return {0, 1};
    };

    // Fighter-type sweep (Milestone 119 in the console build): DQoK.pdf's
    // own manual, "Fighter-types may also 'sweep' through several weak
    // opponents in one combat round." Every alive instance adjacent to
    // `pos`, or empty when this fight's monster isn't "weak"
    // (combat::isSweepEligible) -- an encounter is always N copies of one
    // combat::Monster, so eligibility is a single check for the whole
    // fight, not per-instance. Callers additionally require the result to
    // have 2+ entries before treating this as a sweep (a single adjacent
    // weak enemy is just an ordinary attack). Direct port of
    // GameLoop.cpp:2076-2085.
    // Not footprint-aware (bare isAdjacent, not isAdjacentToFootprint): a
    // sweep-eligible monster (isSweepEligible: hpDiceCount <= 1, this
    // roster's "weak" line troops -- Goblin/Kobold/Hobgoblin/Skeleton) is
    // never one of Milestone 190's SIZE-carrying monsters (Ogre/Troll/
    // Griffon/Dragon, all HD4+), so this path never runs against a
    // multi-cell footprint in practice.
    auto combatAdjacentWeakInstances = [&](combat::GridPos pos) -> std::vector<int> {
        std::vector<int> result;
        if (!combat::isSweepEligible(combatSession.monster)) return result;
        for (size_t i = 0; i < combatSession.instances.size(); ++i) {
            if (combatSession.instances[i].hp > 0 && combat::isAdjacent(pos, combatSession.instancePositions[i])) {
                result.push_back(static_cast<int>(i));
            }
        }
        return result;
    };

    // Every alive companion's own turn, AI-controlled (player-directed
    // party control stays out of scope project-wide -- see
    // docs/COMBAT_NOTES.md's "Extending this later"): attacks the first
    // adjacent alive instance found, or takes one combat::stepToward step
    // toward the nearest alive instance if none is adjacent yet. Fighter
    // sweep and Thief backstab both apply (see combatAdjacentWeakInstances/
    // combatBackstabBonus above); still no mid-round retarget-on-kill (this
    // phase's one-target-per-round simplification, see CombatUiState's own
    // doc comment) -- a real simplification versus GameLoop::companionActs.
    auto combatCompanionActs = [&]() {
        for (size_t ci = 0; ci < state.companions.size(); ++ci) {
            if (combatSession.uiState == CombatUiState::Lost) return;
            if (!combatCompanionAlive(ci)) continue;
            combatSession.currentTurnActor = {CombatSession::ViewCandidate::Kind::Companion, static_cast<int>(ci)};
            character::Character& companion = state.companions[ci].character;
            combat::GridPos& companionPos = combatSession.companionPositions[ci];
            int targetIndex = -1;
            for (size_t i = 0; i < combatSession.instances.size(); ++i) {
                if (combatSession.instances[i].hp > 0 &&
                    combat::isAdjacentToFootprint(companionPos, combatSession.instancePositions[i],
                                                   combatSession.monster.footprintWidth,
                                                   combatSession.monster.footprintHeight)) {
                    targetIndex = static_cast<int>(i);
                    break;
                }
            }
            if (targetIndex < 0) {
                combat::GridPos nearest{};
                int nearestDist = -1;
                for (size_t i = 0; i < combatSession.instances.size(); ++i) {
                    if (combatSession.instances[i].hp <= 0) continue;
                    combat::GridPos nearestCell = combat::nearestFootprintCell(
                        companionPos, combatSession.instancePositions[i], combatSession.monster.footprintWidth,
                        combatSession.monster.footprintHeight);
                    int dist = combat::chebyshevDistance(companionPos, nearestCell);
                    if (nearestDist < 0 || dist < nearestDist) {
                        nearestDist = dist;
                        nearest = nearestCell;
                    }
                }
                if (nearestDist < 0) continue;
                std::vector<combat::GridPos> blocked{combatSession.playerPos};
                for (size_t i = 0; i < combatSession.instances.size(); ++i) {
                    if (combatSession.instances[i].hp > 0) {
                        // Milestone 190: every cell of a footprint>1x1
                        // instance blocks companion pathing, not just its
                        // anchor -- degenerates to the single existing cell
                        // for every ordinary 1x1 monster.
                        std::vector<combat::GridPos> cells = combat::footprintCells(
                            combatSession.instancePositions[i], combatSession.monster.footprintWidth,
                            combatSession.monster.footprintHeight);
                        blocked.insert(blocked.end(), cells.begin(), cells.end());
                    }
                }
                for (size_t oi = 0; oi < state.companions.size(); ++oi) {
                    if (oi != ci && combatCompanionAlive(oi)) blocked.push_back(combatSession.companionPositions[oi]);
                }
                // Milestone 188: walls block companion pathing same as any
                // occupied cell -- appended to the same `blocked` list
                // rather than threaded through as a separate parameter.
                blocked.insert(blocked.end(), combatSession.wallPositions.begin(), combatSession.wallPositions.end());
                // Milestone 185: closes the gap up to the companion's own
                // movement budget in one turn (previously always exactly one
                // step), stopping early once adjacent to its target -- the
                // real target itself never moves during this loop (monster
                // instances only act in their own separate combatMonstersAct
                // turn), so `nearest` stays valid to re-check against.
                // Milestone 188: combat::stepTowardBfs (a real shortest-path
                // step, replacing stepToward's greedy single-axis heuristic)
                // so a companion actually routes around a wall instead of
                // getting stuck against it -- see that function's own doc
                // comment. stepToward itself is unchanged and still used by
                // the console build, which has no walls.
                const int budget = character::movementSquares(companion);
                for (int step = 0; step < budget; ++step) {
                    combat::GridPos next =
                        combat::stepTowardBfs(companionPos, nearest, kCombatGridWidth, kCombatGridHeight, blocked);
                    if (next.x == companionPos.x && next.y == companionPos.y) break; // fully blocked
                    companionPos = next;
                    combatAnimateAiStep(companionPos); // walk one square at a time, camera follows -- see its own doc comment
                    if (combat::isAdjacent(companionPos, nearest)) break;
                }
                continue;
            }
            // Identity used by combatBackstabBonus's firstAttackerId
            // tracking -- 0 is the player, so a companion is ci+1. Shared by
            // the sweep block below and the ordinary attack loop after it.
            int attackerId = static_cast<int>(ci) + 1;
            // Fighter-type companion sweep -- same rule and same
            // combatAdjacentWeakInstances helper as the player's own sweep
            // in combatConfirmAim. Ends this companion's turn
            // (continue) rather than falling into the single-target loop
            // below. Direct port of GameLoop.cpp:2347-2380.
            if (character::classGroupFor(companion.charClass) == character::ClassGroup::Warrior) {
                std::vector<int> weakTargets = combatAdjacentWeakInstances(companionPos);
                if (weakTargets.size() >= 2) {
                    combatPushLog(companion.name + " sweeps through the " +
                                                 pluralMonsterName(combatSession.monster.name) + "!");
                    for (int idx : weakTargets) {
                        if (combatSession.instances[static_cast<size_t>(idx)].hp <= 0) continue;
                        std::string targetName = combatMonsterLabel(idx);
                        auto [backstabThac0, backstabMultiplier] =
                            combatBackstabBonus(companion, attackerId, companionPos, idx);
                        combat::AttackOutcome outcome = combat::resolvePlayerAttack(
                            companion, combatSession.monster, backstabThac0, 0, backstabMultiplier,
                            combatSession.monsterAcPenalty[static_cast<size_t>(idx)]);
                        if (outcome.hit) {
                            combatSession.instances[static_cast<size_t>(idx)].hp -= outcome.damage;
                            combatPushLog(std::string(backstabMultiplier > 1 ? "Backstab! " : "") +
                                                         companion.name + " hits the " + targetName + " for " +
                                                         std::to_string(outcome.damage) + ".");
                            if (combatSession.instances[static_cast<size_t>(idx)].hp <= 0) {
                                if (combatHandleInstanceDeath(idx)) return;
                            }
                        } else {
                            combatPushLog(companion.name + " misses the " + targetName + ".");
                        }
                    }
                    continue;
                }
            }
            int attacks = character::meleeAttacksThisRound(companion.charClass, companion.level,
                                                             combatSession.roundNumber, false);
            for (int i = 0; i < attacks; ++i) {
                if (combatSession.instances[static_cast<size_t>(targetIndex)].hp <= 0) break;
                std::string targetName = combatMonsterLabel(targetIndex);
                auto [backstabThac0, backstabMultiplier] =
                    combatBackstabBonus(companion, attackerId, companionPos, targetIndex);
                // A slowed instance (Slow's AC penalty) is easier for every
                // attacker to hit, not just whoever cast it -- see
                // GameLoop.cpp:2362-2364's own companion-attack call.
                combat::AttackOutcome outcome = combat::resolvePlayerAttack(
                    companion, combatSession.monster, backstabThac0, 0, backstabMultiplier,
                    combatSession.monsterAcPenalty[static_cast<size_t>(targetIndex)]);
                if (outcome.hit) {
                    combatSession.instances[static_cast<size_t>(targetIndex)].hp -= outcome.damage;
                    combatPushLog(std::string(backstabMultiplier > 1 ? "Backstab! " : "") +
                                                 companion.name + " hits the " + targetName + " for " +
                                                 std::to_string(outcome.damage) + ".");
                    if (combatSession.instances[static_cast<size_t>(targetIndex)].hp <= 0) {
                        if (combatHandleInstanceDeath(targetIndex)) return;
                    }
                } else {
                    combatPushLog(companion.name + " misses the " + targetName + ".");
                }
            }
        }
    };

    // Every alive monster instance's own turn: Bozak's Magic Missile and
    // Aurak's breath weapon (both real, sourced, passive specials that
    // fire on their own turn with no player choice involved -- see this
    // file's top-of-file comment for why these stayed in scope even before
    // the player's own spellcasting did) take priority over its plain
    // weapon attack; otherwise it attacks whichever of the player/alive
    // companions it's adjacent to (uniformly at random if more than one),
    // or closes on whichever is nearest. Mirrors GameLoop::monstersAct.
    struct CombatPartyTarget {
        combat::GridPos pos;
        character::Character* character;
        bool isPlayer;
    };
    auto combatMonstersAct = [&]() {
        for (size_t i = 0; i < combatSession.instances.size() && state.character.currentHp > 0; ++i) {
            if (combatSession.instances[i].hp <= 0) continue;
            combatSession.currentTurnActor = {CombatSession::ViewCandidate::Kind::Monster, static_cast<int>(i)};
            std::string name = combatMonsterLabel(static_cast<int>(i));
            // Web/Hold-style block (character::SpellEffect::
            // BlockMonsterAttacks) -- checked before even Magic Missile/
            // breath weapon, same ordering as GameLoop.cpp:2438-2446.
            if (combatSession.incapacitatedRestOfFight[i]) {
                combatPushLog("The " + name + " is unable to act!");
                continue;
            }
            if (combatSession.blockedAttacksRemaining[i] > 0) {
                --combatSession.blockedAttacksRemaining[i];
                combatPushLog("The " + name + " can't bring itself to attack!");
                continue;
            }
            const combat::Monster& monster = combatSession.monster;
            // The Brooch's globe wards the player only (see the melee check
            // below) but Magic Missile/breath weapon always target the
            // player directly (Milestone 117 never taught either to pick a
            // companion instead), so a plain globeActive check is enough
            // here -- mirrors GameLoop.cpp:2463/2482.
            if (monster.castsMagicMissile && character::roll(1, 100) <= monster.magicMissileChancePercent) {
                // Milestone 200: a small paced beat ahead of the outcome --
                // pure presentation, the roll above already decided this
                // monster IS casting this turn, no new mechanic.
                combatPushLog("The " + name + " begins casting a spell!");
                if (combatSession.globeActive) {
                    combatPushLog("The globe of invulnerability absorbs the " + name +
                                                 "'s Magic Missile!");
                    continue;
                }
                int missileDamage = (character::roll(1, 4) + 1) + (character::roll(1, 4) + 1);
                state.character.currentHp -= missileDamage;
                combatPushLog("The " + name + " casts Magic Missile! It strikes you for " +
                                             std::to_string(missileDamage) + " -- no saving throw.");
                continue;
            }
            // Generalized at Milestone 190 (previously Aurak-only
            // hardcoded) -- see GameLoop.cpp's identical breath weapon
            // block and docs/COMBAT_NOTES.md.
            if (monster.hasBreathWeapon && character::roll(1, 100) <= monster.breathWeaponChancePercent) {
                // Milestone 200: same paced anticipation beat as Magic
                // Missile above.
                combatPushLog("The " + name + " rears back to unleash its breath weapon!");
                if (combatSession.globeActive) {
                    combatPushLog("The globe of invulnerability absorbs the " + name +
                                                 "'s breath weapon!");
                    continue;
                }
                int fullDamage = character::roll(monster.breathDamageDiceCount, monster.breathDamageDiceSides) +
                                 monster.breathDamageFlatBonus;
                if (combat::rollSavingThrow(state.character, character::SaveCategory::BreathWeapon)) {
                    int halfDamage = fullDamage / 2;
                    state.character.currentHp -= halfDamage;
                    combatPushLog("The " + name + " breathes a " + monster.breathWeaponName +
                                                 "! You resist -- " + std::to_string(halfDamage) + " damage.");
                } else {
                    state.character.currentHp -= fullDamage;
                    if (monster.breathWeaponBlindsOnFail) combatSession.playerThac0Bonus -= 4;
                    combatPushLog(
                        "The " + name + " breathes a " + monster.breathWeaponName + "! It burns you for " +
                        std::to_string(fullDamage) + " damage" +
                        (monster.breathWeaponBlindsOnFail ? " and blinds you" : "") + ".");
                }
                continue;
            }
            std::vector<CombatPartyTarget> party{{combatSession.playerPos, &state.character, true}};
            for (size_t pi = 0; pi < state.companions.size(); ++pi) {
                if (combatCompanionAlive(pi)) {
                    party.push_back({combatSession.companionPositions[pi], &state.companions[pi].character, false});
                }
            }
            std::vector<size_t> adjacentTargets;
            for (size_t pi = 0; pi < party.size(); ++pi) {
                if (combat::isAdjacentToFootprint(party[pi].pos, combatSession.instancePositions[i],
                                                   monster.footprintWidth, monster.footprintHeight)) {
                    adjacentTargets.push_back(pi);
                }
            }
            if (adjacentTargets.empty()) {
                size_t nearest = 0;
                int nearestDist = -1;
                for (size_t pi = 0; pi < party.size(); ++pi) {
                    combat::GridPos nearestCell = combat::nearestFootprintCell(
                        party[pi].pos, combatSession.instancePositions[i], monster.footprintWidth,
                        monster.footprintHeight);
                    int dist = combat::chebyshevDistance(nearestCell, party[pi].pos);
                    if (nearestDist < 0 || dist < nearestDist) {
                        nearestDist = dist;
                        nearest = pi;
                    }
                }
                std::vector<combat::GridPos> blocked;
                for (const CombatPartyTarget& target : party) blocked.push_back(target.pos);
                for (size_t j = 0; j < combatSession.instances.size(); ++j) {
                    if (j != i && combatSession.instances[j].hp > 0) blocked.push_back(combatSession.instancePositions[j]);
                }
                // Milestone 188: walls block monster pathing same as any
                // occupied cell -- see combatCompanionActs' identical
                // append above.
                blocked.insert(blocked.end(), combatSession.wallPositions.begin(), combatSession.wallPositions.end());
                // Milestone 185: closes the gap up to the monster's own
                // MOVE (moveSquares) in one turn instead of always exactly
                // one step, stopping early once adjacent -- party[nearest]'s
                // own position stays fixed for this loop (the party doesn't
                // move during the monsters' turn).
                // Milestone 188: combat::stepTowardBfs -- see
                // combatCompanionActs' identical swap above for why.
                // Milestone 190: a multi-cell footprint (footprintWidth *
                // footprintHeight > 1 -- Ogre/Troll/Griffon/Dragon) instead
                // uses stepFootprintToward, the simpler greedy-with-fallback
                // heuristic stepToward/stepTowardBfs itself replaced at
                // Milestone 188, validated against the WHOLE footprint --
                // see that function's own doc comment for why no multi-cell
                // BFS was attempted. Every ordinary 1x1 monster's path is
                // completely unchanged.
                bool multiCell = monster.footprintWidth * monster.footprintHeight > 1;
                bool moved = false;
                for (int step = 0; step < monster.moveSquares; ++step) {
                    combat::GridPos next =
                        multiCell ? combat::stepFootprintToward(combatSession.instancePositions[i], party[nearest].pos,
                                                                  monster.footprintWidth, monster.footprintHeight,
                                                                  kCombatGridWidth, kCombatGridHeight, blocked)
                                  : combat::stepTowardBfs(combatSession.instancePositions[i], party[nearest].pos,
                                                           kCombatGridWidth, kCombatGridHeight, blocked);
                    if (next.x == combatSession.instancePositions[i].x && next.y == combatSession.instancePositions[i].y) {
                        break; // fully blocked
                    }
                    combatSession.instancePositions[i] = next;
                    moved = true;
                    combatAnimateAiStep(combatSession.instancePositions[i]); // walk one square at a time, camera follows
                    if (combat::isAdjacentToFootprint(party[nearest].pos, combatSession.instancePositions[i],
                                                       monster.footprintWidth, monster.footprintHeight)) {
                        break;
                    }
                }
                if (moved) combatPushLog("The " + name + " closes in.");
                continue;
            }
            size_t chosen = adjacentTargets.size() == 1
                                 ? adjacentTargets.front()
                                 : adjacentTargets[static_cast<size_t>(
                                       character::roll(1, static_cast<int>(adjacentTargets.size())) - 1)];
            const CombatPartyTarget& target = party[chosen];
            // The Brooch's globe wards the player only (see docs/
            // COMBAT_NOTES.md's "Extending this later") -- it does NOT
            // protect any companion, so this check only applies once the
            // resolved target is actually the player. Mirrors
            // GameLoop.cpp:2549-2556.
            if (target.isPlayer && combatSession.globeActive) {
                combatPushLog("The globe of invulnerability absorbs the blow from the " + name + "!");
                continue;
            }
            std::string targetName = target.isPlayer ? "you" : target.character->name;
            // playerAcBonus (Protection from Evil/Shield/...) only wards the
            // player, same as GameLoop.cpp:2559's own target.isPlayer check
            // -- it never applies to a companion.
            combat::AttackOutcome outcome =
                combat::resolveMonsterAttack(monster, *target.character, target.isPlayer ? combatSession.playerAcBonus : 0,
                                              combatSession.monsterThac0Penalty[i], combatSession.monsterDamagePenalty[i]);
            if (outcome.hit) {
                target.character->currentHp -= outcome.damage;
                combatPushLog("The " + name + " hits " + targetName + " for " +
                                             std::to_string(outcome.damage) + ".");
                if (monster.poisonOnHit) {
                    if (combat::rollSavingThrow(*target.character, character::SaveCategory::ParalyzationPoisonDeath)) {
                        combatPushLog(target.isPlayer ? "You resist the poison."
                                                                     : targetName + " resists the poison.");
                    } else {
                        combatPushLog("The poison overwhelms " + targetName + "!");
                        target.character->currentHp = 0;
                    }
                }
                if (!target.isPlayer && target.character->currentHp <= 0) {
                    target.character->currentHp = 0;
                    combatPushLog(targetName + " is knocked out!");
                }
            } else {
                combatPushLog("The " + name + " misses " + targetName + ".");
            }
        }
    };

    // PHB p.124: rolls which side acts first, and if the monsters do, runs
    // their turn immediately (before the player has even chosen an
    // action) -- mirrors GameLoop::runCombat's own dispatch. Returns false
    // if that already ended the fight, so the caller (about to resolve a
    // move/attack) knows to stop.
    //
    // Idempotent per round (Milestone 185): a no-op returning true once
    // initiativeRolledThisRound is already set, so every action this
    // round -- a move, then later an attack, say -- can call this
    // unconditionally without re-rolling initiative or re-running the
    // monsters' whole turn a second time. Only the first action of the
    // round actually rolls/acts.
    auto combatRollGoFirstAndMaybeActMonsters = [&]() -> bool {
        if (combatSession.initiativeRolledThisRound) return true;
        combatSession.initiativeRolledThisRound = true;
        combatSession.pendingGoFirst = combat::playerActsFirst();
        if (!combatSession.pendingGoFirst) {
            combatMonstersAct();
            if (combatCheckPlayerDown(combatSession.monster.name)) return false;
        }
        return true;
    };

    // Runs the rest of the round once the player's own action (move,
    // attack, or an attack's target confirmation) has resolved: companions
    // act, then -- only if the player's side went FIRST, since otherwise
    // monsters already acted before the player's turn even began -- the
    // monsters act, then the round is closed out.
    auto combatFinishPlayerAction = [&](bool goFirst) {
        combatCompanionActs();
        if (combatSession.uiState == CombatUiState::Lost) return;
        if (goFirst) {
            if (combatAliveCount() > 0) combatMonstersAct();
            if (combatCheckPlayerDown(combatSession.monster.name)) return;
        }
        combatWrapUpRound();
    };

    // Resolves every swing of the player's real attacks-per-round (PHB
    // Table 15/35) against one already-chosen target, stopping early if it
    // falls -- this phase's one-target-per-round simplification (see
    // CombatUiState's doc comment) means a swing that would have retargeted
    // onto a fresh instance in the console version is simply skipped here
    // instead.
    auto combatResolveAttackAgainstTarget = [&](int targetIndex) {
        // hasteAttackMultiplier (Haste, PHB p.192): multiplies the whole
        // attacks-per-round count, same as GameLoop.cpp:2257-2259.
        int attacks = character::meleeAttacksThisRound(state.character.charClass, state.character.level,
                                                         combatSession.roundNumber, state.character.specializedWeapon) *
                      combatSession.hasteAttackMultiplier;
        for (int i = 0; i < attacks; ++i) {
            if (combatSession.instances[static_cast<size_t>(targetIndex)].hp <= 0) break;
            std::string targetName = combatMonsterLabel(targetIndex);
            auto [backstabThac0, backstabMultiplier] =
                combatBackstabBonus(state.character, 0, combatSession.playerPos, targetIndex);
            combat::AttackOutcome outcome = combat::resolvePlayerAttack(
                state.character, combatSession.monster, combatSession.playerThac0Bonus + backstabThac0,
                combatSession.playerDamageBonus, backstabMultiplier,
                combatSession.monsterAcPenalty[static_cast<size_t>(targetIndex)]);
            combatFlashPlayerAttackPose(); // Combat sprite art: brief attack-pose frame, whether the swing hits or misses
            if (outcome.hit) {
                combatSession.instances[static_cast<size_t>(targetIndex)].hp -= outcome.damage;
                combatPushLog(std::string(backstabMultiplier > 1 ? "Backstab! " : "") + "You hit the " +
                                             targetName + " for " + std::to_string(outcome.damage) + ".");
                if (combatSession.instances[static_cast<size_t>(targetIndex)].hp <= 0) {
                    if (combatHandleInstanceDeath(targetIndex)) return;
                }
            } else {
                combatPushLog("You miss the " + targetName + ".");
            }
        }
    };

    // Direct port of GameLoop::playerCasts's own switch (GameLoop.cpp:
    // 2616-2722), minus character::castSpell (already run by the caller --
    // see combatCommitSpellChoice below) and minus target-picking (already
    // resolved, passed in as targetIndex -- -1 for the self-only effects:
    // HealCaster and the three player-only buffs). Every SpellEffect case
    // is handled, so any sourced spell in character::spellListFor resolves
    // correctly here, not just a hand-picked subset.
    auto combatApplySpellEffect = [&](const character::SpellCastResult& result, int targetIndex) {
        std::string targetName = targetIndex >= 0 ? combatMonsterLabel(targetIndex) : combatSession.monster.name;
        switch (result.effect) {
            case character::SpellEffect::DamageMonster:
                combatSession.instances[static_cast<size_t>(targetIndex)].hp -= result.amount;
                combatPushLog("Your " + result.spellName + " strikes the " + targetName + " for " +
                                             std::to_string(result.amount) + ".");
                if (combatSession.instances[static_cast<size_t>(targetIndex)].hp <= 0) {
                    if (combatHandleInstanceDeath(targetIndex)) return;
                }
                break;
            case character::SpellEffect::DamageArea: {
                // Fireball/Delayed Blast Fireball (PHB p.193): the picked
                // target's cell is the burst's epicenter, and every alive
                // instance within result.radius cells (Chebyshev distance)
                // takes the same single damage roll -- see
                // GameLoop::playerCasts's own DamageArea case.
                combat::GridPos epicenter = combatSession.instancePositions[static_cast<size_t>(targetIndex)];
                std::vector<int> hitTargets;
                for (size_t i = 0; i < combatSession.instances.size(); ++i) {
                    if (combatSession.instances[i].hp <= 0) continue;
                    if (combat::chebyshevDistance(epicenter, combatSession.instancePositions[i]) <= result.radius) {
                        hitTargets.push_back(static_cast<int>(i));
                    }
                }
                std::vector<std::string> hitNames;
                for (int idx : hitTargets) hitNames.push_back(combatMonsterLabel(idx));
                std::string names;
                for (size_t i = 0; i < hitNames.size(); ++i) {
                    if (i > 0) names += (i + 1 == hitNames.size()) ? " and " : ", ";
                    names += hitNames[i];
                }
                combatPushLog("Your " + result.spellName + " engulfs the " + names + " for " +
                                             std::to_string(result.amount) + (hitNames.size() > 1 ? " each." : "."));
                for (int idx : hitTargets) {
                    // Mirrors GameLoop::playerCasts's own fightAlreadyEnded
                    // check here -- once a death-burst from an earlier
                    // target in this same blast has already knocked the
                    // player out, later targets in the blast stop taking
                    // damage too.
                    if (combatSession.uiState == CombatUiState::Lost) return;
                    if (combatSession.instances[static_cast<size_t>(idx)].hp <= 0) continue;
                    combatSession.instances[static_cast<size_t>(idx)].hp -= result.amount;
                    if (combatSession.instances[static_cast<size_t>(idx)].hp <= 0) {
                        combatHandleInstanceDeath(idx);
                    }
                }
                break;
            }
            case character::SpellEffect::HealCaster: {
                int healed = std::min(result.amount, state.character.maxHp - state.character.currentHp);
                state.character.currentHp += healed;
                combatPushLog("You cast " + result.spellName + " and heal " +
                                             std::to_string(healed) + " hit points.");
                break;
            }
            case character::SpellEffect::BlockMonsterAttacks:
                if (result.amount == character::kBlockRestOfFight) {
                    combatSession.incapacitatedRestOfFight[static_cast<size_t>(targetIndex)] = true;
                } else {
                    combatSession.blockedAttacksRemaining[static_cast<size_t>(targetIndex)] += result.amount;
                }
                combatPushLog("You cast " + result.spellName + " on the " + targetName + "!");
                break;
            case character::SpellEffect::BuffPlayerThac0:
                combatSession.playerThac0Bonus += result.amount;
                combatPushLog("You cast " + result.spellName + ".");
                break;
            case character::SpellEffect::BuffPlayerDamage:
                combatSession.playerDamageBonus += result.amount;
                combatPushLog("You cast " + result.spellName + ".");
                break;
            case character::SpellEffect::BuffPlayerAc:
                combatSession.playerAcBonus += result.amount;
                combatPushLog("You cast " + result.spellName + ".");
                break;
            case character::SpellEffect::DebuffMonsterThac0:
                combatSession.monsterThac0Penalty[static_cast<size_t>(targetIndex)] += result.amount;
                combatPushLog("You cast " + result.spellName + " on the " + targetName + "!");
                break;
            case character::SpellEffect::DebuffMonsterDamage:
                combatSession.monsterDamagePenalty[static_cast<size_t>(targetIndex)] += result.amount;
                combatPushLog("You cast " + result.spellName + " on the " + targetName + "!");
                break;
            case character::SpellEffect::BuffPlayerAndDebuffMonsterThac0:
                combatSession.playerThac0Bonus += result.amount;
                combatSession.monsterThac0Penalty[static_cast<size_t>(targetIndex)] += result.amount;
                combatPushLog("You cast " + result.spellName + ".");
                break;
            case character::SpellEffect::HastePlayer:
                combatSession.hasteAttackMultiplier = result.amount;
                combatPushLog("You cast " + result.spellName + "! Your attacks quicken.");
                break;
            case character::SpellEffect::DebuffMonsterThac0AndAc:
                combatSession.monsterThac0Penalty[static_cast<size_t>(targetIndex)] += result.amount;
                combatSession.monsterAcPenalty[static_cast<size_t>(targetIndex)] += result.amount;
                combatPushLog("You cast " + result.spellName + " on the " + targetName + "!");
                break;
            case character::SpellEffect::InstantDefeat:
                combatPushLog("Your " + result.spellName + " destroys the " + targetName +
                                             " outright!");
                combatSession.instances[static_cast<size_t>(targetIndex)].hp = 0;
                if (combatHandleInstanceDeath(targetIndex)) return;
                break;
        }
    };

    // A spell has been definitively chosen (the only one memorized, or
    // picked from PickingSpell) -- rolls initiative now, exactly like
    // GameLoop::playerCasts only ever running as part of the shared
    // playerActsFirst() dispatch (GameLoop.cpp:3009-3021), AFTER any spell/
    // target choice. If the monsters go first and that already knocks the
    // player out, the spell is never cast at all -- same real console
    // behavior (its own castSpell() call is inside playerCasts, itself
    // gated on the player still being conscious, GameLoop.cpp:3017-3020).
    auto combatCommitSpellChoice = [&](const std::string& spellId) {
        combatSession.actionLogStart = combatSession.log.size();
        combatSession.roundJustConcluded = false;
        combatSession.currentTurnActor = {CombatSession::ViewCandidate::Kind::Player, 0};
        if (!combatRollGoFirstAndMaybeActMonsters()) return;
        combatSession.currentTurnActor = {CombatSession::ViewCandidate::Kind::Player, 0};
        character::SpellCastResult result = character::castSpell(state.character, spellId);
        if (!result.success) { // defensive -- shouldn't happen, combatBeginCast already checked
            combatFinishPlayerAction(combatSession.pendingGoFirst);
            return;
        }
        bool needsTarget = result.effect == character::SpellEffect::DamageMonster ||
                            result.effect == character::SpellEffect::DamageArea ||
                            result.effect == character::SpellEffect::BlockMonsterAttacks ||
                            result.effect == character::SpellEffect::DebuffMonsterThac0 ||
                            result.effect == character::SpellEffect::DebuffMonsterDamage ||
                            result.effect == character::SpellEffect::BuffPlayerAndDebuffMonsterThac0 ||
                            result.effect == character::SpellEffect::DebuffMonsterThac0AndAc ||
                            result.effect == character::SpellEffect::InstantDefeat;
        if (!needsTarget) {
            combatApplySpellEffect(result, -1);
            if (combatSession.uiState == CombatUiState::Lost) return;
            combatFinishPlayerAction(combatSession.pendingGoFirst);
            return;
        }
        // Spell targeting is never adjacency-restricted (PHB spell ranges
        // aren't modeled on the grid) -- every alive instance is eligible,
        // matching GameLoop.cpp:1935's own anyAlive filter for casting --
        // except Milestone 189's real sightline check, now that battlemap
        // walls exist (DamageArea's "target" is Fireball/Delayed Blast
        // Fireball's epicenter cell -- everyone within its radius still
        // takes the blast once the epicenter itself is confirmed visible,
        // matching this project's existing restraint against modeling 2e's
        // own burst-vs-wall diagramming rules).
        std::vector<int> candidates;
        for (size_t i = 0; i < combatSession.instances.size(); ++i) {
            if (combatSession.instances[i].hp <= 0) continue;
            // Milestone 190: LOS targets the nearest cell of a footprint>1x1
            // instance (degenerates to the bare anchor for every ordinary
            // 1x1 monster) -- a shooter/caster peeking around a corner can
            // see the near edge of a big creature even if its far edge is
            // behind a wall.
            combat::GridPos losTarget = combat::nearestFootprintCell(
                combatSession.playerPos, combatSession.instancePositions[i], combatSession.monster.footprintWidth,
                combatSession.monster.footprintHeight);
            if (!combat::hasLineOfSight(combatSession.playerPos, losTarget, combatSession.wallPositions)) continue;
            candidates.push_back(static_cast<int>(i));
        }
        // A genuinely new state once line of sight is real: every alive
        // instance used to be unconditionally eligible, so this couldn't
        // happen before. The spell is already cast (character::castSpell
        // ran above) and the round is spent either way -- same "a wasted
        // action still costs the round" precedent combatConfirmAim's own
        // post-initiative re-check falls back to when a physical attack's
        // target stops being legal after all.
        if (candidates.empty()) {
            combatPushLog("Your " + result.spellName + " finds no target in sight.");
            combatFinishPlayerAction(combatSession.pendingGoFirst);
            return;
        }
        if (candidates.size() == 1) {
            combatApplySpellEffect(result, candidates.front());
            if (combatSession.uiState == CombatUiState::Lost) return;
            combatFinishPlayerAction(combatSession.pendingGoFirst);
            return;
        }
        combatSession.pickReason = TargetPickReason::Spell;
        combatSession.pendingSpellResult = result;
        combatSession.pickCandidates = candidates;
        combatSession.pickSelected = 0;
        combatSession.uiState = CombatUiState::PickingTarget;
    };

    // M pressed while Idle: mirrors GameLoop::runCombat's own Cast key
    // handling (GameLoop.cpp:2869-2929) up through choosing WHICH spell --
    // validated, and (for 2+ distinct memorized spells) chosen, entirely
    // before initiative is rolled, same as the console's own blocking
    // spell-choice loop running before its shared playerActsFirst()
    // dispatch. No round is consumed by an invalid Cast or a cancelled
    // spell choice (see PickingSpell's own Escape/Q handling in the key
    // dispatch below).
    auto combatBeginCast = [&]() {
        if (!character::canCastSpells(state.character.charClass)) {
            combatPushLog("You have no spell to cast.");
            return;
        }
        if (!character::hasMemorizedSpellsAvailable(state.character)) {
            combatPushLog("You have no spells remaining today.");
            return;
        }
        std::vector<std::string> distinctIds;
        for (const std::string& id : state.character.memorizedSpellIds) {
            if (std::find(distinctIds.begin(), distinctIds.end(), id) == distinctIds.end()) {
                distinctIds.push_back(id);
            }
        }
        if (distinctIds.size() == 1) {
            combatCommitSpellChoice(distinctIds.front());
            return;
        }
        combatSession.spellPickIds = distinctIds;
        combatSession.spellPickLabels.clear();
        for (const std::string& id : distinctIds) {
            const character::SpellInfo* spell = character::findSpell(state.character.charClass, id);
            int count = static_cast<int>(std::count(state.character.memorizedSpellIds.begin(),
                                                       state.character.memorizedSpellIds.end(), id));
            combatSession.spellPickLabels.push_back((spell != nullptr ? spell->name : id) +
                                                      (count > 1 ? " (x" + std::to_string(count) + ")" : ""));
        }
        combatSession.spellPickSelected = 0;
        combatSession.uiState = CombatUiState::PickingSpell;
    };

    // An item has been definitively chosen (the only one usable, or picked
    // from PickingItem) -- rolls initiative now, exactly like
    // GameLoop::runCombat's own item-use lambdas (playerDrinksPotion/
    // playerUsesWebnet/playerActivatesBrooch/playerUsesStaffCure,
    // GameLoop.cpp:2724-2759) only ever running as part of the shared
    // playerActsFirst() dispatch: if the monsters go first and that already
    // knocks the player out, the item is never actually used -- same
    // real console behavior combatCommitSpellChoice already established for
    // Cast.
    auto combatCommitItemChoice = [&](character::CombatItemKind kind) {
        combatSession.actionLogStart = combatSession.log.size();
        combatSession.roundJustConcluded = false;
        combatSession.currentTurnActor = {CombatSession::ViewCandidate::Kind::Player, 0};
        if (!combatRollGoFirstAndMaybeActMonsters()) return;
        combatSession.currentTurnActor = {CombatSession::ViewCandidate::Kind::Player, 0};
        const long long today = state.hoursElapsed / 24;
        switch (kind) {
            case character::CombatItemKind::Potion: {
                character::PurchaseResult result =
                    character::drinkPotion(state.character, character::firstPotionIndex(state.character));
                combatPushLog(result.message);
                combatFinishPlayerAction(combatSession.pendingGoFirst);
                break;
            }
            case character::CombatItemKind::Webnet: {
                character::PurchaseResult result =
                    character::useWebnet(state.character, character::firstWebnetIndex(state.character));
                combatPushLog(result.message);
                if (!result.success) {
                    combatFinishPlayerAction(combatSession.pendingGoFirst);
                    break;
                }
                // Webnet targeting is never adjacency-restricted, same
                // "every alive instance is eligible" rule spell targeting
                // follows (GameLoop.cpp:1935's anyAlive filter) -- mirrors
                // GameLoop::playerUsesWebnet's own pickTarget(..., anyAlive)
                // call.
                std::vector<int> candidates;
                for (size_t i = 0; i < combatSession.instances.size(); ++i) {
                    if (combatSession.instances[i].hp > 0) candidates.push_back(static_cast<int>(i));
                }
                if (candidates.size() == 1) {
                    // No extra log line here -- matches
                    // GameLoop::playerUsesWebnet's own silent
                    // ++blockedAttacksRemaining[...] once pickTarget
                    // returns.
                    ++combatSession.blockedAttacksRemaining[static_cast<size_t>(candidates.front())];
                    combatFinishPlayerAction(combatSession.pendingGoFirst);
                    break;
                }
                combatSession.pickReason = TargetPickReason::Webnet;
                combatSession.pickCandidates = candidates;
                combatSession.pickSelected = 0;
                combatSession.uiState = CombatUiState::PickingTarget;
                break;
            }
            case character::CombatItemKind::Brooch: {
                character::PurchaseResult result = character::activateBrooch(state.character, today);
                combatPushLog(result.message);
                if (result.success) combatSession.globeActive = true;
                combatFinishPlayerAction(combatSession.pendingGoFirst);
                break;
            }
            case character::CombatItemKind::StaffCure: {
                character::PurchaseResult result = character::useStaffCure(state.character, today);
                combatPushLog(result.message);
                combatFinishPlayerAction(combatSession.pendingGoFirst);
                break;
            }
        }
    };

    // I pressed while Idle: mirrors GameLoop::runCombat's own Inventory-key
    // handling (GameLoop.cpp:2931-2985) up through choosing WHICH item --
    // built fresh each time from character::availableCombatItems (Potion/
    // Webnet/Brooch/StaffCure, in that fixed order), validated and (for 2+
    // usable items) chosen entirely before initiative is rolled, same as
    // combatBeginCast above. No round is consumed by "nothing to use" or a
    // cancelled choice (see PickingItem's own Escape/Q handling in the key
    // dispatch below).
    auto combatBeginUseItem = [&]() {
        std::vector<character::CombatItem> usable =
            character::availableCombatItems(state.character, state.hoursElapsed / 24);
        if (usable.empty()) {
            combatPushLog("You have nothing to use.");
            return;
        }
        if (usable.size() == 1) {
            combatCommitItemChoice(usable.front().kind);
            return;
        }
        combatSession.itemPickKinds.clear();
        combatSession.itemPickLabels.clear();
        for (const character::CombatItem& item : usable) {
            combatSession.itemPickKinds.push_back(item.kind);
            combatSession.itemPickLabels.push_back(item.label);
        }
        combatSession.itemPickSelected = 0;
        combatSession.uiState = CombatUiState::PickingItem;
    };

    // Milestone 204: whether `instanceIdx` is a legal physical-attack target
    // RIGHT NOW, and if not, the exact reason -- shared by the Aiming
    // status line (evaluated live every frame the cursor moves, at zero
    // cost) and combatConfirmAim's actual commit (re-evaluated once more
    // right after initiative rolls, since monster movement earlier in the
    // round can change the answer -- see that lambda's own comment).
    // Melee-locked (must be adjacent) unless wielding the Light Crossbow,
    // which can hit anyone on the grid in sight but is disabled outright
    // the instant an enemy closes to melee range (DQoK.pdf's own manual) --
    // mirrors GameLoop::playerAttacks/the old combatBeginPlayerAttack this
    // replaces. Kept as one small function instead of inlined so both call
    // sites can't silently drift apart.
    auto combatAimLegality = [&](int instanceIdx) -> std::pair<bool, std::string> {
        bool hasRangedWeapon = state.character.weaponName == character::kLightCrossbowName;
        if (hasRangedWeapon) {
            for (size_t i = 0; i < combatSession.instances.size(); ++i) {
                if (combatSession.instances[i].hp > 0 &&
                    combat::isAdjacentToFootprint(combatSession.playerPos, combatSession.instancePositions[i],
                                                   combatSession.monster.footprintWidth,
                                                   combatSession.monster.footprintHeight)) {
                    return {false, "An enemy is too close to fire your crossbow!"};
                }
            }
            combat::GridPos losTarget = combat::nearestFootprintCell(
                combatSession.playerPos, combatSession.instancePositions[static_cast<size_t>(instanceIdx)],
                combatSession.monster.footprintWidth, combatSession.monster.footprintHeight);
            if (!combat::hasLineOfSight(combatSession.playerPos, losTarget, combatSession.wallPositions)) {
                return {false, "Nothing in your line of sight."};
            }
            return {true, ""};
        }
        if (!combat::isAdjacentToFootprint(combatSession.playerPos,
                                            combatSession.instancePositions[static_cast<size_t>(instanceIdx)],
                                            combatSession.monster.footprintWidth,
                                            combatSession.monster.footprintHeight)) {
            return {false, "Too far away to attack."};
        }
        return {true, ""};
    };

    // 'a' pressed while Idle: real DQoK "AIM" semantics (see this
    // milestone's own writeup in docs/COMBAT_NOTES.md) -- opens a free
    // look-around cursor (the Aiming case in the key dispatch below) rather
    // than immediately attempting an attack. Costs nothing: no initiative
    // roll, no monster turn, until Enter actually commits to a legal
    // target via combatConfirmAim. Starts on the nearest living instance
    // (combatNearestLivingEnemyPos, falling back to the player's own
    // position on the never-really-possible case of no living instances)
    // so the common case -- something's already adjacent -- still needs
    // just one more Enter, same feel the old immediate-resolve path had.
    auto combatBeginAim = [&]() {
        combatSession.aimCursor = combatNearestLivingEnemyPos().value_or(combatSession.playerPos);
        combatSession.uiState = CombatUiState::Aiming;
    };

    // Enter pressed while Aiming: commits to attacking whatever the cursor
    // is currently on, if legal -- a silent no-op otherwise (the status
    // line already shows why; there's nothing to confirm). This is the
    // ONLY point in the whole Aim flow that costs a round: rolls initiative
    // now (idempotent, same call every other committing action uses), then
    // re-checks combatAimLegality once more for the SAME instance --
    // monster AI only ever closes distance, never retreats, so the one way
    // this can flip from the pre-roll check above is a DIFFERENT monster
    // closing to melee and disabling an already-aimed ranged shot; same
    // "still costs the round" consequence the old immediate-resolve path
    // always had once initiative was rolled. Fighter-type sweep is
    // re-checked here too, at the exact same point relative to initiative
    // the old combatBeginPlayerAttack had it (after the roll, before
    // resolving a single target) -- moved here from that function
    // (Milestone 204) rather than left at Aim-entry, so a monster that
    // closes in THIS round can still trigger it, not just one already
    // adjacent when 'a' was first pressed.
    auto combatConfirmAim = [&]() {
        const int idx = combatInstanceAtCell(combatSession.aimCursor);
        if (idx < 0 || !combatAimLegality(idx).first) return;
        combatSession.actionLogStart = combatSession.log.size();
        combatSession.roundJustConcluded = false;
        combatSession.currentTurnActor = {CombatSession::ViewCandidate::Kind::Player, 0};
        if (!combatRollGoFirstAndMaybeActMonsters()) return;
        combatSession.currentTurnActor = {CombatSession::ViewCandidate::Kind::Player, 0};
        auto [stillLegal, reason] = combatAimLegality(idx);
        if (!stillLegal) {
            combatPushLog(reason);
            combatFinishPlayerAction(combatSession.pendingGoFirst);
            return;
        }
        // Fighter-type sweep bypasses the single-target resolve below
        // entirely -- one swing per adjacent weak instance, no to-hit/
        // damage bonus (DQoK's own wording gives none; sweeping is an
        // action-economy ability only). See combatAdjacentWeakInstances's
        // doc comment. Direct port of GameLoop.cpp:2214-2245.
        if (character::classGroupFor(state.character.charClass) == character::ClassGroup::Warrior) {
            std::vector<int> weakTargets = combatAdjacentWeakInstances(combatSession.playerPos);
            if (weakTargets.size() >= 2) {
                combatPushLog("You sweep through the " +
                                             pluralMonsterName(combatSession.monster.name) + "!");
                for (int targetIndex : weakTargets) {
                    if (combatSession.instances[static_cast<size_t>(targetIndex)].hp <= 0) continue;
                    std::string targetName = combatMonsterLabel(targetIndex);
                    auto [backstabThac0, backstabMultiplier] =
                        combatBackstabBonus(state.character, 0, combatSession.playerPos, targetIndex);
                    combat::AttackOutcome outcome = combat::resolvePlayerAttack(
                        state.character, combatSession.monster, combatSession.playerThac0Bonus + backstabThac0,
                        combatSession.playerDamageBonus, backstabMultiplier,
                        combatSession.monsterAcPenalty[static_cast<size_t>(targetIndex)]);
                    combatFlashPlayerAttackPose(); // Combat sprite art: one flash per sweep swing, hit or miss
                    if (outcome.hit) {
                        combatSession.instances[static_cast<size_t>(targetIndex)].hp -= outcome.damage;
                        combatPushLog(std::string(backstabMultiplier > 1 ? "Backstab! " : "") +
                                                     "You hit the " + targetName + " for " +
                                                     std::to_string(outcome.damage) + ".");
                        if (combatSession.instances[static_cast<size_t>(targetIndex)].hp <= 0) {
                            // A death-burst knockout mid-sweep -- combatFinishPlayerAction is still called
                            // unconditionally below (it and combatCompanionActs both no-op safely once
                            // uiState is Lost), same pattern the single-target resolve below uses.
                            if (combatHandleInstanceDeath(targetIndex)) break;
                        }
                    } else {
                        combatPushLog("You miss the " + targetName + ".");
                    }
                }
                combatFinishPlayerAction(combatSession.pendingGoFirst);
                return;
            }
        }
        combatResolveAttackAgainstTarget(idx);
        combatFinishPlayerAction(combatSession.pendingGoFirst);
    };

    // combatCellOccupied itself now lives up near combatCompanionAlive (see
    // its own comment there for why it moved).

    // Milestone 188: true if `cell` is a wall on this fight's battlemap
    // (see CombatSession::battleMap/wallPositions above) -- nullptr/empty
    // means no battlemap was found for this terrain, treated as an
    // all-open floor, the pre-Milestone-188 behavior.
    auto combatCellIsWall = [&](combat::GridPos cell) {
        return combatSession.battleMap != nullptr && combatSession.battleMap->isWall(cell.x, cell.y);
    };

    // A direction key pressed while Idle: takes one step out of this
    // round's movement budget (Milestone 185, character::movementSquares --
    // DQoK.pdf p.51) rather than ending the round outright, so the player
    // can take several steps (and still attack/cast/use an item/flee
    // afterward) in one round, same as DQoK's own manual: "the character's
    // movement range is displayed... during the character's segment in
    // combat." Validated up front (bounds/occupancy/remaining budget)
    // before it ever costs a step; a valid move still triggers opportunity
    // attacks from anyone being left adjacent. combatRollGoFirstAndMaybeActMonsters
    // is idempotent per round (see its own doc comment) -- only the FIRST
    // move or attack of a round actually rolls initiative/runs the
    // monsters' turn.
    auto combatBeginPlayerMove = [&](int dx, int dy) {
        combatSession.actionLogStart = combatSession.log.size();
        combatSession.roundJustConcluded = false;
        combatSession.currentTurnActor = {CombatSession::ViewCandidate::Kind::Player, 0};
        if (combatSession.movementRemaining <= 0) {
            combatPushLog("You have no movement left this round.");
            return;
        }
        combat::GridPos destination{combatSession.playerPos.x + dx, combatSession.playerPos.y + dy};
        if (destination.x < 0 || destination.x >= kCombatGridWidth || destination.y < 0 ||
            destination.y >= kCombatGridHeight) {
            combatPushLog("You can't move that way.");
            return;
        }
        if (combatCellIsWall(destination)) {
            combatPushLog("Blocked: cannot walk onto a wall.");
            return;
        }
        // Milestone 188: same corner-cutting rule Milestone 187 already
        // applies to overworld/zone diagonal movement (see this file's own
        // Overworld-branch key handler) -- a diagonal step also needs both
        // flanking cardinal cells wall-free, not just the destination
        // itself, or the player could squeeze diagonally between two
        // walls. Checks walls only, not occupancy: corner-cutting is about
        // solid terrain geometry, not another combatant's square (the
        // exact gap Milestone 187's own doc comment flagged as needing
        // revisiting once real wall geometry existed).
        if (dx != 0 && dy != 0 &&
            (combatCellIsWall({combatSession.playerPos.x + dx, combatSession.playerPos.y}) ||
             combatCellIsWall({combatSession.playerPos.x, combatSession.playerPos.y + dy}))) {
            combatPushLog("Blocked: can't cut across the wall.");
            return;
        }
        if (combatCellOccupied(destination)) {
            combatPushLog("Something's in the way.");
            return;
        }
        if (!combatRollGoFirstAndMaybeActMonsters()) return;
        combatSession.currentTurnActor = {CombatSession::ViewCandidate::Kind::Player, 0};
        combatTriggerOpportunityAttacks(destination);
        if (combatCheckPlayerDown(combatSession.monster.name)) return;
        // Re-check: whatever just acted above may have moved into
        // `destination` itself (see combatCellOccupied's own comment) --
        // only possible on this round's very first step, since nothing
        // else moves between one of the player's own steps and the next.
        // Just cancels this one step (no round/movement cost) rather than
        // ending the round the way this used to -- the player still has
        // their full movement budget and action left to spend differently.
        if (combatCellOccupied(destination)) {
            combatPushLog("The way is blocked now.");
            return;
        }
        // Milestone 187: an 8-way lookup, not the old 4-way ternary chain
        // (which silently mislabeled every diagonal move as just "north"
        // or "south", dy taking priority over dx by construction) -- now
        // that keypad diagonal movement can actually produce dx AND dy
        // both nonzero in the same step.
        const int ddx = destination.x - combatSession.playerPos.x;
        const int ddy = destination.y - combatSession.playerPos.y;
        std::string dirLabel;
        if (ddy < 0 && ddx == 0) dirLabel = "north";
        else if (ddy > 0 && ddx == 0) dirLabel = "south";
        else if (ddx < 0 && ddy == 0) dirLabel = "west";
        else if (ddx > 0 && ddy == 0) dirLabel = "east";
        else if (ddx < 0 && ddy < 0) dirLabel = "northwest";
        else if (ddx > 0 && ddy < 0) dirLabel = "northeast";
        else if (ddx < 0 && ddy > 0) dirLabel = "southwest";
        else dirLabel = "southeast";
        combatSession.playerPos = destination;
        --combatSession.movementRemaining;
        combatPushLog("You move " + dirLabel + ".");
    };

    // F pressed while Idle: an unconditional escape -- unlike attack/move,
    // this never goes through the initiative dispatch above (mirrors
    // GameLoop::runCombat, where Flee is checked and handled before
    // anything else in its round loop), so no monster gets a free action.
    auto combatBeginFlee = [&]() {
        combatSession.actionLogStart = combatSession.log.size();
        combatSession.roundJustConcluded = false;
        combatSession.currentTurnActor = {CombatSession::ViewCandidate::Kind::Player, 0};
        combatPushLog("You break off and retreat.");
        pushLog("You fled from the " + combatSession.monster.name + ".");
        combatSession.uiState = CombatUiState::Fled;
        combatSession.roundJustConcluded = true;
    };

    // Space pressed while Idle (Milestone 185, new): deliberately end the
    // round without attacking -- the counterpart to a round that now takes
    // several move steps before an action (see combatBeginPlayerMove's own
    // doc comment). Still rolls initiative/lets the monsters go first if
    // this is the round's first action (a player who presses Space
    // immediately, having not moved at all, shouldn't skip that check).
    auto combatEndTurn = [&]() {
        combatSession.actionLogStart = combatSession.log.size();
        combatSession.roundJustConcluded = false;
        combatSession.currentTurnActor = {CombatSession::ViewCandidate::Kind::Player, 0};
        if (!combatRollGoFirstAndMaybeActMonsters()) return;
        combatSession.currentTurnActor = {CombatSession::ViewCandidate::Kind::Player, 0};
        combatPushLog("You hold your action.");
        combatFinishPlayerAction(combatSession.pendingGoFirst);
    };

    // 'v' pressed while Idle (Milestone 186): a pure info window, same
    // "costs no round" treatment as Help/Journal/the character sheet --
    // deliberately does NOT call combatRollGoFirstAndMaybeActMonsters, so
    // looking someone up never gives the monsters a free turn. Builds the
    // full roster (player, every alive companion, every alive monster
    // instance) as candidates for the picker that follows.
    auto combatBeginView = [&]() {
        combatSession.viewCandidates.clear();
        combatSession.viewCandidates.push_back({CombatSession::ViewCandidate::Kind::Player, 0});
        for (size_t i = 0; i < state.companions.size(); ++i) {
            if (combatCompanionAlive(i)) {
                combatSession.viewCandidates.push_back(
                    {CombatSession::ViewCandidate::Kind::Companion, static_cast<int>(i)});
            }
        }
        for (size_t i = 0; i < combatSession.instances.size(); ++i) {
            if (combatSession.instances[i].hp > 0) {
                combatSession.viewCandidates.push_back(
                    {CombatSession::ViewCandidate::Kind::Monster, static_cast<int>(i)});
            }
        }
        combatSession.viewSelected = 0;
        combatSession.uiState = CombatUiState::ViewPicking;
    };

    // Human-readable label for one viewCandidates entry -- shared by the
    // picker list and combatConfirmView's own card title below.
    auto combatViewCandidateLabel = [&](const CombatSession::ViewCandidate& candidate) -> std::string {
        switch (candidate.kind) {
            case CombatSession::ViewCandidate::Kind::Player:
                return state.character.name;
            case CombatSession::ViewCandidate::Kind::Companion:
                return state.companions[static_cast<size_t>(candidate.index)].character.name;
            case CombatSession::ViewCandidate::Kind::Monster:
                return combatMonsterLabel(candidate.index);
        }
        return ""; // unreachable -- every Kind handled above
    };

    // Enter pressed while PickingTarget: commit to the highlighted
    // candidate and resolve the rest of the round. Spell/Webnet only now --
    // a physical Attack used to be a third reason here, moved to its own
    // free-look Aiming cursor at Milestone 204 (combatConfirmAim).
    auto combatConfirmTarget = [&]() {
        // Deliberately does NOT re-arm actionLogStart/roundJustConcluded --
        // PickingTarget is only ever entered from combatCommitSpellChoice/
        // combatCommitItemChoice, both of which already snapshotted
        // actionLogStart at their own top before this picker ever opened.
        // Re-arming here would silently drop any log line pushed before
        // the picker opened -- confirmed this actually happens:
        // combatCommitItemChoice's Webnet case pushes the "you use the
        // Webnet" result.message (main.cpp ~4725) before checking
        // candidate count, so a 2+-candidate Webnet throw would otherwise
        // lose that line from the paced replay.
        int target = combatSession.pickCandidates[static_cast<size_t>(combatSession.pickSelected)];
        switch (combatSession.pickReason) {
            case TargetPickReason::Spell:
                combatApplySpellEffect(combatSession.pendingSpellResult, target);
                break;
            case TargetPickReason::Webnet:
                // No extra log line -- matches GameLoop::playerUsesWebnet's
                // own silent ++blockedAttacksRemaining[...] once pickTarget
                // returns.
                ++combatSession.blockedAttacksRemaining[static_cast<size_t>(target)];
                break;
        }
        if (combatSession.uiState == CombatUiState::Lost) return;
        combatFinishPlayerAction(combatSession.pendingGoFirst);
    };

    // Builds a read-only stat card for one ViewCandidate. Reuses THAC0/AC/HP
    // straight from the relevant character::Character (player/companion) or
    // combatSession.monster + combatSession.instances (monster instance,
    // which has no individually-named weapon -- its dice line stands in for
    // one). Factored out of combatConfirmView (Milestone 199) so the new
    // DQoK-style persistent player card and PickingTarget's secondary target
    // card can build the identical card data View already did, instead of
    // duplicating this switch.
    auto combatBuildCardLines = [&](const CombatSession::ViewCandidate& candidate) -> std::vector<std::string> {
        std::vector<std::string> lines;
        if (candidate.kind == CombatSession::ViewCandidate::Kind::Monster) {
            const int idx = candidate.index;
            const CombatInstance& inst = combatSession.instances[static_cast<size_t>(idx)];
            const combat::Monster& monster = combatSession.monster;
            lines.push_back("HP " + std::to_string(std::max(0, inst.hp)) + "/" + std::to_string(inst.maxHp));
            lines.push_back("AC " + std::to_string(monster.armorClass));
            lines.push_back("THAC0 " + std::to_string(monster.thac0));
            lines.push_back(
                "Damage " + std::to_string(monster.damageDiceCount) + "d" +
                std::to_string(monster.damageDiceSides) +
                (monster.damageFlatBonus != 0
                     ? (monster.damageFlatBonus > 0 ? "+" : "") + std::to_string(monster.damageFlatBonus)
                     : ""));
            std::vector<std::string> tags = combatMonsterStatusTags(idx);
            if (!tags.empty()) {
                std::string joined = "Status: ";
                for (size_t i = 0; i < tags.size(); ++i) joined += (i > 0 ? ", " : "") + tags[i];
                lines.push_back(joined);
            }
        } else {
            const character::Character& c = candidate.kind == CombatSession::ViewCandidate::Kind::Player
                                                  ? state.character
                                                  : state.companions[static_cast<size_t>(candidate.index)].character;
            lines.push_back("HP " + std::to_string(std::max(0, c.currentHp)) + "/" + std::to_string(c.maxHp));
            lines.push_back("AC " + std::to_string(c.armorClass));
            lines.push_back("THAC0 " + std::to_string(c.thac0));
            lines.push_back("Weapon: " + c.weaponName);
            // combatPlayerStatusTags is player-only (see its own doc
            // comment) -- there is no per-companion equivalent, so a
            // companion's card simply has no Status line at all.
            if (candidate.kind == CombatSession::ViewCandidate::Kind::Player) {
                std::vector<std::string> tags = combatPlayerStatusTags();
                if (!tags.empty()) {
                    std::string joined = "Status: ";
                    for (size_t i = 0; i < tags.size(); ++i) joined += (i > 0 ? ", " : "") + tags[i];
                    lines.push_back(joined);
                }
            }
        }
        return lines;
    };

    // Compact DQoK-format card (Milestone 200): the real card sampled from
    // References/BattleFrames_extracted/ is exactly NAME / HITPOINTS
    // (current only, no max) / AC / WEAPON -- no THAC0, no Status line.
    // This is a DELIBERATELY separate lambda from combatBuildCardLines
    // above, not a trim of it in place: the dedicated 'v' View command
    // still needs the full detailed card (THAC0 + Status -- Milestone
    // 186's whole point), so it keeps calling combatBuildCardLines
    // unchanged. Only the persistent top-right card and its PickingTarget
    // secondary card use this compact version.
    auto combatBuildCompactCardLines = [&](const CombatSession::ViewCandidate& candidate) -> std::vector<std::string> {
        std::vector<std::string> lines;
        if (candidate.kind == CombatSession::ViewCandidate::Kind::Monster) {
            const CombatInstance& inst = combatSession.instances[static_cast<size_t>(candidate.index)];
            const combat::Monster& monster = combatSession.monster;
            lines.push_back("HITPOINTS " + std::to_string(std::max(0, inst.hp)));
            lines.push_back("AC " + std::to_string(monster.armorClass));
            lines.push_back(
                "Damage " + std::to_string(monster.damageDiceCount) + "d" +
                std::to_string(monster.damageDiceSides) +
                (monster.damageFlatBonus != 0
                     ? (monster.damageFlatBonus > 0 ? "+" : "") + std::to_string(monster.damageFlatBonus)
                     : ""));
        } else {
            const character::Character& c = candidate.kind == CombatSession::ViewCandidate::Kind::Player
                                                  ? state.character
                                                  : state.companions[static_cast<size_t>(candidate.index)].character;
            lines.push_back("HITPOINTS " + std::to_string(std::max(0, c.currentHp)));
            lines.push_back("AC " + std::to_string(c.armorClass));
            lines.push_back("Weapon: " + c.weaponName);
        }
        return lines;
    };

    // Turn-follow camera/card (Milestone 200): resolves a ViewCandidate to
    // its current grid position, so the per-frame camera-focus code and
    // the persistent-card code can both center on whoever
    // CombatSession::currentTurnActor/logActorForLine says is acting.
    // Falls back to the player's own position for Player, or for a
    // Companion/Monster that's no longer alive (e.g. died earlier the same
    // round the paced replay is now walking back through) -- same "harmless
    // fallback for a stale/dead actor" reasoning as combatBackstabBonus's
    // own dead-first-attacker check above.
    auto combatActorPosition = [&](const CombatSession::ViewCandidate& actor) -> combat::GridPos {
        if (actor.kind == CombatSession::ViewCandidate::Kind::Monster) {
            const size_t idx = static_cast<size_t>(actor.index);
            if (idx < combatSession.instances.size() && combatSession.instances[idx].hp > 0) {
                return combatSession.instancePositions[idx];
            }
        } else if (actor.kind == CombatSession::ViewCandidate::Kind::Companion) {
            const size_t idx = static_cast<size_t>(actor.index);
            if (idx < state.companions.size() && combatCompanionAlive(idx)) {
                return combatSession.companionPositions[idx];
            }
        }
        return combatSession.playerPos;
    };

    // Small bordered-box chrome for a content-sized card -- same color
    // trio as drawPanelChrome (kPanelBg/kPanelBorderOuter/
    // kPanelBorderInner) but sized to arbitrary content instead of always
    // the whole window. Introduced for combat's compact card (Milestone
    // 200); Milestone 201 reuses it, unchanged, for the non-combat
    // Overworld/Zone sidebar's stat and log boxes too.
    auto drawCardPanel = [&](float x, float y, float w, float h) {
        sf::RectangleShape bg(sf::Vector2f(w, h));
        bg.setPosition(sf::Vector2f(x, y));
        bg.setFillColor(kPanelBg);
        window.draw(bg);

        sf::RectangleShape outerBorder(sf::Vector2f(w - 6.f, h - 6.f));
        outerBorder.setPosition(sf::Vector2f(x + 3.f, y + 3.f));
        outerBorder.setFillColor(sf::Color::Transparent);
        outerBorder.setOutlineColor(kPanelBorderOuter);
        outerBorder.setOutlineThickness(2.f);
        window.draw(outerBorder);

        sf::RectangleShape innerBorder(sf::Vector2f(w - 12.f, h - 12.f));
        innerBorder.setPosition(sf::Vector2f(x + 6.f, y + 6.f));
        innerBorder.setFillColor(sf::Color::Transparent);
        innerBorder.setOutlineColor(kPanelBorderInner);
        innerBorder.setOutlineThickness(1.f);
        window.draw(innerBorder);
    };

    // Enter pressed while ViewPicking: no round cost, matching
    // combatBeginView's own reasoning.
    auto combatConfirmView = [&]() {
        const CombatSession::ViewCandidate candidate =
            combatSession.viewCandidates[static_cast<size_t>(combatSession.viewSelected)];
        combatSession.viewCardLines = combatBuildCardLines(candidate);
        combatSession.uiState = CombatUiState::ViewingCard;
    };

    // A real random encounter fires (see the movement handling below):
    // builds a fresh CombatSession exactly the way GameLoop::runCombat's
    // own opening lines do (group size, starting grid layout, fight-start
    // bonuses), then hands off to the AwaitContinue dismissal beat.
    auto combatStartEncounter = [&](const combat::Monster& monster) {
        combatSession = CombatSession{};
        combatSession.active = true;
        combatSession.monster = monster;
        int groupSize = combat::rollGroupSize(monster);
        combatSession.useLetters = groupSize > 1;
        combatSession.instances.assign(static_cast<size_t>(groupSize), CombatInstance{});
        for (CombatInstance& inst : combatSession.instances) {
            inst.maxHp = character::roll(monster.hpDiceCount, monster.hpDiceSides) + monster.hpFlatBonus;
            inst.hp = inst.maxHp;
        }
        // Fresh per-instance spell debuff/block state -- same "zeroed for
        // every new fight" initialization as GameLoop.cpp:1967-1978.
        combatSession.monsterThac0Penalty.assign(static_cast<size_t>(groupSize), 0);
        combatSession.monsterDamagePenalty.assign(static_cast<size_t>(groupSize), 0);
        combatSession.monsterAcPenalty.assign(static_cast<size_t>(groupSize), 0);
        combatSession.blockedAttacksRemaining.assign(static_cast<size_t>(groupSize), 0);
        combatSession.incapacitatedRestOfFight.assign(static_cast<size_t>(groupSize), false);
        combatSession.firstAttackerId.assign(static_cast<size_t>(groupSize), -1);
        combatSession.floorTerrainCode = grid.terrainCodeAt(state.x, state.y);
        // Milestone 188: this terrain's wall layout, flattened into a plain
        // list of blocked cells once per fight (rather than re-querying
        // BattleMap::isWall for all 1250 cells on every movement/pathing
        // check) -- see CombatSession::wallPositions's own doc comment.
        combatSession.battleMap = battleMaps.forTerrain(combatSession.floorTerrainCode);
        combatSession.wallPositions.clear();
        if (combatSession.battleMap != nullptr) {
            for (int wy = 0; wy < combatSession.battleMap->height(); ++wy) {
                for (int wx = 0; wx < combatSession.battleMap->width(); ++wx) {
                    if (combatSession.battleMap->isWall(wx, wy)) combatSession.wallPositions.push_back({wx, wy});
                }
            }
        }
        // Milestone 185: round 1's own movement budget -- combatWrapUpRound
        // refreshes this for every round after.
        combatSession.movementRemaining = character::movementSquares(state.character);
        combatSession.playerPos = {kCombatGridWidth / 2, kCombatGridHeight - 1};
        combatSession.instancePositions.resize(static_cast<size_t>(groupSize));
        constexpr int kMonsterSpacing = 2;
        const int centerX = kCombatGridWidth / 2;
        for (int i = 0; i < groupSize; ++i) {
            int offsetIndex = i - (groupSize - 1) / 2;
            combatSession.instancePositions[static_cast<size_t>(i)] = {centerX + offsetIndex * kMonsterSpacing, 0};
        }
        combatSession.companionPositions.resize(state.companions.size());
        for (size_t i = 0; i < combatSession.companionPositions.size(); ++i) {
            int offset = (i % 2 == 0) ? static_cast<int>(i / 2) + 1 : -(static_cast<int>(i / 2) + 1);
            combatSession.companionPositions[i] = {combatSession.playerPos.x + offset, combatSession.playerPos.y};
        }
        if (state.character.weaponName == character::kFrostreaverName &&
            std::string(world::terrainFor(combatSession.floorTerrainCode).name) == "glacier") {
            combatSession.playerThac0Bonus += character::kFrostreaverMagicBonus;
            combatSession.playerDamageBonus += character::kFrostreaverMagicBonus;
            combatPushLog("Your Frostreaver's edge bites keener than steel, sharpened by the glacier's own cold.");
        }
        if (state.character.charClass == character::ClassId::Fighter && state.character.specializedWeapon) {
            combatSession.playerThac0Bonus += character::kWeaponSpecializationToHitBonus;
            combatSession.playerDamageBonus += character::kWeaponSpecializationDamageBonus;
        }
        if (combatSession.useLetters) {
            combatPushLog(std::to_string(groupSize) + " " + pluralMonsterName(monster.name) +
                                         " appear! " + monster.description);
            pushLog(std::to_string(groupSize) + " " + pluralMonsterName(monster.name) + " appear!");
        } else {
            combatPushLog("A " + monster.name + " appears! " + monster.description);
            pushLog("A " + monster.name + " appears!");
        }
        // Message-pacing queue (Milestone 199): the fresh CombatSession{}
        // above already reset uiState to AwaitContinue (its struct default,
        // main.cpp:696-ish) -- this just makes the "X appears!" beat above a
        // degenerate 1-message instance of the same queue every other round-
        // ending action uses, rather than a hardcoded special case.
        combatSession.actionLogStart = 0;
        combatSession.pendingMessageIndex = 0;
        combatSession.pendingMessageEnd = combatSession.log.size();
        combatSession.pendingTargetState = CombatUiState::Idle;
    };

    // Character sheet ('C') -- a read-only, full-window overlay drawn on top
    // of whatever's already rendered this frame (the map+sidebar draw keeps
    // running underneath every frame the sheet is open; this is purely a
    // final compositing layer, so no branch was needed in either of the
    // draw chains above). Content mirrors render::MapRenderer::
    // drawCharacterSheet (MapRenderer.cpp:689-840) field-for-field --
    // including its 's' = full-spellbook drill-down, see spellbookOpen and
    // drawSpellbookOverlay below -- laid out in two real pixel-space
    // columns instead of one long vertical dump, since covering the whole
    // window means there's no 320px sidebar constraint to work within.
    // kSheetTitleCharSize/kSheetHeaderCharSize/kSheetBodyCharSize/
    // kSheetSectionColor/kSheetBodyColor/kSheetMarginX moved up to right
    // after font load -- see the comment there -- so only the two
    // constants specific to this two-column layout stay here.
    const float kSheetRightX = static_cast<float>(windowW) / 2.f + 20.f;
    const float kSheetColumnWidth = static_cast<float>(windowW) / 2.f - kSheetMarginX - 20.f;
    const float kSheetMaxWidthPx = kSheetColumnWidth - 10.f;

    auto drawCharacterSheetOverlay = [&]() {
        drawPanelChrome();

        const character::Character& c = state.character;
        const auto& race = character::raceInfo(c.race);
        const auto& cls = character::classInfo(c.charClass);
        const character::SubraceInfo* sub = character::subraceInfo(c.subrace);

        auto drawAt = [&](float x, float& yRef, const std::string& text, sf::Color color, unsigned size) {
            sf::Text sfText(font, text, size);
            sfText.setFillColor(color);
            sfText.setPosition(sf::Vector2f(x, yRef));
            window.draw(sfText);
            yRef += static_cast<float>(size) + 6.f;
        };

        float headY = 20.f;
        drawAt(kSheetMarginX, headY, c.name, sf::Color::White, kSheetTitleCharSize);

        std::ostringstream classLine;
        classLine << (sub != nullptr ? sub->name : race.name) << " " << cls.name << ", level " << c.level << " ("
                   << c.experience << " XP)";
        drawAt(kSheetMarginX, headY, classLine.str(), kSheetBodyColor, kSheetHeaderCharSize);
        drawAt(kSheetMarginX, headY, character::alignmentName(c.alignment), kSheetBodyColor, kSheetHeaderCharSize);
        if (c.knightOrder != character::KnightOrder::None) {
            drawAt(kSheetMarginX, headY, character::knightOrderName(c.knightOrder), kSheetBodyColor,
                   kSheetHeaderCharSize);
        }
        if (c.charClass == character::ClassId::Mage) {
            if (c.robeColor == character::RobeColor::None) {
                drawAt(kSheetMarginX, headY, "Unaffiliated student of the arcane", kSheetBodyColor,
                       kSheetHeaderCharSize);
            } else {
                std::ostringstream robeLine;
                robeLine << character::robeColorName(c.robeColor) << ", sworn to "
                         << character::robeMoonName(c.robeColor);
                drawAt(kSheetMarginX, headY, robeLine.str(), kSheetBodyColor, kSheetHeaderCharSize);
            }
        }

        sf::RectangleShape divider(sf::Vector2f(static_cast<float>(windowW) - 2.f * kSheetMarginX, 2.f));
        divider.setFillColor(sf::Color(70, 70, 85));
        divider.setPosition(sf::Vector2f(kSheetMarginX, headY + 6.f));
        window.draw(divider);

        float leftY = headY + 24.f;
        float rightY = headY + 24.f;

        // -- Left column: abilities, HP/AC/THAC0 (with a real pixel-space HP
        // bar -- the one piece of this screen the console version couldn't
        // do), weapon/armor.
        drawAt(kSheetMarginX, leftY, "Abilities:", kSheetSectionColor, kSheetHeaderCharSize);
        std::ostringstream abilities1;
        abilities1 << "STR " << c.scores.strength;
        if (c.exceptionalStrengthPercentile > 0) {
            // 18/01-18/99 zero-pad to two digits; 18/00 is the top bracket,
            // conventionally written "00" rather than "100" -- same
            // formatting as the console version.
            int pct = c.exceptionalStrengthPercentile;
            abilities1 << "/" << (pct == 100 ? "00" : (pct < 10 ? "0" : "")) << (pct == 100 ? "" : std::to_string(pct));
        }
        abilities1 << "   DEX " << c.scores.dexterity << "   CON " << c.scores.constitution;
        drawAt(kSheetMarginX, leftY, abilities1.str(), kSheetBodyColor, kSheetBodyCharSize);
        std::ostringstream abilities2;
        abilities2 << "INT " << c.scores.intelligence << "   WIS " << c.scores.wisdom << "   CHA "
                   << c.scores.charisma;
        drawAt(kSheetMarginX, leftY, abilities2.str(), kSheetBodyColor, kSheetBodyCharSize);
        leftY += 14.f;

        drawAt(kSheetMarginX, leftY, "Combat:", kSheetSectionColor, kSheetHeaderCharSize);
        std::ostringstream hpLine;
        hpLine << "HP " << c.currentHp << "/" << c.maxHp << "   AC " << c.armorClass << "   THAC0 " << c.thac0;
        drawAt(kSheetMarginX, leftY, hpLine.str(), kSheetBodyColor, kSheetBodyCharSize);

        constexpr float kHpBarWidth = 200.f;
        constexpr float kHpBarHeight = 14.f;
        const float hpFrac = c.maxHp > 0
                                  ? std::clamp(static_cast<float>(c.currentHp) / static_cast<float>(c.maxHp), 0.f, 1.f)
                                  : 0.f;
        sf::RectangleShape hpBarBg(sf::Vector2f(kHpBarWidth, kHpBarHeight));
        hpBarBg.setFillColor(sf::Color(60, 30, 30));
        hpBarBg.setPosition(sf::Vector2f(kSheetMarginX, leftY));
        window.draw(hpBarBg);
        sf::RectangleShape hpBarFill(sf::Vector2f(kHpBarWidth * hpFrac, kHpBarHeight));
        hpBarFill.setFillColor(sf::Color(190, 60, 60));
        hpBarFill.setPosition(sf::Vector2f(kSheetMarginX, leftY));
        window.draw(hpBarFill);
        leftY += kHpBarHeight + 10.f;

        std::ostringstream weaponLine;
        weaponLine << "Weapon: " << c.weaponName;
        if (c.specializedWeapon) weaponLine << " (specialized)";
        drawAt(kSheetMarginX, leftY, weaponLine.str(), kSheetBodyColor, kSheetBodyCharSize);
        if (c.equippedArmor != character::ArmorId::None || c.hasShield) {
            std::ostringstream armorLine;
            armorLine << "Armor: ";
            if (c.equippedArmor != character::ArmorId::None) {
                armorLine << character::armorInfo(c.equippedArmor).name;
                if (c.hasShield) armorLine << " + Shield";
            } else {
                armorLine << "Shield only";
            }
            drawAt(kSheetMarginX, leftY, armorLine.str(), kSheetBodyColor, kSheetBodyCharSize);
        }

        // -- Right column: saving throws, steel/inventory, spells-memorized
        // summary, companions.
        drawAt(kSheetRightX, rightY, "Saving Throws:", kSheetSectionColor, kSheetHeaderCharSize);
        for (int i = 0; i < static_cast<int>(character::SaveCategory::Count); ++i) {
            auto category = static_cast<character::SaveCategory>(i);
            std::ostringstream saveLine;
            saveLine << character::saveCategoryName(category) << ": " << c.saves.at(category);
            drawAt(kSheetRightX, rightY, saveLine.str(), kSheetBodyColor, kSheetBodyCharSize);
        }
        rightY += 14.f;

        drawAt(kSheetRightX, rightY, "Steel: " + std::to_string(c.steelPieces) + " stl", kSheetBodyColor,
               kSheetBodyCharSize);
        std::ostringstream carriedLine;
        carriedLine << "Carried: ";
        if (c.inventory.empty()) {
            carriedLine << "nothing";
        } else {
            for (size_t i = 0; i < c.inventory.size(); ++i) {
                if (i > 0) carriedLine << ", ";
                carriedLine << character::inventoryItemLabel(c.inventory[i]);
            }
        }
        for (const std::string& wrapped : wrapToPixelWidth(font, kSheetBodyCharSize, carriedLine.str(), kSheetMaxWidthPx)) {
            drawAt(kSheetRightX, rightY, wrapped, kSheetBodyColor, kSheetBodyCharSize);
        }
        rightY += 14.f;

        if (character::canCastSpells(c.charClass)) {
            std::string spellsLine;
            if (character::maxAccessibleSpellLevel(c) == 0) {
                spellsLine = "Spells: cannot cast arcane magic";
            } else if (c.memorizedSpellIds.empty()) {
                spellsLine = "Spells: none memorized -- rest to prepare";
            } else {
                std::vector<std::string> distinctIds;
                for (const auto& id : c.memorizedSpellIds) {
                    if (std::find(distinctIds.begin(), distinctIds.end(), id) == distinctIds.end()) {
                        distinctIds.push_back(id);
                    }
                }
                std::ostringstream spellLine;
                spellLine << "Spells memorized: ";
                for (size_t i = 0; i < distinctIds.size(); ++i) {
                    if (i > 0) spellLine << ", ";
                    const character::SpellInfo* spell = character::findSpell(c.charClass, distinctIds[i]);
                    int count = static_cast<int>(
                        std::count(c.memorizedSpellIds.begin(), c.memorizedSpellIds.end(), distinctIds[i]));
                    spellLine << (spell != nullptr ? spell->name : distinctIds[i]);
                    if (count > 1) spellLine << " (x" << count << ")";
                }
                spellsLine = spellLine.str();
            }
            for (const std::string& wrapped : wrapToPixelWidth(font, kSheetBodyCharSize, spellsLine, kSheetMaxWidthPx)) {
                drawAt(kSheetRightX, rightY, wrapped, kSheetBodyColor, kSheetBodyCharSize);
            }
            rightY += 14.f;
        }

        if (!state.companions.empty()) {
            drawAt(kSheetRightX, rightY, "Companions:", kSheetSectionColor, kSheetHeaderCharSize);
            for (const game::RecruitedCompanion& rc : state.companions) {
                const character::Character& companion = rc.character;
                const auto& compRace = character::raceInfo(companion.race);
                const character::SubraceInfo* compSub = character::subraceInfo(companion.subrace);
                std::ostringstream compLine1;
                compLine1 << companion.name << ", " << (compSub != nullptr ? compSub->name : compRace.name) << " "
                           << character::classInfo(companion.charClass).name << ", level " << companion.level;
                drawAt(kSheetRightX, rightY, compLine1.str(), kSheetBodyColor, kSheetBodyCharSize);
                std::ostringstream compLine2;
                compLine2 << "HP " << companion.currentHp << "/" << companion.maxHp << "   AC "
                           << companion.armorClass << "   THAC0 " << companion.thac0;
                drawAt(kSheetRightX, rightY, compLine2.str(), kSheetBodyColor, kSheetBodyCharSize);
            }
        }

        const std::string sheetFooterText = character::canCastSpells(c.charClass)
                                                 ? "(s=view spells known, any other key to continue)"
                                                 : "(press any key to continue)";
        sf::Text footer(font, sheetFooterText, kSheetHeaderCharSize);
        footer.setFillColor(sf::Color(150, 150, 160));
        footer.setPosition(sf::Vector2f(kSheetMarginX, static_cast<float>(windowH) - 40.f));
        window.draw(footer);
    };

    // Generic picker overlay -- pixel-space equivalent of
    // render::MapRenderer::drawPickerFrame (MapRenderer.cpp:1149), the
    // console build's single most-reused screen primitive (Talk-to-whom,
    // topic menus, Look-at-whom, quest offer/accept, boat departure,
    // companion recruit, spell-memorization keep-loadout -- see
    // GameLoop.cpp's own callers). Now declared right after font load (see
    // the comment there) instead of here, so the save-slot menu and
    // character creation wizard -- which run before this point, before
    // game::GameState even exists -- can reuse it too. Every caller below
    // (dialogue's PickingCandidate/TopicPicker, Shop, Inventory, Spellbook,
    // Help/Log/World Map/Journal, the quit-confirm dialog) is unaffected by
    // the move. Deliberately NOT used by combat's PickingTarget -- that
    // picker's cursor is drawn embedded in the roster panel, a structurally
    // different visual shape from this full-window overlay. Its optional
    // trailing `message` param (default "") is drawn between the item list
    // and the footer -- added for Shop's post-transaction feedback.

    // Help ('/', this build's bind for the console's '?') -- pixel-space
    // equivalent of render::MapRenderer::drawHelpFrame
    // (MapRenderer.cpp:1474-1500), ported verbatim: static content, no
    // game-state dependency at all. One wording change from the console
    // text: "/ = this help screen" instead of "? = this help screen",
    // matching the real key this build binds (see the switch's own
    // sf::Keyboard::Key::Slash case above).
    auto drawHelpOverlay = [&]() {
        static const std::vector<std::string> kHelpLines = {
            "Movement:",
            "  Arrow keys = move    numpad = move + diagonals",
            "",
            "Overworld / zone:",
            "  l = look around        t = talk to someone here",
            "  Enter = step in/out     c = character sheet",
            "  p = shop (at a shop)    i = inventory / equip",
            "  v = full event log      g = quest journal",
            "  r = rest                z = bed rest (at a bed)",
            "  o = world map           / = this help screen",
            "",
            "Combat:",
            "  Arrow keys = move (spend movement)",
            "  a = aim (free look; Enter attacks a legal target)",
            "  c = cast (if a caster)         u = use an item",
            "  d / Space = end turn           f = flee",
            "  v = view any unit's stats",
            "",
            "q / Esc = quit (asks to confirm) or leave the current screen",
        };
        drawPickerOverlay("Help", kHelpLines, -1, "(press any key to continue)");
    };

    // Journal ('g') -- pixel-space equivalent of GameLoop::showJournal
    // (GameLoop.cpp:1542-1568), flattened into drawPickerOverlay's single
    // "items" list (one quest title line, optionally a "ready to turn in"
    // line, then one [x]/[ ] line per objective, then a blank separator)
    // rather than render::MapRenderer::JournalEntry's grouped structure,
    // which doesn't exist on this side.
    auto drawJournalOverlay = [&]() {
        std::vector<std::string> lines;
        for (const auto& [questId, status] : state.quests) {
            const quest::Quest* q = quests.find(questId);
            if (q == nullptr) continue; // defensive -- startup already cross-validated every zone QUEST id
            const bool complete = status == game::QuestStatus::Complete;
            lines.push_back(q->name + (complete ? " (complete)" : ""));
            if (status == game::QuestStatus::ReadyToTurnIn) {
                lines.push_back("  Ready to turn in! Return to " + q->giver + ".");
            }
            for (const auto& objective : q->objectives) {
                const bool met =
                    complete || status == game::QuestStatus::ReadyToTurnIn || objectiveMet(objective, state);
                std::ostringstream line;
                line << "  " << (met ? "[x] " : "[ ] ") << objective.label;
                if (objective.kind == quest::ObjectiveKind::Slay && !met) {
                    line << " (" << objectiveProgress(objective, state) << "/" << objective.count << ")";
                }
                lines.push_back(line.str());
            }
            lines.push_back("");
        }
        if (!lines.empty() && lines.back().empty()) lines.pop_back(); // no trailing blank separator
        if (lines.empty()) lines.push_back("No quests yet.");
        drawPickerOverlay("Journal", lines, -1, "(press any key to continue)");
    };

    // Quit confirmation -- reuses drawPickerOverlay the same way Help/
    // Journal/Shop/Inventory/Spellbook already do, rather than a new
    // visual primitive. up/down moves quitConfirmSelected, Enter acts on
    // it, Escape/Q cancels the dialog itself -- see the key-dispatch
    // block below.
    auto drawQuitConfirmOverlay = [&]() {
        static const std::vector<std::string> kQuitOptions = {
            "Yes, end my adventure",
            "No, keep playing",
        };
        drawPickerOverlay("Are you sure you want to end your adventure?", kQuitOptions,
                           quitConfirmSelected, "up/down=select   Enter=confirm   Escape=cancel");
    };

    // Rest ('r') / Bed Rest ('z') spell-loadout wizard -- reuses
    // drawPickerOverlay for both of its two possible screens, same as every
    // other list-shaped overlay here. No cancel hint in the footer -- see
    // RestSession's own doc comment for why Escape/Q are swallowed instead
    // of bound to anything while this is open.
    auto drawRestOverlay = [&]() {
        if (restSession.keepSamePrompt) {
            static const std::vector<std::string> kKeepSameOptions = {"Yes", "No -- choose new spells"};
            drawPickerOverlay("Keep the same spells memorized?", kKeepSameOptions,
                               restSession.keepSameSelected, "up/down=select   Enter=choose");
            return;
        }
        const RestSpellPick& pick = restSession.queue[restSession.queueIndex];
        std::vector<std::string> labels;
        for (const character::SpellInfo* spell : pick.choices) labels.push_back(spell->name);
        std::ostringstream title;
        title << "Level " << pick.level << " spell (" << pick.slotNumber << "/" << pick.totalSlotsAtLevel << ")";
        drawPickerOverlay(title.str(), labels, restSession.pickSelected, "up/down=select   Enter=choose");
    };

    // Cast-which-spell picker (M, 2+ distinct memorized spells) -- reuses
    // drawPickerOverlay the same way every other list-shaped overlay here
    // does, composited on top of the ordinary combat frame exactly like
    // drawQuitConfirmOverlay is. Mirrors GameLoop::runCombat's own blocking
    // "Cast which spell?" chooser (GameLoop.cpp:2906-2928), which draws
    // over drawCombatFrame the same way.
    auto drawCombatSpellPickerOverlay = [&]() {
        drawPickerOverlay("Cast which spell?", combatSession.spellPickLabels, combatSession.spellPickSelected,
                           "up/down=select   Enter=cast   Escape=cancel");
    };

    // Use-which-item picker (I, 2+ usable combat items) -- same shape as
    // drawCombatSpellPickerOverlay above. Only one of PickingSpell/
    // PickingItem is ever active at once (Idle only ever starts one
    // chooser), so the two composite calls below never overlap. Mirrors
    // GameLoop::runCombat's own blocking "Use which item?" chooser
    // (GameLoop.cpp:2955-2977).
    auto drawCombatItemPickerOverlay = [&]() {
        drawPickerOverlay("Use which item?", combatSession.itemPickLabels, combatSession.itemPickSelected,
                           "up/down=select   Enter=use   Escape=cancel");
    };

    // View-who picker ('v', Milestone 186) -- same shape as
    // drawCombatSpellPickerOverlay/drawCombatItemPickerOverlay above.
    auto drawCombatViewPickerOverlay = [&]() {
        std::vector<std::string> labels;
        for (const CombatSession::ViewCandidate& candidate : combatSession.viewCandidates) {
            labels.push_back(combatViewCandidateLabel(candidate));
        }
        drawPickerOverlay("View who?", labels, combatSession.viewSelected,
                           "up/down=select   Enter=view   Escape=cancel");
    };

    // The read-only stat card itself, once a candidate is confirmed --
    // selectedIndex -1 (no cursor), same display-only shape as
    // drawHelpOverlay/drawJournalOverlay.
    auto drawCombatViewCardOverlay = [&]() {
        const CombatSession::ViewCandidate& candidate =
            combatSession.viewCandidates[static_cast<size_t>(combatSession.viewSelected)];
        drawPickerOverlay(combatViewCandidateLabel(candidate), combatSession.viewCardLines, -1,
                           "(press any key to continue)");
    };

    // Full event log ('v') -- pixel-space equivalent of
    // render::MapRenderer::drawLogFrame (MapRenderer.cpp:1220-1256). Wraps
    // and scroll-windows log (the same vector the sidebar's own "-- Log --"
    // section draws in full every frame, unclipped) rather than a flat
    // always-visible list, since a long session's log can run into the
    // hundreds of wrapped lines (see GameLoop::handleLog's own comment,
    // GameLoop.cpp:1678-1680) -- then delegates to drawPickerOverlay above
    // for the actual title/list/status/footer rendering, same as every
    // other picker-shaped screen in this file.
    auto drawLogOverlay = [&]() {
        const float maxLineWidthPx = static_cast<float>(windowW) - 2.f * kSheetMarginX;
        std::vector<std::string> wrapped;
        for (const std::string& entry : log) {
            for (std::string& line : wrapToPixelWidth(font, kSheetBodyCharSize, entry, maxLineWidthPx)) {
                wrapped.push_back(std::move(line));
            }
        }
        const int total = static_cast<int>(wrapped.size());
        // Sized to comfortably fit this build's original 1280x800 design
        // size alongside drawPickerOverlay's own title/status/footer lines
        // -- since launching maximized (windowW/windowH reflect the real,
        // larger runtime size) this under-fills the available height rather
        // than overflowing it, which is the safe direction to be wrong in;
        // a live-measured row count is a reasonable follow-up, not done
        // here.
        constexpr int kLogVisibleRows = 20;
        const int maxOffset = std::max(0, total - kLogVisibleRows);
        const int offset =
            logSession.scrollOffset < 0 ? maxOffset : std::clamp(logSession.scrollOffset, 0, maxOffset);
        logSession.scrollOffset = offset; // resolve the -1 sentinel for next frame's own scroll math

        std::vector<std::string> rows;
        for (int i = offset; i < std::min(total, offset + kLogVisibleRows); ++i) {
            rows.push_back(wrapped[static_cast<size_t>(i)]);
        }
        const std::string status =
            total == 0 ? "(nothing logged yet)"
                       : "Lines " + std::to_string(offset + 1) + "-" +
                             std::to_string(std::min(total, offset + kLogVisibleRows)) + " of " +
                             std::to_string(total);
        drawPickerOverlay("Event Log", rows, -1, "up/down=scroll   v/q=return", status);
    };

    // Look ('l') -- pixel-space equivalent of GameLoop::pickAndLook
    // (GameLoop.cpp:937-969), delegating to drawPickerOverlay same as every
    // other picker-shaped screen here. showingDetail reuses the info-screen
    // idiom (selectedIndex -1, a single-item list) to show one candidate's
    // name/description, matching pickAndLook's drawDialogueFrame call; the
    // picker step (2+ candidates, not yet chosen) lists every candidate's
    // name instead.
    auto drawLookOverlay = [&]() {
        if (lookSession.showingDetail) {
            const LookCandidate& c = lookSession.candidates[static_cast<size_t>(lookSession.selected)];
            drawPickerOverlay(c.name, {c.description}, -1, "(press any key to continue)");
            return;
        }
        std::vector<std::string> names;
        for (const LookCandidate& c : lookSession.candidates) names.push_back(c.name);
        drawPickerOverlay("Look at whom?", names, lookSession.selected, "up/down=select   Enter=look   q=cancel");
    };

    // World Map ('o') -- unlike the console's render::MapRenderer::
    // drawWorldMapFrame (MapRenderer.cpp:1258-1435, ASCII box-majority-vote
    // downsampling), this draws the real dragonlancemap2.png scaled down to
    // fit, since mapTexture and the real grid-to-pixel scale
    // (pxPerTileX/pxPerTileY) the live overworld itself uses are already
    // loaded -- higher fidelity than the console version and far less code.
    // Side legend (every location, alphabetically) for the same reason
    // drawWorldMapFrame's own comment gives (MapRenderer.cpp:1396-1402):
    // too many locations sit close together at this resolution for inline
    // labels drawn directly on the map to stay legible.
    auto drawWorldMapOverlay = [&]() {
        drawPanelChrome();

        sf::Text title(font, "World Map", kSheetTitleCharSize);
        title.setFillColor(kSheetSectionColor);
        title.setPosition(sf::Vector2f(kSheetMarginX, 30.f));
        window.draw(title);

        constexpr float kLegendWidth = 260.f;
        constexpr float kTopMargin = 90.f;
        constexpr float kBottomMargin = 50.f;
        const float mapBoxW = static_cast<float>(windowW) - 2.f * kSheetMarginX - kLegendWidth - 20.f;
        const float mapBoxH = static_cast<float>(windowH) - kTopMargin - kBottomMargin;

        const float scale =
            std::min(mapBoxW / static_cast<float>(mapSize.x), mapBoxH / static_cast<float>(mapSize.y));
        const float mapDrawW = static_cast<float>(mapSize.x) * scale;
        const float mapDrawH = static_cast<float>(mapSize.y) * scale;
        const float mapOriginX = kSheetMarginX + (mapBoxW - mapDrawW) / 2.f;
        const float mapOriginY = kTopMargin + (mapBoxH - mapDrawH) / 2.f;

        sf::Sprite miniMap(mapTexture);
        miniMap.setScale(sf::Vector2f(scale, scale));
        miniMap.setPosition(sf::Vector2f(mapOriginX, mapOriginY));
        window.draw(miniMap);

        std::vector<const world::Location*> byName;
        for (const world::Location& loc : world.allLocations()) byName.push_back(&loc);
        std::sort(byName.begin(), byName.end(),
                  [](const world::Location* a, const world::Location* b) { return a->name < b->name; });

        // A distinct hue per location (evenly spaced around the color
        // wheel, keyed by loc.id so the marker loop and the legend loop
        // below agree) -- found live: the old green-town/red-other scheme
        // left same-colored dots indistinguishable wherever several
        // locations cluster close together on the map, with no way to tell
        // which legend entry a given dot was. Fixed saturation/value stay
        // legible against the dark background without clipping to white
        // (which would collide with the player's own marker below).
        auto hueToColor = [](float hue) -> sf::Color {
            const float h = hue * 6.f;
            const float s = 0.65f, v = 0.95f;
            const int i = static_cast<int>(h);
            const float f = h - static_cast<float>(i);
            const float p = v * (1.f - s);
            const float q = v * (1.f - s * f);
            const float t = v * (1.f - s * (1.f - f));
            float r, g, b;
            switch (i % 6) {
                case 0: r = v; g = t; b = p; break;
                case 1: r = q; g = v; b = p; break;
                case 2: r = p; g = v; b = t; break;
                case 3: r = p; g = q; b = v; break;
                case 4: r = t; g = p; b = v; break;
                default: r = v; g = p; b = q; break;
            }
            return sf::Color(static_cast<std::uint8_t>(r * 255.f), static_cast<std::uint8_t>(g * 255.f),
                              static_cast<std::uint8_t>(b * 255.f));
        };
        std::unordered_map<std::string, sf::Color> colorById;
        for (std::size_t i = 0; i < byName.size(); ++i) {
            colorById[byName[i]->id] = hueToColor(static_cast<float>(i) / static_cast<float>(byName.size()));
        }

        // Towns still get a slightly larger marker than everything else --
        // the one piece of the old two-tone scheme worth keeping, since
        // town/not-town is still useful at a glance and doesn't need its
        // own color now that hue is spoken for identifying which location.
        sf::CircleShape marker(3.f);
        for (const world::Location& loc : world.allLocations()) {
            const float px = mapOriginX + (static_cast<float>(loc.x) + 0.5f) * pxPerTileX * scale;
            const float py = mapOriginY + (static_cast<float>(loc.y) + 0.5f) * pxPerTileY * scale;
            const float radius = loc.isTown ? 4.f : 3.f;
            marker.setRadius(radius);
            marker.setOrigin(sf::Vector2f(radius, radius));
            marker.setFillColor(colorById.at(loc.id));
            marker.setPosition(sf::Vector2f(px, py));
            window.draw(marker);
        }
        {
            const float px = mapOriginX + (static_cast<float>(state.x) + 0.5f) * pxPerTileX * scale;
            const float py = mapOriginY + (static_cast<float>(state.y) + 0.5f) * pxPerTileY * scale;
            sf::CircleShape playerDot(4.f);
            playerDot.setOrigin(sf::Vector2f(4.f, 4.f));
            playerDot.setFillColor(sf::Color::White);
            playerDot.setPosition(sf::Vector2f(px, py));
            window.draw(playerDot);
        }

        const float legendX = kSheetMarginX + mapBoxW + 20.f;
        float legendY = kTopMargin;
        constexpr unsigned kLegendCharSize = 13;
        for (const world::Location* loc : byName) {
            sf::CircleShape bullet(3.f);
            bullet.setOrigin(sf::Vector2f(3.f, 3.f));
            bullet.setFillColor(colorById.at(loc->id));
            bullet.setPosition(sf::Vector2f(legendX + 4.f, legendY + 8.f));
            window.draw(bullet);

            sf::Text label(font, loc->name, kLegendCharSize);
            label.setFillColor(kSheetBodyColor);
            label.setPosition(sf::Vector2f(legendX + 14.f, legendY));
            window.draw(label);
            legendY += static_cast<float>(kLegendCharSize) + 8.f;
        }

        sf::Text footer(font, "(press any key to close)", kSheetHeaderCharSize);
        footer.setFillColor(sf::Color(150, 150, 160));
        footer.setPosition(sf::Vector2f(kSheetMarginX, static_cast<float>(windowH) - 36.f));
        window.draw(footer);
    };

    // Zone landmark plate -- shown full-window on entry to a zone that has
    // one (see loadZonePlateTexture above). Cover-fit-and-crop (Milestone
    // 209), same math drawTownMenuOverlay's own plate image uses (Milestone
    // 207) -- fills the whole box, cropping the centered excess off
    // whichever axis overflows, rather than the contain-fit-and-letterbox
    // this shipped with originally. Switched once the user's own real plate
    // art turned out to be uniformly banner-shaped (2508x627, same as
    // Solace's) even for ordinary non-TOWN_MENU zones -- contain-fit left
    // large empty bands above/below at that aspect ratio.
    auto drawZonePlateOverlay = [&]() {
        drawPanelChrome();

        constexpr float kTopMargin = 40.f;
        constexpr float kCaptionAreaH = 90.f;
        const float boxW = static_cast<float>(windowW) - 2.f * kSheetMarginX;
        const float boxH = static_cast<float>(windowH) - kTopMargin - kCaptionAreaH;

        const sf::Vector2u plateSize = zonePlateTexture.getSize();
        if (plateSize.x > 0 && plateSize.y > 0) {
            const float imgW = static_cast<float>(plateSize.x);
            const float imgH = static_cast<float>(plateSize.y);
            const float scale = std::max(boxW / imgW, boxH / imgH);
            const float visibleSrcW = boxW / scale;
            const float visibleSrcH = boxH / scale;
            const int cropX = static_cast<int>(std::round((imgW - visibleSrcW) / 2.f));
            const int cropY = static_cast<int>(std::round((imgH - visibleSrcH) / 2.f));
            sf::Sprite plateSprite(zonePlateTexture);
            plateSprite.setTextureRect(
                sf::IntRect({cropX, cropY}, {static_cast<int>(std::round(visibleSrcW)),
                                              static_cast<int>(std::round(visibleSrcH))}));
            plateSprite.setScale(sf::Vector2f(scale, scale));
            plateSprite.setPosition(sf::Vector2f(kSheetMarginX, kTopMargin));
            window.draw(plateSprite);
        }

        sf::Text title(font, zonePlateZoneName, kSheetTitleCharSize);
        title.setFillColor(kSheetSectionColor);
        const float titleY = kTopMargin + boxH + 12.f;
        title.setPosition(sf::Vector2f(kSheetMarginX, titleY));
        window.draw(title);

        sf::Text footer(font, "(press any key to continue)", kSheetHeaderCharSize);
        footer.setFillColor(sf::Color(150, 150, 160));
        footer.setPosition(sf::Vector2f(kSheetMarginX, static_cast<float>(windowH) - 36.f));
        window.draw(footer);
    };

    // Gold-Box town menu (Milestone 206, docs/ZONE_NOTES.md's "Town
    // menus") -- unlike drawZonePlateOverlay above, this never dismisses
    // on its own: it's the base scene for a TOWN_MENU zone for as long as
    // the player is in it, drawn every frame (see the render-loop call
    // site) rather than toggled by a bool. The plate texture (if any)
    // stays visible the whole time -- confirmed with the user up front,
    // matching how the real Gold Box games kept the town picture on
    // screen while its own letter menu sat underneath it. currentZone is
    // guaranteed non-null by the isEffectiveMenuTown() check at every call
    // site (Milestone 210 -- also true for a zone that's menu-town purely
    // because it has art, not just an explicit TOWN_MENU flag).
    //
    // Layout, top to bottom: title, then the image filling whatever
    // vertical space is left over once the letter list (anchored to the
    // bottom, above the footer) has claimed its own space -- rather than
    // a fixed-height image band, so the image actually fills the window
    // instead of leaving a large dead gap above a short menu (found live
    // 2026-09-17, screenshotted by the user).
    auto drawTownMenuOverlay = [&]() {
        drawPanelChrome();

        sf::Text title(font, currentZone->name(), kSheetTitleCharSize);
        title.setFillColor(kSheetSectionColor);
        title.setPosition(sf::Vector2f(kSheetMarginX, 30.f));
        window.draw(title);

        const std::vector<TownMenuItem> items = buildTownMenuItems(*currentZone, townMenuLeaveLabel());
        const float lineHeight = static_cast<float>(kSheetBodyCharSize) + 10.f;
        const float listHeight = static_cast<float>(items.size()) * lineHeight;
        const float footerY = static_cast<float>(windowH) - 36.f;
        constexpr float kListFooterGap = 20.f;
        const float listStartY = footerY - kListFooterGap - listHeight;

        constexpr float kImageTopY = 90.f;
        constexpr float kImageListGap = 20.f;
        const float imageAreaH = std::max(0.f, listStartY - kImageListGap - kImageTopY);
        if (zonePlateLoaded && imageAreaH > 0.f) {
            const float boxW = static_cast<float>(windowW) - 2.f * kSheetMarginX;
            const sf::Vector2u plateSize = zonePlateTexture.getSize();
            if (plateSize.x > 0 && plateSize.y > 0) {
                // Cover-fit (fills the whole image area, cropping any
                // overflow off the centered excess axis) rather than
                // contain-fit -- the box and the source image are rarely
                // the exact same aspect ratio, and letterboxing read as
                // "too small" (found live, user feedback 2026-09-17).
                const float imgW = static_cast<float>(plateSize.x);
                const float imgH = static_cast<float>(plateSize.y);
                const float scale = std::max(boxW / imgW, imageAreaH / imgH);
                const float visibleSrcW = boxW / scale;
                const float visibleSrcH = imageAreaH / scale;
                const int cropX = static_cast<int>(std::round((imgW - visibleSrcW) / 2.f));
                const int cropY = static_cast<int>(std::round((imgH - visibleSrcH) / 2.f));
                sf::Sprite plateSprite(zonePlateTexture);
                plateSprite.setTextureRect(
                    sf::IntRect({cropX, cropY}, {static_cast<int>(std::round(visibleSrcW)),
                                                  static_cast<int>(std::round(visibleSrcH))}));
                plateSprite.setScale(sf::Vector2f(scale, scale));
                plateSprite.setPosition(sf::Vector2f(kSheetMarginX, kImageTopY));
                window.draw(plateSprite);
            }
        }

        float listY = listStartY;
        for (const TownMenuItem& item : items) {
            sf::Text line(font, "[" + std::string(1, item.hotkeyChar) + "] " + item.label, kSheetBodyCharSize);
            line.setFillColor(kSheetBodyColor);
            line.setPosition(sf::Vector2f(kSheetMarginX, listY));
            window.draw(line);
            listY += lineHeight;
        }

        sf::Text footer(font, "(press a letter)", kSheetHeaderCharSize);
        footer.setFillColor(sf::Color(150, 150, 160));
        footer.setPosition(sf::Vector2f(kSheetMarginX, footerY));
        window.draw(footer);
    };

    // Spellbook ('s' from the character sheet, casters only) -- the
    // sheet's own full-spellbook drill-down, pixel-space equivalent of
    // render::MapRenderer::drawSpellbookFrame (MapRenderer.cpp:842-885).
    // Purely informational (no selection at all, unlike every other
    // drawPickerOverlay caller), so it always passes selectedIndex -1 --
    // the same "no cursor, nothing to select" idiom drawInventoryOverlay
    // already uses for its own leading header rows.
    auto drawSpellbookOverlay = [&]() {
        const character::Character& c = state.character;
        std::vector<std::string> rows;
        const int maxLevel = character::maxAccessibleSpellLevel(c);
        if (maxLevel == 0) {
            rows.push_back("Cannot cast arcane magic.");
        } else {
            for (int lvl = 1; lvl <= maxLevel; ++lvl) {
                std::vector<const character::SpellInfo*> atLevel;
                for (const auto& spell : character::spellListFor(c.charClass)) {
                    if (spell.level == lvl) atLevel.push_back(&spell);
                }
                if (atLevel.empty()) continue;
                const int slots = character::spellSlotsPerDay(c, lvl);
                std::ostringstream header;
                header << "Level " << lvl << " (" << slots << " slot" << (slots == 1 ? "" : "s") << "/day):";
                rows.push_back(header.str());
                for (const auto* spell : atLevel) {
                    std::ostringstream line;
                    line << "  " << spell->name;
                    const int count = static_cast<int>(
                        std::count(c.memorizedSpellIds.begin(), c.memorizedSpellIds.end(), spell->id));
                    if (count > 0) {
                        line << " (memorized";
                        if (count > 1) line << " x" << count;
                        line << ")";
                    }
                    rows.push_back(line.str());
                }
            }
        }
        // Scroll-windowed the same way drawLogOverlay is -- see
        // spellbookScrollOffset's own declaration comment for why this
        // build needs it where the console original didn't.
        const int total = static_cast<int>(rows.size());
        constexpr int kSpellbookVisibleRows = 20;
        const int maxOffset = std::max(0, total - kSpellbookVisibleRows);
        const int offset = std::clamp(spellbookScrollOffset, 0, maxOffset);
        spellbookScrollOffset = offset;
        std::vector<std::string> visible;
        for (int i = offset; i < std::min(total, offset + kSpellbookVisibleRows); ++i) {
            visible.push_back(rows[static_cast<size_t>(i)]);
        }
        const std::string status = total <= kSpellbookVisibleRows
                                        ? ""
                                        : "Lines " + std::to_string(offset + 1) + "-" +
                                              std::to_string(std::min(total, offset + kSpellbookVisibleRows)) +
                                              " of " + std::to_string(total);
        drawPickerOverlay("Spells Known", visible, -1, "up/down=scroll   any other key=return", status);
    };

    // Dialogue -- a full-window overlay, same compositing approach as
    // drawCharacterSheetOverlay above (drawn as a final layer on top of
    // the map/sidebar, gated on dialogueSession.active). Pixel-space
    // equivalent of render::MapRenderer::drawDialogueFrame, reusing the
    // character sheet's own color/size constants above for visual
    // consistency across overlays. PickingCandidate/TopicPicker below
    // delegate to drawPickerOverlay above instead of rendering their own
    // list.
    auto drawDialogueOverlay = [&]() {
        // Dialogue portrait (Milestone 208, redesigned Milestone 210) --
        // only for the states below that render text directly in this
        // lambda (not the picker-delegated ones further down, which share
        // drawPickerOverlay with non-dialogue screens -- see
        // docs/ARCHITECTURE.md's SFML section). Full panel width now (no
        // more narrowing beside a corner box) -- the image moved above the
        // text instead.
        const float maxWidthPx = static_cast<float>(windowW) - 2.f * kSheetMarginX;
        float y = 40.f;

        auto drawLine = [&](const std::string& text, sf::Color color, unsigned size) {
            sf::Text sfText(font, text, size);
            sfText.setFillColor(color);
            sfText.setPosition(sf::Vector2f(kSheetMarginX, y));
            window.draw(sfText);
            y += static_cast<float>(size) + 10.f;
        };

        // Contain-fit (Milestone 210, replacing the small 180x180 cover-fit
        // corner box) -- deliberately the *other* fit strategy from every
        // other image in this codebase (zone plates, town menu, the old
        // portrait box all crop-to-fill). The user's actual portrait art is
        // portrait-oriented (~4:5) single-character art, not a wide
        // establishing shot -- cropping it to fill an arbitrary box only
        // ever hides more of it as the box grows, never reveals more.
        // Contain-fit always shows the whole portrait, scaled as large as
        // the available space allows.
        //
        // A *fixed* reserved text height (kBodyLinesPerPage worth of
        // lines, plus the footer prompt), not one measured from the
        // current response -- changed at the same time pagination was
        // added (see paginateBodyText/setDialogueBodyText above): the
        // user didn't want the portrait shrinking for a long response
        // (Otik's TALK_AFTER did, under the original adaptive-height
        // version), they wanted it to stay large and consistent every
        // time, with long text paginating underneath it instead. Width is
        // the full panel width. Returns the y to resume drawing at (a
        // no-op passthrough of the current y when no portrait is loaded).
        auto drawPortraitFixed = [&]() -> float {
            if (!dialoguePortraitLoaded) return y;
            const sf::Vector2u texSize = dialoguePortraitTexture.getSize();
            if (texSize.x == 0 || texSize.y == 0) return y;
            constexpr float kPortraitGap = 20.f;
            constexpr float kBottomMargin = 30.f;
            const float reservedTextH =
                static_cast<float>(kBodyLinesPerPage) * (kSheetBodyCharSize + 10.f) + 10.f + kSheetHeaderCharSize + 10.f;
            const float boxH = static_cast<float>(windowH) - y - reservedTextH - kBottomMargin;
            const float imgW = static_cast<float>(texSize.x);
            const float imgH = static_cast<float>(texSize.y);
            const float scale = std::min(maxWidthPx / imgW, boxH / imgH);
            const float drawW = imgW * scale;
            const float drawH = imgH * scale;
            sf::Sprite portraitSprite(dialoguePortraitTexture);
            portraitSprite.setScale(sf::Vector2f(scale, scale));
            portraitSprite.setPosition(
                sf::Vector2f(kSheetMarginX + (maxWidthPx - drawW) / 2.f, y + (boxH - drawH) / 2.f));
            window.draw(portraitSprite);
            return y + boxH + kPortraitGap;
        };

        switch (dialogueSession.uiState) {
            case DialogueUiState::Greeting:
            case DialogueUiState::TopicText:
            case DialogueUiState::AskResponse:
            case DialogueUiState::QuestOfferText:
            case DialogueUiState::QuestAcceptText:
            case DialogueUiState::QuestProgressText:
            case DialogueUiState::QuestCompleteText:
            case DialogueUiState::WayrethIntro: {
                // Chrome drawn per-case (not once at the top of this
                // lambda) so the picker-delegated states below don't
                // double-draw it via their own drawPickerOverlay() call.
                drawPanelChrome();
                // Only Greeting can differ from the NPC's own name (the
                // askLimitLocked override, see dialogueStartTalk) --
                // TopicText/AskResponse always speak as the NPC itself,
                // matching GameLoop::talkTo's own `name` vs `speakerName`
                // split.
                const std::string& speaker = dialogueSession.uiState == DialogueUiState::Greeting
                                                  ? dialogueSession.displaySpeaker
                                                  : dialogueSession.current.name;
                drawLine(speaker, sf::Color::White, kSheetTitleCharSize);
                y += 10.f;
                y = drawPortraitFixed();
                // Only the current page's lines draw -- see
                // paginateBodyText/dialogueContinue above. bodyTextPage is
                // clamped defensively (a shorter response than the page
                // it was left on, e.g. after AskResponse advances to its
                // next queued message, resets via setDialogueBodyText
                // anyway, but this is cheap insurance).
                const std::vector<std::vector<std::string>> pages = paginateBodyText(dialogueSession.bodyText);
                const std::size_t pageIndex =
                    std::min(static_cast<std::size_t>(dialogueSession.bodyTextPage), pages.size() - 1);
                for (const std::string& wrapped : pages[pageIndex]) {
                    drawLine(wrapped, kSheetBodyColor, kSheetBodyCharSize);
                }
                y += 10.f;
                const bool morePages = pageIndex + 1 < pages.size();
                drawLine(morePages ? "(press Enter to see more)" : "(press Enter to continue)",
                         sf::Color(150, 150, 160), kSheetHeaderCharSize);
                break;
            }
            case DialogueUiState::PickingCandidate: {
                std::vector<std::string> names;
                names.reserve(dialogueSession.candidates.size());
                for (const auto& candidate : dialogueSession.candidates) names.push_back(candidate.name);
                drawPickerOverlay("Talk to whom?", names, dialogueSession.candidateSelected,
                                   "(up/down = select, Enter = talk, q = cancel)");
                break;
            }
            case DialogueUiState::TopicPicker:
                drawPickerOverlay("Ask " + dialogueSession.current.name + " about...", dialogueSession.topicLabels,
                                   dialogueSession.topicSelected, "(up/down = select, Enter = ask, q = leave)");
                break;
            case DialogueUiState::BoatOffer: {
                const world::Location* destination = world.getLocation(dialogueSession.current.boatDestinationId);
                const std::string title =
                    "Depart for " + (destination != nullptr ? destination->name : std::string("?")) + "?";
                drawPickerOverlay(title, {"Board", "Not yet"}, dialogueSession.boatOfferSelected,
                                   "(up/down = select, Enter = choose, q = cancel)");
                break;
            }
            case DialogueUiState::RecruitOffer: {
                const std::string title = "Ask " + dialogueSession.current.name + " to join your journey?";
                drawPickerOverlay(title, {"Join me", "Not yet"}, dialogueSession.recruitOfferSelected,
                                   "(up/down = select, Enter = choose, q = cancel)");
                break;
            }
            case DialogueUiState::QuestAcceptDecline: {
                const quest::Quest* q = quests.find(dialogueSession.current.questId);
                const std::string title = q != nullptr ? q->name : "Accept this quest?";
                drawPickerOverlay(title, {"Accept", "Decline"}, dialogueSession.questOfferSelected,
                                   "(up/down = select, Enter = choose, q = cancel)");
                break;
            }
            case DialogueUiState::WayrethChoice: {
                // No "q=cancel" in the footer -- mirrors offerOrTurnInQuest's
                // own picker (GameLoop.cpp:1469): Quit is deliberately
                // ignored here, there's no meaningful cancel once the
                // Conclave is asking (see the key-dispatch wiring below).
                static const std::vector<std::string> kEthicLabels = {
                    "Let it stand, and find another way.",
                    "Take only the narrowest path through.",
                    "Cut it down, and take what's yours.",
                };
                const quest::Quest* q = quests.find(dialogueSession.current.questId);
                const std::string title = q != nullptr ? q->name : "Choose.";
                drawPickerOverlay(title, kEthicLabels, dialogueSession.wayrethChoiceSelected,
                                   "(up/down = select, Enter = choose)");
                break;
            }
            case DialogueUiState::AskInput: {
                drawPanelChrome();
                drawLine("Ask " + dialogueSession.current.name + " about...", kSheetSectionColor,
                         kSheetTitleCharSize);
                y += 10.f;
                y = drawPortraitFixed();
                std::vector<std::string> wrappedHints;
                if (!dialogueSession.askInputHints.empty()) {
                    std::string hintLine = "You could ask about: ";
                    for (std::size_t i = 0; i < dialogueSession.askInputHints.size(); ++i) {
                        if (i > 0) hintLine += ", ";
                        hintLine += dialogueSession.askInputHints[i];
                    }
                    wrappedHints = wrapToPixelWidth(font, kSheetHeaderCharSize, hintLine, maxWidthPx);
                }
                for (const std::string& wrapped : wrappedHints) {
                    drawLine(wrapped, sf::Color(150, 150, 160), kSheetHeaderCharSize);
                }
                if (!wrappedHints.empty()) y += 10.f;
                // Static trailing cursor glyph, no blink -- this project has
                // no animation/timing primitive yet (see
                // docs/CURRENT_WORK.md's "Parked" section) and this box
                // doesn't need one.
                drawLine("> " + dialogueSession.askInputBuffer + "_", sf::Color::White, kSheetBodyCharSize);
                y += 10.f;
                drawLine("(type a subject, Enter to ask -- empty Enter cancels)", sf::Color(150, 150, 160),
                         kSheetHeaderCharSize);
                break;
            }
        }
    };

    // Shop -- pixel-space equivalent of render::MapRenderer::drawShopFrame
    // (MapRenderer.cpp:1052-1103), delegating to drawPickerOverlay above
    // instead of rendering its own list (this screen is exactly the
    // "picker-shaped" case that lambda was extracted for). Recomputes
    // availableShopItems/sellableItems fresh every call rather than caching
    // on ShopSession -- matches those functions' own "compute on demand,
    // don't cache" comment in Equipment.h, since a purchase/sale changes
    // the character's inventory and steel out from under a cached list.
    auto drawShopOverlay = [&]() {
        std::ostringstream title;
        title << shopSession.shopName << (shopSession.sellMode ? " -- Selling   " : " -- Buying   ")
              << "Steel: " << state.character.steelPieces << " stl";
        std::vector<std::string> rows;
        if (shopSession.sellMode) {
            for (const character::SellItem& item : character::sellableItems(state.character)) {
                std::ostringstream row;
                row << item.label << " -- " << item.valueStl << " stl";
                if (!item.sellable) row << "  (cannot sell)";
                rows.push_back(row.str());
            }
        } else {
            for (const character::ShopItem& item : character::availableShopItems(state.character, shopSession.catalog)) {
                std::ostringstream row;
                row << item.label << " -- " << item.costStl << " stl";
                if (item.alreadyOwned) {
                    row << "  (owned)";
                } else if (!item.buyable) {
                    row << "  (cannot use)";
                }
                rows.push_back(row.str());
            }
        }
        std::ostringstream footer;
        footer << "up/down=select   Enter=" << (shopSession.sellMode ? "sell" : "buy")
               << "   i=" << (shopSession.sellMode ? "view buy list" : "view sell list") << "   q=leave";
        drawPickerOverlay(title.str(), rows, shopSession.selected, footer.str(), shopSession.message);
    };

    // Inventory -- pixel-space equivalent of render::MapRenderer::
    // drawInventoryFrame (MapRenderer.cpp:1105-1147), delegating to
    // drawPickerOverlay above the same way Shop does. That lambda only
    // takes a flat item list with no separate header block, so the HP/
    // Weapon/Armor summary lines are prepended as non-selectable leading
    // rows and the cursor index is offset past them -- the cleanest way
    // to reuse it unchanged. cursorIndex is -1 (never matches any row)
    // when the inventory is empty, so "(nothing carried)" never draws a
    // cursor -- there's nothing to select.
    auto drawInventoryOverlay = [&]() {
        std::vector<std::string> rows;
        rows.push_back("HP: " + std::to_string(state.character.currentHp) + "/" +
                        std::to_string(state.character.maxHp));
        rows.push_back("Weapon: " + state.character.weaponName);
        std::string armorLine = "Armor: " + (state.character.equippedArmor == character::ArmorId::None
                                                  ? std::string("none")
                                                  : std::string(character::armorInfo(state.character.equippedArmor).name));
        if (state.character.hasShield) armorLine += " + Shield";
        rows.push_back(armorLine);
        rows.push_back("");
        if (state.character.inventory.empty()) {
            rows.push_back("(nothing carried)");
        } else {
            rows.push_back("Carried items:");
        }
        const int headerCount = static_cast<int>(rows.size());
        for (const character::InventoryItem& item : state.character.inventory) {
            rows.push_back(character::inventoryItemLabel(item));
        }
        const int cursorIndex = state.character.inventory.empty() ? -1 : headerCount + inventorySession.selected;
        drawPickerOverlay("Inventory", rows, cursorIndex, "up/down=select   Enter=equip/use   q=leave",
                           inventorySession.message);
    };

    std::cout << "init ok; world pixel size " << worldW << "x" << worldH << std::endl;

    while (window.isOpen()) {
        try {
            while (const std::optional<sf::Event> event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    // Autosave on an abrupt OS-level close (title-bar X /
                    // Alt+F4) too, same reasoning as the KeyPressed block's
                    // own autosave call below -- capture the same final
                    // state a deliberate quit would.
                    game::SaveGame::save(state, activeSavePath);
                    window.close();
                } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                    const sf::Keyboard::Key key = keyPressed->code;
                    int dx = 0;
                    int dy = 0;
                    std::string placeholder;
                    bool handleEnter = false;
                    bool wantsTalk = false;
                    bool wantsShop = false;
                    bool wantsQuit = false;
                    bool wantsBackspace = false;
                    bool wantsLook = false;
                    bool wantsLog = false;
                    bool wantsJournal = false;
                    bool wantsWorldMap = false;
                    bool wantsHelp = false;
                    bool wantsRest = false;
                    bool wantsBedRest = false;
                    switch (key) {
                        // Milestone 200: WASD removed project-wide (arrow
                        // keys + the numpad scheme below already cover
                        // every direction with redundancy, and freeing up
                        // the letters lets combat bind DQoK's own verbs to
                        // them instead -- see docs/COMBAT_NOTES.md).
                        case sf::Keyboard::Key::Up:
                        case sf::Keyboard::Key::Numpad8: dy = -1; break;
                        case sf::Keyboard::Key::Down:
                        case sf::Keyboard::Key::Numpad2: dy = 1; break;
                        case sf::Keyboard::Key::Left:
                        case sf::Keyboard::Key::Numpad4: dx = -1; break;
                        case sf::Keyboard::Key::Right:
                        case sf::Keyboard::Key::Numpad6: dx = 1; break;
                        // Milestone 187: keypad diagonal movement (numpad,
                        // reintroduced for ansalon_sfml_phase1 only -- see
                        // docs/COMBAT_NOTES.md/docs/MAP_NOTES.md for why
                        // this doesn't reverse Milestone 63's console-only
                        // diagonal removal). Two bindings per direction:
                        // Windows only reports Numpad7/9/1/3 when NumLock
                        // is ON -- with it off, the same physical keys
                        // report as Home/PageUp/End/PageDown instead, none
                        // of which are bound to anything else in this
                        // file, so binding both makes diagonal movement
                        // work regardless of NumLock state.
                        case sf::Keyboard::Key::Numpad7:
                        case sf::Keyboard::Key::Home: dx = -1; dy = -1; break;
                        case sf::Keyboard::Key::Numpad9:
                        case sf::Keyboard::Key::PageUp: dx = 1; dy = -1; break;
                        case sf::Keyboard::Key::Numpad1:
                        case sf::Keyboard::Key::End: dx = -1; dy = 1; break;
                        case sf::Keyboard::Key::Numpad3:
                        case sf::Keyboard::Key::PageDown: dx = 1; dy = 1; break;
                        // Deferred, not closed inline here -- while the ask-input
                        // text box is open, 'q' is an ordinary letter a player may
                        // need to type (see the ask-input guard below), not a quit
                        // key, and Escape cancels that box instead of the window.
                        case sf::Keyboard::Key::Q:
                        case sf::Keyboard::Key::Escape: wantsQuit = true; break;
                        case sf::Keyboard::Key::Backspace: wantsBackspace = true; break;
                        case sf::Keyboard::Key::L: wantsLook = true; break;
                        case sf::Keyboard::Key::T: wantsTalk = true; break;
                        case sf::Keyboard::Key::Enter: handleEnter = true; break;
                        case sf::Keyboard::Key::C: break; // handled explicitly below via sheetOpen
                        case sf::Keyboard::Key::P: wantsShop = true; break;
                        // Handled explicitly below: opens Inventory in the
                        // ordinary case, but toggles Shop's own buy/sell view
                        // while a shop is open (that branch reads `key`
                        // directly, bypassing this switch -- see the
                        // shopSession.active dispatch below).
                        case sf::Keyboard::Key::I: break;
                        case sf::Keyboard::Key::V: wantsLog = true; break;
                        case sf::Keyboard::Key::G: wantsJournal = true; break;
                        case sf::Keyboard::Key::O: wantsWorldMap = true; break;
                        case sf::Keyboard::Key::Slash: wantsHelp = true; break;
                        case sf::Keyboard::Key::F: placeholder = "Flee: not available outside combat."; break;
                        case sf::Keyboard::Key::R: wantsRest = true; break;
                        case sf::Keyboard::Key::Z: wantsBedRest = true; break;
                        default: break;
                    }

                    // Ask-input is the one screen where an ordinary letter
                    // (e.g. 'q', part of a typed question) must not quit the
                    // game, and where Escape means "cancel this box" rather
                    // than "close the window". Every other dialogue state
                    // DOES get a Quit override just below, matching
                    // GameLoop::talkTo/pickAndTalk's own console behavior
                    // exactly: pickAndTalk's "Talk to whom?" picker treats
                    // Quit as cancel (GameLoop.cpp:931-932), talkTo's topic
                    // picker treats it as "leave" -- same as selecting
                    // "Nothing, thanks" (GameLoop.cpp:1301-1302) -- and a
                    // plain greeting/topic-text/ask-response screen's
                    // readKey() doesn't special-case Quit at all, so it just
                    // acts like any other key (continue). Found live:
                    // talking to a canon Hero and pressing Escape/Q closed
                    // the whole application instead of leaving the
                    // conversation, since this build previously had no
                    // per-screen cancel here and fell through to the
                    // general "close the window" branch below.
                    const bool askInputActive =
                        dialogueSession.active && dialogueSession.uiState == DialogueUiState::AskInput;
                    if (quitConfirmOpen) {
                        // Checked first, ahead of every other guard below --
                        // once this is open it owns all input, same "modal
                        // takes total priority" shape askInputActive's own
                        // Escape check has. up/down (already computed as
                        // dy above) moves the selection, Enter acts on it,
                        // and Escape/Q always cancels the dialog itself
                        // rather than falling through to anything else --
                        // there is no path from here back to window.close()
                        // except explicitly picking "Yes".
                        if (dy < 0) {
                            quitConfirmSelected = 0;
                        } else if (dy > 0) {
                            quitConfirmSelected = 1;
                        } else if (handleEnter) {
                            if (quitConfirmSelected == 0) {
                                window.close();
                            } else {
                                quitConfirmOpen = false;
                            }
                        } else if (wantsQuit) {
                            quitConfirmOpen = false;
                        }
                    } else if (askInputActive && key == sf::Keyboard::Key::Escape) {
                        dialogueSession.uiState = DialogueUiState::TopicPicker;
                    } else if (dialogueSession.active && wantsQuit && !askInputActive) {
                        switch (dialogueSession.uiState) {
                            case DialogueUiState::PickingCandidate:
                                dialogueSession.active = false;
                                break;
                            case DialogueUiState::TopicPicker:
                                dialogueEnd();
                                break;
                            case DialogueUiState::BoatOffer:
                                dialogueDeclineBoat(); // q=cancel, same as GameLoop.cpp:1076-1078
                                break;
                            case DialogueUiState::RecruitOffer:
                                dialogueDeclineRecruit(); // q=cancel, same as GameLoop.cpp:1140-1142
                                break;
                            case DialogueUiState::QuestAcceptDecline:
                                // q=cancel -- "declined, re-offerable next
                                // time, nothing recorded", same as
                                // GameLoop.cpp:1345-1346.
                                dialogueAfterGreeting();
                                break;
                            case DialogueUiState::WayrethChoice:
                                // Quit deliberately ignored here, matching
                                // GameLoop.cpp:1481-1482 -- there's no
                                // meaningful "cancel" once the Conclave is
                                // asking.
                                break;
                            case DialogueUiState::Greeting:
                            case DialogueUiState::TopicText:
                            case DialogueUiState::AskResponse:
                            case DialogueUiState::QuestOfferText:
                            case DialogueUiState::QuestAcceptText:
                            case DialogueUiState::QuestProgressText:
                            case DialogueUiState::QuestCompleteText:
                            case DialogueUiState::WayrethIntro:
                                dialogueContinue();
                                break;
                            case DialogueUiState::AskInput:
                                break; // unreachable -- askInputActive excludes this above
                        }
                    } else if (shopSession.active && wantsQuit) {
                        // One deliberate deviation from the "Q always closes the
                        // whole window" rule every other screen here follows --
                        // GameLoop::handleShop's own Key::Quit explicitly "exits
                        // the shop, not the whole game" (GameLoop.cpp:1611), and a
                        // player backing in and out of a shop repeatedly is the
                        // normal case, not an edge case, so faithfully porting the
                        // console's behavior matters more than this build's
                        // otherwise-uniform Q convention here.
                        shopSession.active = false;
                    } else if (inventorySession.active && wantsQuit) {
                        // Same deviation as Shop just above -- GameLoop::
                        // handleInventory's own Key::Quit just returns from its
                        // local loop (GameLoop.cpp:1671-1672), closing the
                        // screen, not the whole game.
                        inventorySession.active = false;
                    } else if (spellbookOpen && dy != 0) {
                        // North/South scrolls (see spellbookScrollOffset's
                        // declaration comment) instead of dismissing --
                        // checked ahead of the plain spellbookOpen dismiss
                        // branch just below.
                        spellbookScrollOffset += (dy < 0 ? -1 : 1) * 5;
                    } else if (spellbookOpen) {
                        // Any other key returns to the sheet -- spellbookOpen
                        // is only ever true while sheetOpen is too. Checked
                        // ahead of the general quit branch below for the
                        // same reason sheetOpen's own guard just under this
                        // one is: Q/Escape should return to the sheet, not
                        // close the whole window.
                        spellbookOpen = false;
                    } else if (sheetOpen && dy > 0 && character::canCastSpells(state.character.charClass)) {
                        // 's'/Down opens the spellbook drill-down instead of
                        // dismissing the sheet -- GameLoop::
                        // showCharacterSheet's own South dispatch
                        // (GameLoop.cpp:632-639), offered to casters only.
                        spellbookOpen = true;
                        spellbookScrollOffset = 0;
                    } else if (sheetOpen) {
                        // Dismiss on any other key -- see sheetOpen's own
                        // top comment. Deliberately swallows dx/dy/
                        // handleEnter too, so the same keypress that closes
                        // the sheet never also moves the character or opens
                        // combat. Checked ahead of the general quit branch
                        // below (same placement shop/inventory's own Quit
                        // overrides use above) so Q/Escape dismiss the sheet
                        // instead of closing the whole window -- "dismissed
                        // by any key" includes Quit, matching the console's
                        // showCharacterSheet loop, which has no special
                        // Quit handling of its own either.
                        sheetOpen = false;
                    } else if (helpOpen) {
                        // Any key dismisses -- same "informational, no cancel
                        // key needed" shape as sheetOpen above. Checked ahead
                        // of the general quit branch below so Q/Escape close
                        // this screen, not the whole window.
                        helpOpen = false;
                    } else if (worldMapOpen) {
                        // Same "any key dismisses" shape as helpOpen above.
                        worldMapOpen = false;
                    } else if (zonePlateOpen) {
                        // Same "any key dismisses" shape as helpOpen above.
                        zonePlateOpen = false;
                    } else if (journalOpen) {
                        // Same "any key dismisses" shape as helpOpen above.
                        journalOpen = false;
                    } else if (logSession.active) {
                        // Full log ('v') -- North/South scroll by a fixed
                        // chunk, matching GameLoop::handleLog's own
                        // kLogScrollStep (GameLoop.cpp:1681); 'v' (the key
                        // that opened it) closes it too, same as Quit --
                        // handleLog's own comment notes both close it
                        // (GameLoop.cpp:1687-1690). Checked ahead of the
                        // general quit branch below for the same reason
                        // every other overlay guard above is.
                        constexpr int kLogScrollStep = 10;
                        if (dy < 0) {
                            logSession.scrollOffset = std::max(0, logSession.scrollOffset - kLogScrollStep);
                        } else if (dy > 0) {
                            logSession.scrollOffset += kLogScrollStep; // clamped for real next draw
                        } else if (wantsQuit || key == sf::Keyboard::Key::V) {
                            logSession.active = false;
                        }
                    } else if (lookSession.active) {
                        // Look ('l') -- mirrors GameLoop::pickAndLook: a
                        // single candidate's showingDetail dismisses on any
                        // key (Look never loops back to its own picker,
                        // unlike Talk's pickAndTalk); 2+ candidates get a
                        // North/South/Enter/Quit picker first, same shape as
                        // the Talk candidate picker above.
                        if (lookSession.showingDetail) {
                            lookSession.active = false;
                        } else if (dy < 0) {
                            lookSession.selected =
                                (lookSession.selected - 1 + static_cast<int>(lookSession.candidates.size())) %
                                static_cast<int>(lookSession.candidates.size());
                        } else if (dy > 0) {
                            lookSession.selected =
                                (lookSession.selected + 1) % static_cast<int>(lookSession.candidates.size());
                        } else if (handleEnter) {
                            lookSession.showingDetail = true;
                        } else if (wantsQuit) {
                            lookSession.active = false;
                        }
                    } else if (restSession.active) {
                        // Rest/Bed Rest's own spell-loadout wizard -- see
                        // RestSession's doc comment for why every key but
                        // North/South/Enter (Q/Escape included) is simply
                        // swallowed here rather than treated as a cancel.
                        // Checked ahead of the general quit branch below for
                        // that reason, same as every other overlay guard
                        // above.
                        if (restSession.keepSamePrompt) {
                            if (dy < 0 || dy > 0) {
                                restSession.keepSameSelected = restSession.keepSameSelected == 0 ? 1 : 0;
                            } else if (handleEnter) {
                                restConfirmKeepSame();
                            }
                        } else if (!restSession.queue.empty()) {
                            const int optionCount =
                                static_cast<int>(restSession.queue[restSession.queueIndex].choices.size());
                            if (dy < 0) {
                                restSession.pickSelected = (restSession.pickSelected - 1 + optionCount) % optionCount;
                            } else if (dy > 0) {
                                restSession.pickSelected = (restSession.pickSelected + 1) % optionCount;
                            } else if (handleEnter) {
                                restConfirmSpellPick();
                            }
                        }
                    } else if (combatSession.active &&
                               (combatSession.uiState == CombatUiState::PickingSpell ||
                                combatSession.uiState == CombatUiState::PickingItem ||
                                combatSession.uiState == CombatUiState::ViewPicking ||
                                combatSession.uiState == CombatUiState::PickingTarget ||
                                combatSession.uiState == CombatUiState::Aiming) &&
                               wantsQuit) {
                        // Escape/Q cancels the spell/item/view/target choice
                        // itself, back to Idle, no round consumed -- mirrors
                        // GameLoop::runCombat's own blocking spell-choice and
                        // USE-menu loops, where Quit sets cancelled=true and
                        // the round loop `continue`s without ever reaching
                        // the initiative dispatch (GameLoop.cpp:2923-2928 for
                        // spells, GameLoop.cpp:2973-2978 for items). Checked
                        // ahead of the general quit branch below for the
                        // same reason every other overlay guard above is.
                        // ViewPicking (Milestone 186) never rolled
                        // initiative in the first place (combatBeginView
                        // costs no round), so cancelling it is even lower-
                        // stakes than the spell/item cases this guard
                        // already covered.
                        //
                        // PickingTarget (found live 2026-09-12: this state
                        // had no cancel at all -- Escape/Q fell through to
                        // the top-level quit-confirmation instead, and I/F/
                        // Backspace were silently swallowed, see
                        // docs/CURRENT_WORK.md) covers three pickReasons that
                        // aren't equally cancel-safe: Attack (combatBegin-
                        // PlayerAttack) has committed nothing yet at this
                        // point -- the swing only happens in
                        // combatResolveAttackAgainstTarget once a target is
                        // confirmed -- so this is a genuinely free cancel,
                        // same as PickingItem/PickingSpell's own WHICH-spell/
                        // item choice, letting the player press F/I/Space
                        // instead. Spell and Webnet are different: by the
                        // time PickingTarget opens for them,
                        // combatCommitSpellChoice/combatCommitItemChoice has
                        // already run character::castSpell/useWebnet (slot/
                        // charge already spent -- see the empty-candidates
                        // branches just above each, which log a "finds no
                        // target"/silent-no-op and still end the round for
                        // the identical reason), so cancelling here can't
                        // refund that; it just leaves the round not yet
                        // finished (combatFinishPlayerAction not called),
                        // same as never having pressed Attack at all this
                        // round -- the very next action taken (Attack, Flee,
                        // Space, ...) still closes the round out normally,
                        // idempotent initiative roll included. A spell/
                        // webnet wasted this way is an accepted, honest
                        // consequence of picking a reason that had 2+ valid
                        // targets and then changing your mind -- not a bug.
                        //
                        // Aiming (Milestone 204) is the simplest case of
                        // all: combatBeginAim never rolls initiative in the
                        // first place (same as ViewPicking), so cancelling
                        // out of the free-look cursor costs nothing either.
                        combatSession.uiState = CombatUiState::Idle;
                    } else if (wantsQuit && !askInputActive) {
                        // Opens the confirmation instead of closing outright
                        // -- see quitConfirmOpen's own declaration comment
                        // above. Reached from Overworld/Zone idle and from
                        // Combat (which has no wantsQuit override of its
                        // own), always resetting to the safe "No" default.
                        quitConfirmOpen = true;
                        quitConfirmSelected = 1;
                    } else if (combatSession.active) {
                        // Combat's own input dispatch -- see CombatSession/
                        // CombatUiState's doc comments above for the state
                        // machine this drives. Reuses the same dx/dy/
                        // handleEnter the switch above already computed;
                        // F/M/I are read straight off `key` since the
                        // switch's own placeholder text for them only
                        // applies outside combat (see the `else` branch
                        // below).
                        switch (combatSession.uiState) {
                            case CombatUiState::AwaitContinue:
                                // Message-pacing queue (Milestone 199): each
                                // Enter reveals the next queued log line
                                // (see the bottom-bar render code) until
                                // pendingMessageEnd is reached, then applies
                                // the real state combatWrapUpRound/
                                // combatKnockedOutBy/combatBeginFlee/
                                // combatStartEncounter actually decided.
                                // Milestone 204: an auto-advancing queue
                                // (pendingMessageAutoAdvance) also advances
                                // on its own via the main loop's timer check
                                // -- Enter here still works too, as a manual
                                // skip-ahead, via the same shared
                                // combatAdvancePendingMessage.
                                if (handleEnter) combatAdvancePendingMessage();
                                break;
                            case CombatUiState::PickingTarget: {
                                const int candidateCount = static_cast<int>(combatSession.pickCandidates.size());
                                if (dy < 0) {
                                    combatSession.pickSelected =
                                        (combatSession.pickSelected - 1 + candidateCount) % candidateCount;
                                } else if (dy > 0) {
                                    combatSession.pickSelected = (combatSession.pickSelected + 1) % candidateCount;
                                } else if (handleEnter) {
                                    combatConfirmTarget();
                                }
                                break;
                            }
                            case CombatUiState::PickingSpell: {
                                const int optionCount = static_cast<int>(combatSession.spellPickIds.size());
                                if (dy < 0) {
                                    combatSession.spellPickSelected =
                                        (combatSession.spellPickSelected - 1 + optionCount) % optionCount;
                                } else if (dy > 0) {
                                    combatSession.spellPickSelected = (combatSession.spellPickSelected + 1) % optionCount;
                                } else if (handleEnter) {
                                    combatCommitSpellChoice(
                                        combatSession.spellPickIds[static_cast<size_t>(combatSession.spellPickSelected)]);
                                }
                                break;
                            }
                            case CombatUiState::PickingItem: {
                                const int optionCount = static_cast<int>(combatSession.itemPickKinds.size());
                                if (dy < 0) {
                                    combatSession.itemPickSelected =
                                        (combatSession.itemPickSelected - 1 + optionCount) % optionCount;
                                } else if (dy > 0) {
                                    combatSession.itemPickSelected = (combatSession.itemPickSelected + 1) % optionCount;
                                } else if (handleEnter) {
                                    combatCommitItemChoice(
                                        combatSession.itemPickKinds[static_cast<size_t>(combatSession.itemPickSelected)]);
                                }
                                break;
                            }
                            case CombatUiState::Won:
                            case CombatUiState::Lost:
                            case CombatUiState::Fled:
                                if (handleEnter) combatSession.active = false;
                                break;
                            // Milestone 200: rebound to match real DQoK's
                            // own first-letter verb hotkeys (confirmed via
                            // References/BattleFrames_extracted -- "MOVE
                            // VIEW AIM USE CAST QUICK DONE"), replacing the
                            // old Enter-for-Aim/m-for-Cast/i-for-Use
                            // mismatch. QUICK (Q) is still not built -- no
                            // equivalent mechanic, and Q is already
                            // reserved for quit/cancel above. Flee (F)
                            // isn't one of DQoK's own words -- kept as this
                            // project's own addition, unchanged key. Move
                            // has no dedicated key at all: direction keys
                            // already move directly in this engine, so
                            // dx/dy (arrows/numpad only, WASD removed
                            // above) covers DQoK's own MOVE verb without a
                            // mode-switch key.
                            case CombatUiState::Idle:
                                if (key == sf::Keyboard::Key::F) {
                                    combatBeginFlee();
                                } else if (key == sf::Keyboard::Key::C) {
                                    combatBeginCast();
                                } else if (key == sf::Keyboard::Key::U) {
                                    combatBeginUseItem();
                                } else if (key == sf::Keyboard::Key::V) {
                                    combatBeginView();
                                } else if (key == sf::Keyboard::Key::A) {
                                    combatBeginAim();
                                } else if (key == sf::Keyboard::Key::Space || key == sf::Keyboard::Key::D) {
                                    combatEndTurn();
                                } else if (dx != 0 || dy != 0) {
                                    combatBeginPlayerMove(dx, dy);
                                }
                                break;
                            // Milestone 204: dx/dy pan the free-look cursor
                            // by one cell anywhere on the 50x25 grid -- a
                            // viewfinder, not the character moving, so only
                            // bounds-clamped, no wall/occupancy check (see
                            // combatBeginAim's own doc comment). Enter
                            // commits via combatConfirmAim, a no-op if the
                            // cursor isn't on a legal target right now.
                            // Escape/Q cancels for free -- handled by the
                            // guard above this whole switch, alongside
                            // PickingSpell/PickingItem/ViewPicking/
                            // PickingTarget.
                            case CombatUiState::Aiming:
                                if (dx != 0 || dy != 0) {
                                    combatSession.aimCursor.x =
                                        std::clamp(combatSession.aimCursor.x + dx, 0, kCombatGridWidth - 1);
                                    combatSession.aimCursor.y =
                                        std::clamp(combatSession.aimCursor.y + dy, 0, kCombatGridHeight - 1);
                                } else if (handleEnter) {
                                    combatConfirmAim();
                                }
                                break;
                            case CombatUiState::ViewPicking: {
                                const int candidateCount = static_cast<int>(combatSession.viewCandidates.size());
                                if (dy < 0) {
                                    combatSession.viewSelected = (combatSession.viewSelected - 1 + candidateCount) % candidateCount;
                                } else if (dy > 0) {
                                    combatSession.viewSelected = (combatSession.viewSelected + 1) % candidateCount;
                                } else if (handleEnter) {
                                    combatConfirmView();
                                }
                                break;
                            }
                            case CombatUiState::ViewingCard:
                                // Any key dismisses -- same "informational,
                                // no cancel key needed" shape as
                                // helpOpen/journalOpen elsewhere in this
                                // file. Reaching this case at all already
                                // means a key was pressed.
                                combatSession.uiState = CombatUiState::Idle;
                                break;
                        }
                        // Message-pacing queue (Milestone 199): fires only
                        // when one of the real round-ending functions
                        // (combatWrapUpRound/combatKnockedOutBy/
                        // combatBeginFlee) actually ran during the switch
                        // above -- an ordinary action that leaves uiState at
                        // Idle without ending the round (e.g. a plain move)
                        // never sets roundJustConcluded, so it's correctly
                        // not paced (see the bottom-bar render code's
                        // always-shown log.back() line for how that case
                        // still surfaces). Diverts the real target state
                        // into AwaitContinue until every log line pushed
                        // since the action began has been shown one at a
                        // time.
                        if (combatSession.roundJustConcluded) {
                            combatSession.pendingTargetState = combatSession.uiState;
                            combatSession.pendingMessageIndex = combatSession.actionLogStart;
                            combatSession.pendingMessageEnd = combatSession.log.size();
                            combatSession.uiState = CombatUiState::AwaitContinue;
                            combatSession.roundJustConcluded = false;
                            // Milestone 204: only an ordinary round
                            // conclusion landing back on Idle auto-plays --
                            // a finishing Won/Lost/Fled stays a deliberate
                            // manual Enter-to-continue (see
                            // pendingMessageAutoAdvance's own doc comment).
                            combatSession.pendingMessageAutoAdvance =
                                combatSession.pendingTargetState == CombatUiState::Idle;
                            combatMessageClock.restart();
                        }
                    } else if (dialogueSession.active) {
                        // Dialogue's own input dispatch -- see
                        // DialogueSession/DialogueUiState's doc comments
                        // above. Same "reuse dx/dy/handleEnter the switch
                        // above already computed" shape as combat's own
                        // dispatch. Quit/Escape are handled earlier by this
                        // block's own dialogueSession.active guard above
                        // (PickingCandidate cancels, TopicPicker leaves same
                        // as "Nothing, thanks", the rest just continue), so
                        // this switch only ever sees dy/handleEnter/
                        // Backspace here.
                        switch (dialogueSession.uiState) {
                            case DialogueUiState::PickingCandidate: {
                                const int candidateCount = static_cast<int>(dialogueSession.candidates.size());
                                if (dy < 0) {
                                    dialogueSession.candidateSelected =
                                        (dialogueSession.candidateSelected - 1 + candidateCount) % candidateCount;
                                } else if (dy > 0) {
                                    dialogueSession.candidateSelected =
                                        (dialogueSession.candidateSelected + 1) % candidateCount;
                                } else if (handleEnter) {
                                    dialogueConfirmCandidate();
                                }
                                break;
                            }
                            case DialogueUiState::Greeting:
                            case DialogueUiState::TopicText:
                            case DialogueUiState::AskResponse:
                            case DialogueUiState::QuestOfferText:
                            case DialogueUiState::QuestAcceptText:
                            case DialogueUiState::QuestProgressText:
                            case DialogueUiState::QuestCompleteText:
                            case DialogueUiState::WayrethIntro:
                                if (handleEnter) dialogueContinue();
                                break;
                            case DialogueUiState::TopicPicker: {
                                const int topicCount = static_cast<int>(dialogueSession.topicLabels.size());
                                if (dy < 0) {
                                    dialogueSession.topicSelected =
                                        (dialogueSession.topicSelected - 1 + topicCount) % topicCount;
                                } else if (dy > 0) {
                                    dialogueSession.topicSelected = (dialogueSession.topicSelected + 1) % topicCount;
                                } else if (handleEnter) {
                                    dialogueConfirmTopic();
                                }
                                break;
                            }
                            case DialogueUiState::BoatOffer:
                                // Either North or South toggles between the
                                // two options -- mirrors talkTo's own picker
                                // loop exactly (GameLoop.cpp:1071-1072:
                                // `selected = selected == 0 ? 1 : 0;` on
                                // either direction, not a wrap-around list).
                                if (dy != 0) {
                                    dialogueSession.boatOfferSelected =
                                        dialogueSession.boatOfferSelected == 0 ? 1 : 0;
                                } else if (handleEnter) {
                                    if (dialogueSession.boatOfferSelected == 0) {
                                        dialogueBoardBoat();
                                    } else {
                                        dialogueDeclineBoat();
                                    }
                                }
                                break;
                            case DialogueUiState::RecruitOffer:
                                // Same either-direction-toggles shape as
                                // BoatOffer just above -- mirrors talkTo's
                                // own recruit picker loop (GameLoop.cpp:
                                // 1135-1136), identical to its boat picker.
                                if (dy != 0) {
                                    dialogueSession.recruitOfferSelected =
                                        dialogueSession.recruitOfferSelected == 0 ? 1 : 0;
                                } else if (handleEnter) {
                                    if (dialogueSession.recruitOfferSelected == 0) {
                                        dialogueJoinRecruit();
                                    } else {
                                        dialogueDeclineRecruit();
                                    }
                                }
                                break;
                            case DialogueUiState::QuestAcceptDecline:
                                // Same either-direction-toggles shape as
                                // BoatOffer/RecruitOffer above -- mirrors
                                // offerOrTurnInQuest's own Accept/Decline
                                // picker loop (GameLoop.cpp:1329-1330).
                                if (dy != 0) {
                                    dialogueSession.questOfferSelected =
                                        dialogueSession.questOfferSelected == 0 ? 1 : 0;
                                } else if (handleEnter) {
                                    if (dialogueSession.questOfferSelected == 0) {
                                        questAccept();
                                    } else {
                                        dialogueAfterGreeting(); // declined, nothing recorded
                                    }
                                }
                                break;
                            case DialogueUiState::WayrethChoice:
                                // Wraparound cycle through all 3 options
                                // (not a 2-way toggle) -- mirrors
                                // offerOrTurnInQuest's own ethic picker loop
                                // exactly (GameLoop.cpp:1471-1474).
                                if (dy < 0) {
                                    dialogueSession.wayrethChoiceSelected =
                                        (dialogueSession.wayrethChoiceSelected - 1 + 3) % 3;
                                } else if (dy > 0) {
                                    dialogueSession.wayrethChoiceSelected =
                                        (dialogueSession.wayrethChoiceSelected + 1) % 3;
                                } else if (handleEnter) {
                                    questResolveWayreth();
                                }
                                break;
                            case DialogueUiState::AskInput:
                                if (wantsBackspace) {
                                    if (!dialogueSession.askInputBuffer.empty()) {
                                        dialogueSession.askInputBuffer.pop_back();
                                    }
                                } else if (handleEnter) {
                                    dialogueSubmitAsk();
                                }
                                break;
                        }
                    } else if (shopSession.active) {
                        // Shop's own input dispatch -- a single screen shape (no
                        // UiState enum, see ShopSession's own comment), so this is
                        // flatter than combat/dialogue's per-state switches. 'I' is
                        // read directly off `key` rather than the outer switch
                        // (which leaves it a no-op, see the switch's own comment)
                        // -- while a shop is open, 'I' toggles buy/sell instead of
                        // opening Inventory, same "this session owns the key"
                        // precedent combat's Idle state already established for
                        // F/M/I.
                        const int buyCount = static_cast<int>(
                            character::availableShopItems(state.character, shopSession.catalog).size());
                        const int sellCount = static_cast<int>(character::sellableItems(state.character).size());
                        const int activeCount = shopSession.sellMode ? sellCount : buyCount;
                        if (dy < 0) {
                            shopSession.selected =
                                activeCount == 0 ? 0 : (shopSession.selected - 1 + activeCount) % activeCount;
                        } else if (dy > 0) {
                            shopSession.selected = activeCount == 0 ? 0 : (shopSession.selected + 1) % activeCount;
                        } else if (handleEnter) {
                            if (shopSession.sellMode) {
                                if (sellCount > 0) {
                                    character::PurchaseResult result =
                                        character::sellItem(state.character, shopSession.selected);
                                    shopSession.message = result.message;
                                }
                            } else if (buyCount > 0) {
                                character::PurchaseResult result = character::purchaseItem(
                                    state.character, shopSession.selected, shopSession.catalog);
                                shopSession.message = result.message;
                            }
                        } else if (key == sf::Keyboard::Key::I) {
                            shopSession.sellMode = !shopSession.sellMode;
                            shopSession.selected = 0;
                            shopSession.message.clear();
                        }
                    } else if (inventorySession.active) {
                        // Inventory's own input dispatch -- mirrors
                        // GameLoop::handleInventory (GameLoop.cpp:1638-1675)
                        // exactly, including its North/South wrap-if-nonempty
                        // and "reset cursor to 0, the list just changed shape"
                        // rule after any Enter action.
                        auto& inventory = state.character.inventory;
                        const int itemCount = static_cast<int>(inventory.size());
                        if (dy < 0) {
                            inventorySession.selected =
                                itemCount == 0 ? 0 : (inventorySession.selected - 1 + itemCount) % itemCount;
                        } else if (dy > 0) {
                            inventorySession.selected = itemCount == 0 ? 0 : (inventorySession.selected + 1) % itemCount;
                        } else if (handleEnter && itemCount > 0) {
                            const character::ItemKind kind = inventory[static_cast<size_t>(inventorySession.selected)].kind;
                            if (kind == character::ItemKind::Potion) {
                                character::PurchaseResult result =
                                    character::drinkPotion(state.character, inventorySession.selected);
                                inventorySession.message = result.message;
                            } else if (kind == character::ItemKind::Webnet ||
                                       kind == character::ItemKind::BroochOfImog) {
                                inventorySession.message = "That can only be used in combat.";
                            } else if (kind == character::ItemKind::QuestItem) {
                                inventorySession.message = "That's meant for someone else -- you'll need to deliver it.";
                            } else {
                                character::equipInventoryItem(state.character, inventorySession.selected);
                                inventorySession.message.clear();
                            }
                            inventorySession.selected = 0;
                        }
                    } else if (state.mode == game::Mode::Zone && currentZone != nullptr &&
                               isEffectiveMenuTown() && letterIndexForKey(key) >= 0) {
                        // Gold-Box town menu (Milestone 206, docs/ZONE_NOTES.md's
                        // "Town menus"): every letter key is interpreted as a
                        // destination-selection attempt here, full stop -- no
                        // fall-through to a same-lettered global command like
                        // Inventory/Journal/Rest, matching how the original Gold
                        // Box town-menu screens had no other interface running
                        // alongside them. Quit ('Q'/Escape, the wantsQuit branch
                        // above, checked before this one) and non-letter global
                        // commands (Help's Slash, ...) are unaffected either way.
                        const char pressed = static_cast<char>('A' + letterIndexForKey(key));
                        const std::vector<TownMenuItem> items = buildTownMenuItems(*currentZone, townMenuLeaveLabel());
                        const TownMenuItem* match = nullptr;
                        for (const TownMenuItem& item : items) {
                            if (item.hotkeyChar == pressed) {
                                match = &item;
                                break;
                            }
                        }
                        if (match == nullptr) {
                            pushLog("Nothing here by that name.");
                        } else if (match->hotkeyChar == 'L') {
                            leaveCurrentZone();
                        } else if (match->isPortal) {
                            if (!enterZone(match->portalTarget, true)) {
                                pushLog("That doorway doesn't lead anywhere in this build.");
                            }
                        } else {
                            state.zoneX = match->x;
                            state.zoneY = match->y;
                            const world::PointOfInterest* poi = currentZone->poiAt(match->x, match->y);
                            if (poi != nullptr && poi->isShop) {
                                shopBegin();
                            } else if (poi != nullptr && poi->isBed) {
                                restBegin(true);
                            } else {
                                dialogueBegin();
                            }
                        }
                    } else if (state.mode == game::Mode::Zone && currentZone != nullptr &&
                               isEffectiveMenuTown()) {
                        // Any other key while in a town menu (arrows, Enter,
                        // digits, ...) -- no walking and no Enter-to-confirm in
                        // this interaction model, so nothing happens. Silent, to
                        // avoid log spam from an arrow key pressed out of habit.
                    } else if (handleEnter) {
                        if (state.mode == game::Mode::Overworld) {
                            const world::Location* here = world.locationAt(state.x, state.y);
                            if (here == nullptr || !enterZone(here->id, false)) {
                                pushLog("There's nothing to enter here.");
                            }
                        } else if (currentZone) {
                            if (const std::string* portalTarget = currentZone->portalAt(state.zoneX, state.zoneY)) {
                                if (!enterZone(*portalTarget, true)) {
                                    pushLog("That doorway doesn't lead anywhere in this build.");
                                }
                            } else if (state.zoneX == currentZone->entryX() &&
                                       state.zoneY == currentZone->entryY()) {
                                leaveCurrentZone();
                            } else {
                                pushLog("Nothing to step through here.");
                            }
                        }
                    } else if (key == sf::Keyboard::Key::C) {
                        sheetOpen = true;
                    } else if (wantsTalk) {
                        dialogueBegin();
                    } else if (wantsShop) {
                        shopBegin();
                    } else if (key == sf::Keyboard::Key::I) {
                        inventoryBegin();
                    } else if (wantsRest) {
                        restBegin(false);
                    } else if (wantsBedRest) {
                        restBegin(true);
                    } else if (wantsLog) {
                        logSession.active = true;
                        logSession.scrollOffset = -1; // start at the bottom (most recent) every time it's opened
                    } else if (wantsLook) {
                        lookBegin();
                    } else if (wantsJournal) {
                        journalOpen = true;
                    } else if (wantsWorldMap) {
                        worldMapOpen = true;
                    } else if (wantsHelp) {
                        helpOpen = true;
                    } else if (!placeholder.empty()) {
                        pushLog(placeholder);
                    } else if (dx != 0 || dy != 0) {
                        if (state.mode == game::Mode::Overworld) {
                            const int nx = state.x + dx;
                            const int ny = state.y + dy;
                            const world::TerrainInfo& terrain = world::terrainFor(grid.terrainCodeAt(nx, ny));
                            // Milestone 187: corner-cutting check -- a
                            // diagonal step (both dx and dy nonzero) also
                            // needs both flanking cardinal tiles passable,
                            // not just the destination itself, or the
                            // player could squeeze diagonally between two
                            // impassable tiles. A straight move never
                            // enters this (dx or dy is 0), so its own
                            // corner tile is never even looked at.
                            const world::TerrainInfo* blockingCorner = nullptr;
                            if (dx != 0 && dy != 0) {
                                const world::TerrainInfo& flankX = world::terrainFor(grid.terrainCodeAt(nx, state.y));
                                const world::TerrainInfo& flankY = world::terrainFor(grid.terrainCodeAt(state.x, ny));
                                if (!flankX.passable) blockingCorner = &flankX;
                                else if (!flankY.passable) blockingCorner = &flankY;
                            }
                            if (terrain.passable && blockingCorner == nullptr) {
                                state.x = nx;
                                state.y = ny;
                                // hoursElapsed is the sole source of truth for
                                // in-game time (GameState.h) -- only overworld
                                // travel advances it, matching
                                // GameLoop::tryMoveOverworld exactly.
                                state.minutesElapsed += terrain.minutesToCross;
                                state.hoursElapsed += state.minutesElapsed / 60;
                                state.minutesElapsed %= 60;
                                const world::Location* here = world.locationAt(state.x, state.y);
                                if (here != nullptr) {
                                    pushLog("Arrived at " + here->name + ".");
                                    // Mirrors GameLoop::tryMoveOverworld
                                    // (GameLoop.cpp:690-691) -- was missing
                                    // here entirely before the quest system
                                    // port, silently breaking every Visit
                                    // objective for ordinary overworld
                                    // walking (only the character-creation
                                    // starting tile and boat arrival tracked
                                    // visitedLocations until now).
                                    state.visitedLocations.insert(here->id);
                                    checkQuestReadiness();
                                }
                                // Random encounters (see GameLoop::
                                // tryMoveOverworld): towns/named places stay
                                // safe, everywhere else has a per-terrain
                                // chance per move. Computed the same way --
                                // straight-line distance to the nearest town,
                                // so MonsterCatalog can keep high-danger
                                // monsters away from starting towns.
                                if (here == nullptr && monsterCatalog.size() > 0 &&
                                    character::roll(1, 100) <= terrain.encounterChancePercent) {
                                    long long bestDistSq = -1;
                                    for (const world::Location& loc : world.allLocations()) {
                                        if (!loc.isTown) continue;
                                        long long ddx = loc.x - state.x;
                                        long long ddy = loc.y - state.y;
                                        long long distSq = ddx * ddx + ddy * ddy;
                                        if (bestDistSq < 0 || distSq < bestDistSq) bestDistSq = distSq;
                                    }
                                    int townDistance =
                                        bestDistSq < 0
                                            ? 0
                                            : static_cast<int>(std::llround(std::sqrt(static_cast<double>(bestDistSq))));
                                    combatStartEncounter(monsterCatalog.randomMonster(terrain.code, townDistance));
                                }
                            } else if (!terrain.passable) {
                                pushLog("Blocked: cannot walk onto " + std::string(terrain.name) + ".");
                            } else {
                                // terrain.passable is true here, so
                                // blockingCorner must be the reason (see
                                // the guard above) -- the destination
                                // itself is fine, but cutting the corner
                                // to reach it diagonally isn't.
                                pushLog("Blocked: can't cut across " + std::string(blockingCorner->name) + ".");
                            }
                        } else if (currentZone) {
                            const int nx = state.zoneX + dx;
                            const int ny = state.zoneY + dy;
                            const bool isPoi = currentZone->poiAt(nx, ny) != nullptr;
                            const world::ZoneTileInfo& tile = world::zoneTileFor(currentZone->tileCodeAt(nx, ny));
                            // Milestone 187: same corner-cutting check as
                            // the Overworld branch above, reusing the
                            // identical isPoi-or-passable rule the
                            // destination tile already uses.
                            const world::ZoneTileInfo* blockingCorner = nullptr;
                            if (dx != 0 && dy != 0) {
                                const bool flankXPoi = currentZone->poiAt(nx, state.zoneY) != nullptr;
                                const world::ZoneTileInfo& flankX =
                                    world::zoneTileFor(currentZone->tileCodeAt(nx, state.zoneY));
                                const bool flankYPoi = currentZone->poiAt(state.zoneX, ny) != nullptr;
                                const world::ZoneTileInfo& flankY =
                                    world::zoneTileFor(currentZone->tileCodeAt(state.zoneX, ny));
                                if (!flankXPoi && !flankX.passable) blockingCorner = &flankX;
                                else if (!flankYPoi && !flankY.passable) blockingCorner = &flankY;
                            }
                            if ((isPoi || tile.passable) && blockingCorner == nullptr) {
                                state.zoneX = nx;
                                state.zoneY = ny;
                                if (const world::PointOfInterest* poi = currentZone->poiAt(state.zoneX, state.zoneY)) {
                                    pushLog("Here: " + poi->name + ".");
                                }
                            } else if (!isPoi && !tile.passable) {
                                pushLog("Blocked: cannot walk onto " + std::string(tile.name) + ".");
                            } else {
                                // isPoi||tile.passable is true here, so
                                // blockingCorner must be the reason -- same
                                // reasoning as the Overworld branch above.
                                pushLog("Blocked: can't cut across " + std::string(blockingCorner->name) + ".");
                            }
                        }
                    }

                    // Autosaved after every processed keypress, unconditionally
                    // -- same convention GameLoop::run() uses (GameLoop.cpp:408,
                    // docs/ARCHITECTURE.md's "When it saves"), just applied per
                    // KeyPressed event here instead of per outer-loop iteration.
                    // Falls through from every branch above (including the
                    // quit-confirm "Yes" case, which just calls window.close()
                    // and reaches here same as any other key), so this one call
                    // site also covers "quit captures the last action" the same
                    // way console's Key::Quit case does. One real difference
                    // from console, worth noting: a multi-round SFML combat
                    // encounter is many separate KeyPressed events rather than
                    // one blocking GameLoop::runCombat call, so this autosaves
                    // once per round, not just once when the fight ends --
                    // finer granularity than console, not coarser, so no less
                    // safe.
                    game::SaveGame::save(state, activeSavePath);
                } else if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
                    // The only source of real typed characters in this build
                    // (KeyPressed carries a physical key code, not text) --
                    // only acts while the ask-input box is open. True 7-bit
                    // ASCII only (CLAUDE.md), same printable range and length
                    // cap (60) as Console::readLine; Enter/Escape/Backspace
                    // all report codepoints below 0x20 here and are already
                    // handled via KeyPressed, so they're naturally excluded.
                    if (dialogueSession.active && dialogueSession.uiState == DialogueUiState::AskInput &&
                        textEntered->unicode >= 0x20 && textEntered->unicode < 0x7F &&
                        dialogueSession.askInputBuffer.size() < 60) {
                        dialogueSession.askInputBuffer.push_back(static_cast<char>(textEntered->unicode));
                    }
                }
            }

            // Milestone 204: auto-advances a round's AwaitContinue message
            // queue on its own once pendingMessageAutoAdvance is set,
            // instead of waiting for an Enter press per line -- runs once
            // per real frame (this loop is a continuous 60fps pollEvent-
            // then-draw loop, window.setFramerateLimit(60) above, so no new
            // timing infrastructure is needed beyond combatMessageClock).
            // Enter (handled above, inside the KeyPressed branch) still
            // advances manually too, as a faster skip-ahead.
            if (combatSession.active && combatSession.uiState == CombatUiState::AwaitContinue &&
                combatSession.pendingMessageAutoAdvance &&
                combatMessageClock.getElapsedTime().asMilliseconds() >= kMessageAutoAdvanceMs) {
                combatAdvancePendingMessage();
            }

            window.clear(sf::Color::Black);
            window.setView(mapView);

            if (combatSession.active) {
                // Milestone 185: the field is now 50x25 and no longer fits
                // the viewport at a readable tile size, so the camera
                // scrolls -- the exact same clamped-follow approach the
                // Overworld branch below already uses, rather than the
                // fixed centered draw this used when the whole 15x9 grid
                // fit on screen. Follows the player while idle/moving, and
                // the currently-highlighted candidate while PickingTarget,
                // so an off-screen target is never picked blind. No sprite
                // art here either (same reasoning as zones): a uniform
                // floor color plus each entity's own marker color/letter is
                // still the full visual vocabulary for this phase.
                const float combatPxW = static_cast<float>(kCombatGridWidth) * kCombatTilePx;
                const float combatPxH = static_cast<float>(kCombatGridHeight) * kCombatTilePx;
                combat::GridPos combatFocus = combatSession.playerPos;
                if (combatSession.uiState == CombatUiState::PickingTarget && !combatSession.pickCandidates.empty()) {
                    const int focusedIdx = combatSession.pickCandidates[static_cast<size_t>(combatSession.pickSelected)];
                    combatFocus = combatSession.instancePositions[static_cast<size_t>(focusedIdx)];
                } else if (combatSession.uiState == CombatUiState::Aiming) {
                    // Milestone 204: follows the free-look cursor itself,
                    // not any particular instance -- the whole point is
                    // being able to pan somewhere nothing is standing.
                    combatFocus = combatSession.aimCursor;
                } else if (combatSession.uiState == CombatUiState::AwaitContinue &&
                           combatSession.pendingMessageIndex < combatSession.logActorForLine.size()) {
                    // Turn-follow camera (Milestone 200): while the paced
                    // message replay is showing a line, center on whoever
                    // that line is actually about instead of always the
                    // player -- see combatActorPosition's own doc comment.
                    combatFocus = combatActorPosition(combatSession.logActorForLine[combatSession.pendingMessageIndex]);
                }
                const float combatFocusPxX = (static_cast<float>(combatFocus.x) + 0.5f) * kCombatTilePx;
                const float combatFocusPxY = (static_cast<float>(combatFocus.y) + 0.5f) * kCombatTilePx;
                const sf::Vector2f combatViewSize = combatMapView.getSize();
                const float combatHalfW = combatViewSize.x / 2.f;
                const float combatHalfH = combatViewSize.y / 2.f;
                const float combatCamX = std::clamp(combatFocusPxX, combatHalfW, std::max(combatHalfW, combatPxW - combatHalfW));
                const float combatCamY = std::clamp(combatFocusPxY, combatHalfH, std::max(combatHalfH, combatPxH - combatHalfH));
                combatMapView.setCenter(sf::Vector2f(combatCamX, combatCamY));
                window.setView(combatMapView);

                for (int gy = 0; gy < kCombatGridHeight; ++gy) {
                    for (int gx = 0; gx < kCombatGridWidth; ++gx) {
                        const float cx = (static_cast<float>(gx) + 0.5f) * kCombatTilePx;
                        const float cy = (static_cast<float>(gy) + 0.5f) * kCombatTilePx;
                        // Milestone 188: a wall cell draws combatWallTileShape
                        // instead of the ordinary floor tile -- same branch
                        // as combatAnimateAiStep's identical tile loop above.
                        sf::RectangleShape& tileShape = (combatSession.battleMap != nullptr &&
                                                          combatSession.battleMap->isWall(gx, gy))
                                                             ? combatWallTileShape
                                                             : combatTileShape;
                        tileShape.setPosition(
                            sf::Vector2f(cx - (kCombatTilePx - 2.f) / 2.f, cy - (kCombatTilePx - 2.f) / 2.f));
                        window.draw(tileShape);
                    }
                }
                combatGridBorder.setSize(sf::Vector2f(combatPxW, combatPxH));
                combatGridBorder.setPosition(sf::Vector2f(0.f, 0.f));
                window.draw(combatGridBorder);

                if (combatSession.uiState == CombatUiState::PickingTarget && !combatSession.pickCandidates.empty()) {
                    const int pickedIdx = combatSession.pickCandidates[static_cast<size_t>(combatSession.pickSelected)];
                    const combat::GridPos pos = combatSession.instancePositions[static_cast<size_t>(pickedIdx)];
                    // Milestone 190: the highlight spans the whole footprint
                    // (degenerates to the exact single-cell box above for
                    // every ordinary 1x1 monster).
                    const float fw = static_cast<float>(combatSession.monster.footprintWidth);
                    const float fh = static_cast<float>(combatSession.monster.footprintHeight);
                    const float cx = (static_cast<float>(pos.x) + fw / 2.f) * kCombatTilePx;
                    const float cy = (static_cast<float>(pos.y) + fh / 2.f) * kCombatTilePx;
                    combatPickHighlight.setSize(sf::Vector2f(kCombatTilePx * fw - 6.f, kCombatTilePx * fh - 6.f));
                    combatPickHighlight.setPosition(
                        sf::Vector2f(cx - (kCombatTilePx * fw - 6.f) / 2.f, cy - (kCombatTilePx * fh - 6.f) / 2.f));
                    window.draw(combatPickHighlight);
                } else if (combatSession.uiState == CombatUiState::Aiming) {
                    // Milestone 204: same box, but drawn at the cursor
                    // itself rather than a resolved candidate -- sized to
                    // a monster's whole footprint when the cursor happens
                    // to be on one (same sizing as PickingTarget's box
                    // above), else a plain single cell.
                    const int aimedIdx = combatInstanceAtCell(combatSession.aimCursor);
                    combat::GridPos pos = combatSession.aimCursor;
                    float fw = 1.f;
                    float fh = 1.f;
                    if (aimedIdx >= 0) {
                        pos = combatSession.instancePositions[static_cast<size_t>(aimedIdx)];
                        fw = static_cast<float>(combatSession.monster.footprintWidth);
                        fh = static_cast<float>(combatSession.monster.footprintHeight);
                    }
                    const float cx = (static_cast<float>(pos.x) + fw / 2.f) * kCombatTilePx;
                    const float cy = (static_cast<float>(pos.y) + fh / 2.f) * kCombatTilePx;
                    combatPickHighlight.setSize(sf::Vector2f(kCombatTilePx * fw - 6.f, kCombatTilePx * fh - 6.f));
                    combatPickHighlight.setPosition(
                        sf::Vector2f(cx - (kCombatTilePx * fw - 6.f) / 2.f, cy - (kCombatTilePx * fh - 6.f) / 2.f));
                    window.draw(combatPickHighlight);
                }

                constexpr unsigned kCombatGlyphCharSize = 16;
                for (size_t i = 0; i < combatSession.instances.size(); ++i) {
                    if (combatSession.instances[i].hp <= 0) continue;
                    const combat::GridPos pos = combatSession.instancePositions[i];
                    // Milestone 190: centered on the whole footprint, radius
                    // scaled by its larger dimension -- degenerates to the
                    // exact single-cell circle above for every ordinary 1x1
                    // monster (footprintWidth == footprintHeight == 1).
                    const float footprintRadius =
                        kCombatTilePx * 0.32f *
                        static_cast<float>(std::max(combatSession.monster.footprintWidth, combatSession.monster.footprintHeight));
                    const float cx =
                        (static_cast<float>(pos.x) + combatSession.monster.footprintWidth / 2.f) * kCombatTilePx;
                    const float cy =
                        (static_cast<float>(pos.y) + combatSession.monster.footprintHeight / 2.f) * kCombatTilePx;
                    // -wide/-tall/-four art only ever renders bigger than
                    // the real footprint into a neighbor cell confirmed
                    // empty this frame -- see drawCombatSpriteToken's own
                    // comment and combatAnimateAiStep's matching check
                    // above.
                    auto monsterNeighborFree = [&](int dx, int dy) {
                        const combat::GridPos neighbor{pos.x + dx, pos.y + dy};
                        if (combatCellOccupied(neighbor)) return false;
                        return !(neighbor.x == combatSession.playerPos.x && neighbor.y == combatSession.playerPos.y);
                    };
                    if (!drawCombatSpriteToken(combatSession.monster.id, combatSession.monster.footprintWidth,
                                                combatSession.monster.footprintHeight, cx, cy, pos, std::nullopt,
                                                false, monsterNeighborFree)) {
                        combatMonsterMarker.setRadius(footprintRadius);
                        combatMonsterMarker.setOrigin(sf::Vector2f(footprintRadius, footprintRadius));
                        combatMonsterMarker.setPosition(sf::Vector2f(cx, cy));
                        window.draw(combatMonsterMarker);
                        sf::Text glyph(font, std::string(1, static_cast<char>('A' + i)), kCombatGlyphCharSize);
                        glyph.setFillColor(sf::Color::White);
                        glyph.setPosition(sf::Vector2f(cx - 5.f, cy - 10.f));
                        window.draw(glyph);
                    }
                }
                // Companions/player have no -wide/-tall/-four art yet, so
                // the neighbor check is never actually consulted for them.
                auto noNeighborCheckNeeded = [](int, int) { return true; };
                for (size_t i = 0; i < combatSession.companionPositions.size(); ++i) {
                    if (!combatCompanionAlive(i)) continue;
                    const combat::GridPos pos = combatSession.companionPositions[i];
                    const float cx = (static_cast<float>(pos.x) + 0.5f) * kCombatTilePx;
                    const float cy = (static_cast<float>(pos.y) + 0.5f) * kCombatTilePx;
                    if (!drawCombatSpriteToken(state.companions[i].id, 1, 1, cx, cy, pos, std::nullopt, false,
                                                noNeighborCheckNeeded)) {
                        combatCompanionMarker.setPosition(sf::Vector2f(cx, cy));
                        window.draw(combatCompanionMarker);
                        sf::Text glyph(font, std::string(1, static_cast<char>('c' + i)), kCombatGlyphCharSize);
                        glyph.setFillColor(sf::Color::White);
                        glyph.setPosition(sf::Vector2f(cx - 5.f, cy - 10.f));
                        window.draw(glyph);
                    }
                }
                {
                    const float cx = (static_cast<float>(combatSession.playerPos.x) + 0.5f) * kCombatTilePx;
                    const float cy = (static_cast<float>(combatSession.playerPos.y) + 0.5f) * kCombatTilePx;
                    if (!drawCombatSpriteToken("player", 1, 1, cx, cy, combatSession.playerPos,
                                                combatNearestLivingEnemyPos(), false, noNeighborCheckNeeded)) {
                        playerMarker.setPosition(sf::Vector2f(cx, cy));
                        window.draw(playerMarker);
                    }
                }
            } else if (state.mode == game::Mode::Overworld) {
                const float playerPxX = (static_cast<float>(state.x) + 0.5f) * pxPerTileX;
                const float playerPxY = (static_cast<float>(state.y) + 0.5f) * pxPerTileY;

                const sf::Vector2f viewSize = mapView.getSize();
                const float halfW = viewSize.x / 2.f;
                const float halfH = viewSize.y / 2.f;
                const float camX = std::clamp(playerPxX, halfW, std::max(halfW, worldW - halfW));
                const float camY = std::clamp(playerPxY, halfH, std::max(halfH, worldH - halfH));
                mapView.setCenter(sf::Vector2f(camX, camY));
                window.setView(mapView);

                window.draw(mapSprite);
                for (const world::Location& loc : world.allLocations()) {
                    const float px = (static_cast<float>(loc.x) + 0.5f) * pxPerTileX;
                    const float py = (static_cast<float>(loc.y) + 0.5f) * pxPerTileY;
                    locationMarker.setFillColor(loc.isTown ? sf::Color(60, 220, 90) : sf::Color(220, 60, 60));
                    locationMarker.setPosition(sf::Vector2f(px, py));
                    window.draw(locationMarker);
                }
                playerMarker.setPosition(sf::Vector2f(playerPxX, playerPxY));
                window.draw(playerMarker);
            } else if (currentZone) {
                // Whole zone always fits the viewport (see kZoneTilePx) --
                // centered, not scrolled, unlike the overworld.
                const float zonePxW = static_cast<float>(currentZone->width()) * kZoneTilePx;
                const float zonePxH = static_cast<float>(currentZone->height()) * kZoneTilePx;
                mapView.setCenter(sf::Vector2f(zonePxW / 2.f, zonePxH / 2.f));
                window.setView(mapView);

                constexpr unsigned kPoiLabelCharSize = 11;
                for (int zy = 0; zy < currentZone->height(); ++zy) {
                    for (int zx = 0; zx < currentZone->width(); ++zx) {
                        zoneTileShape.setFillColor(colorForZoneTile(currentZone->tileCodeAt(zx, zy)));
                        zoneTileShape.setPosition(
                            sf::Vector2f(static_cast<float>(zx) * kZoneTilePx, static_cast<float>(zy) * kZoneTilePx));
                        window.draw(zoneTileShape);

                        const float centerX = (static_cast<float>(zx) + 0.5f) * kZoneTilePx;
                        const float centerY = (static_cast<float>(zy) + 0.5f) * kZoneTilePx;
                        const std::string* portalTarget = currentZone->portalAt(zx, zy);
                        const world::PointOfInterest* poi = currentZone->poiAt(zx, zy);
                        // A recruited companion's own POI stops rendering
                        // entirely once they've joined -- they're travelling
                        // with the party now, not still standing here.
                        // Mirrors the same fix in render::MapRenderer::
                        // drawZoneFrame (console build); both builds
                        // previously kept drawing this tile's icon/label
                        // forever. Confirmed live 2026-09-10: Bren Alder's
                        // icon stayed in Solace after joining.
                        if (poi != nullptr && !poi->recruitCompanionId.empty() &&
                            companionAlreadyRecruited(poi->recruitCompanionId)) {
                            poi = nullptr;
                        }
                        if (poi) {
                            const PoiKind kind = poiKindFor(portalTarget != nullptr, *poi);
                            drawPoiIcon(window, poiIconShapes, kind, kZoneTilePx, centerX, centerY);
                        }
                        if (poi && !poi->name.empty()) {
                            sf::Text label(font, poi->name, kPoiLabelCharSize);
                            label.setFillColor(sf::Color(240, 240, 220));
                            label.setPosition(sf::Vector2f(centerX + kZoneTilePx * 0.4f, centerY - kZoneTilePx * 0.5f));
                            window.draw(label);
                        }
                        if (zx == currentZone->entryX() && zy == currentZone->entryY()) {
                            entryMarker.setPosition(sf::Vector2f(centerX, centerY));
                            window.draw(entryMarker);
                        }
                    }
                }

                const float playerPxX = (static_cast<float>(state.zoneX) + 0.5f) * kZoneTilePx;
                const float playerPxY = (static_cast<float>(state.zoneY) + 0.5f) * kZoneTilePx;
                playerMarker.setPosition(sf::Vector2f(playerPxX, playerPxY));
                window.draw(playerMarker);
            }

            window.setView(uiView);
            // Milestone 200 gave combat its own compact, content-sized
            // card box in place of the old always-on full-height sidebar
            // fill; Milestone 201 did the same for the non-combat
            // Overworld/Zone sidebar below (see drawCardPanel, shared by
            // both) -- the old flat sidebarBg fill is gone entirely now.

            float lineY = 16.f;
            const float lineX = mapWidth + 16.f;
            const float lineHeight = static_cast<float>(kSidebarCharSize) + 6.f;
            auto drawLine = [&](const std::string& text, sf::Color color) {
                sf::Text sfText(font, text, kSidebarCharSize);
                sfText.setFillColor(color);
                sfText.setPosition(sf::Vector2f(lineX, lineY));
                window.draw(sfText);
                lineY += lineHeight;
            };

            const float maxLineWidthPx = sidebarWidth - 32.f;
            // The combat log below already wraps through this -- these
            // static/dynamic prompt lines (e.g. "ATTACK (Enter)   MOVE
            // (wasd)   FLEE (f)") didn't, so a long enough one ran past the
            // sidebar's own width and was clipped by the window's right
            // edge (found live, mid-fight). Wrapping them the same way.
            auto drawWrappedLine = [&](const std::string& text, sf::Color color) {
                for (const std::string& wrapped : wrapToPixelWidth(font, kSidebarCharSize, text, maxLineWidthPx)) {
                    drawLine(wrapped, color);
                }
            };

            if (combatSession.active) {
                // Persistent stat card (Milestone 199, tightened to DQoK's
                // real compact format at Milestone 200): defaults to the
                // player, but while AwaitContinue is pacing a round's
                // messages, follows whoever's turn the currently-displayed
                // line is about (see combatActorPosition's own doc comment
                // for the same mechanism driving the camera). View ('v')
                // still opens its own separate ViewingCard overlay -- the
                // full detailed card (THAC0/Status), unaffected by this --
                // to check anyone else on demand.
                CombatSession::ViewCandidate primaryActor{CombatSession::ViewCandidate::Kind::Player, 0};
                std::string primaryLabel = state.character.name;
                if (combatSession.uiState == CombatUiState::AwaitContinue &&
                    combatSession.pendingMessageIndex < combatSession.logActorForLine.size()) {
                    primaryActor = combatSession.logActorForLine[combatSession.pendingMessageIndex];
                    if (primaryActor.kind == CombatSession::ViewCandidate::Kind::Companion) {
                        primaryLabel = state.companions[static_cast<size_t>(primaryActor.index)].character.name;
                    } else if (primaryActor.kind == CombatSession::ViewCandidate::Kind::Monster) {
                        primaryLabel = combatMonsterLabel(primaryActor.index);
                    }
                }
                const std::vector<std::string> primaryLines = combatBuildCompactCardLines(primaryActor);

                // Secondary target card: DQoK's own "a second small card
                // appears below it" idiom once a target is highlighted,
                // replacing the old roster's ">" cursor with the same data
                // the picker itself already computes. Milestone 204: also
                // shown live while Aiming, whenever the free-look cursor
                // happens to be sitting on a living instance -- no separate
                // confirm step needed the way View's own card requires,
                // since Aiming already redraws every frame the cursor moves.
                int secondaryFocusedIdx = -1;
                if (combatSession.uiState == CombatUiState::PickingTarget && !combatSession.pickCandidates.empty()) {
                    secondaryFocusedIdx =
                        combatSession.pickCandidates[static_cast<size_t>(combatSession.pickSelected)];
                } else if (combatSession.uiState == CombatUiState::Aiming) {
                    secondaryFocusedIdx = combatInstanceAtCell(combatSession.aimCursor);
                }
                const bool showSecondaryCard = secondaryFocusedIdx >= 0;
                std::vector<std::string> secondaryLines;
                if (showSecondaryCard) {
                    secondaryLines = combatBuildCompactCardLines(
                        {CombatSession::ViewCandidate::Kind::Monster, secondaryFocusedIdx});
                }

                // Compact card box (Milestone 200): sized to actual content
                // (one name row + each card's stat rows, plus a small gap
                // before the secondary card) instead of the old always-on
                // full-height sidebar fill -- see drawCardPanel's own doc
                // comment. Width/position still live inside the same
                // reserved sidebar column (mapWidth/sidebarWidth
                // themselves are untouched, so the map viewport is
                // unaffected) -- only the height and the fact that it's a
                // bordered box rather than a flat fill are new.
                constexpr float kCardPaddingY = 12.f;
                constexpr float kCardMarginTop = 8.f;
                const float cardX = mapWidth + 8.f;
                const float cardWidth = sidebarWidth - 16.f;
                float cardHeight = kCardPaddingY * 2.f + lineHeight * static_cast<float>(1 + primaryLines.size());
                if (showSecondaryCard) {
                    cardHeight += lineHeight * 0.3f + lineHeight * static_cast<float>(1 + secondaryLines.size());
                }
                drawCardPanel(cardX, kCardMarginTop, cardWidth, cardHeight);

                lineY = kCardMarginTop + kCardPaddingY;
                drawLine(primaryLabel, sf::Color(255, 215, 0));
                for (const std::string& cardLine : primaryLines) {
                    drawWrappedLine(cardLine, sf::Color::White);
                }

                if (showSecondaryCard) {
                    lineY += lineHeight * 0.3f;
                    drawLine(combatMonsterLabel(secondaryFocusedIdx), sf::Color(220, 100, 100));
                    for (const std::string& cardLine : secondaryLines) {
                        drawWrappedLine(cardLine, sf::Color::White);
                    }
                }

                // Bottom command/message bar (Milestone 199) -- full window
                // width, drawn in the same uiView space as the sidebar
                // above, just anchored to its own margin/width instead of
                // the sidebar's.
                window.draw(combatBottomBarBg);
                float barY = static_cast<float>(windowH) - kCombatBottomBarHeight + 12.f;
                constexpr float kBarMarginX = 16.f;
                const float barMaxWidthPx = static_cast<float>(windowW) - 2.f * kBarMarginX;
                auto drawBarLine = [&](const std::string& text, sf::Color color) {
                    sf::Text sfText(font, text, kSidebarCharSize);
                    sfText.setFillColor(color);
                    sfText.setPosition(sf::Vector2f(kBarMarginX, barY));
                    window.draw(sfText);
                    barY += lineHeight;
                };
                auto drawWrappedBarLine = [&](const std::string& text, sf::Color color) {
                    for (const std::string& wrapped : wrapToPixelWidth(font, kSidebarCharSize, text, barMaxWidthPx)) {
                        drawBarLine(wrapped, color);
                    }
                };

                switch (combatSession.uiState) {
                    case CombatUiState::AwaitContinue:
                        // Message-pacing queue: one queued log line per
                        // Enter press (see the input dispatch above) --
                        // DQoK's own one-message-at-a-time combat narration.
                        if (combatSession.pendingMessageIndex < combatSession.log.size()) {
                            drawWrappedBarLine(combatSession.log[combatSession.pendingMessageIndex], sf::Color::White);
                        }
                        drawBarLine("(press Enter to continue)", sf::Color(150, 150, 160));
                        break;
                    case CombatUiState::Aiming: {
                        // Milestone 204: live, zero-cost status -- recomputed
                        // every frame the cursor moves, same combatAimLegality
                        // combatConfirmAim itself re-checks at commit time, so
                        // this line and the actual Enter behavior can never
                        // disagree.
                        const int aimedIdx = combatInstanceAtCell(combatSession.aimCursor);
                        if (aimedIdx >= 0) {
                            auto [legal, reason] = combatAimLegality(aimedIdx);
                            if (legal) {
                                drawWrappedBarLine("Attack the " + combatMonsterLabel(aimedIdx) + "? (Enter)",
                                                    sf::Color(230, 220, 160));
                            } else {
                                drawWrappedBarLine(reason, sf::Color(200, 140, 120));
                            }
                        } else {
                            drawWrappedBarLine("Aiming -- arrows to look, Enter to attack, Esc to cancel",
                                                sf::Color(190, 190, 200));
                        }
                        break;
                    }
                    case CombatUiState::PickingTarget: {
                        std::string prompt;
                        switch (combatSession.pickReason) {
                            case TargetPickReason::Spell:
                                prompt = "Cast " + combatSession.pendingSpellResult.spellName + " at which enemy?";
                                break;
                            case TargetPickReason::Webnet:
                                prompt = "Tangle which enemy?";
                                break;
                        }
                        drawWrappedBarLine(prompt, sf::Color(230, 220, 160));
                        drawWrappedBarLine("up/down=select   Enter=choose", sf::Color(150, 150, 160));
                        break;
                    }
                    case CombatUiState::PickingSpell:
                        // The real picker (drawCombatSpellPickerOverlay)
                        // draws on top of this whole frame -- this line is
                        // never actually seen, just here so the switch
                        // covers every CombatUiState value.
                        drawWrappedBarLine("Choose a spell...", sf::Color(230, 220, 160));
                        break;
                    case CombatUiState::PickingItem:
                        // Same "never actually seen" idiom as PickingSpell
                        // above -- drawCombatItemPickerOverlay draws on top.
                        drawWrappedBarLine("Choose an item...", sf::Color(230, 220, 160));
                        break;
                    case CombatUiState::ViewPicking:
                        // Same "never actually seen" idiom as PickingSpell
                        // above -- drawCombatViewPickerOverlay draws on top.
                        drawWrappedBarLine("Choose who to view...", sf::Color(230, 220, 160));
                        break;
                    case CombatUiState::ViewingCard:
                        // Same "never actually seen" idiom -- drawCombatViewCardOverlay draws on top.
                        drawWrappedBarLine("Viewing stats...", sf::Color(230, 220, 160));
                        break;
                    case CombatUiState::Won:
                        drawWrappedBarLine("Victory! Press Enter to continue.", sf::Color(120, 220, 120));
                        break;
                    case CombatUiState::Lost:
                        drawWrappedBarLine("Press Enter to continue.", sf::Color(220, 120, 120));
                        break;
                    case CombatUiState::Fled:
                        drawWrappedBarLine("Press Enter to continue.", sf::Color(220, 190, 120));
                        break;
                    case CombatUiState::Idle:
                        // A live, un-gated status line -- what keeps a
                        // single-line, round-preserving refusal ("Blocked:
                        // cannot walk onto a wall.", "You have nothing to
                        // use.") or a plain "You move north." visible now
                        // that the always-on log block is gone: none of
                        // those set roundJustConcluded, so they never enter
                        // the paced AwaitContinue loop above and must be
                        // shown immediately instead. Also keeps showing the
                        // last paced message's own text for continuity right
                        // after a queue drains.
                        if (!combatSession.log.empty()) {
                            drawWrappedBarLine(combatSession.log.back(), sf::Color(190, 190, 200));
                        }
                        // DQoK's real command-verb bar and its own real
                        // first-letter hotkeys (Milestone 200, confirmed
                        // via References/BattleFrames_extracted) -- QUICK
                        // is dropped outright, still no equivalent
                        // mechanic, and its natural hotkey Q is already
                        // reserved for quit/cancel. Move has no dedicated
                        // key -- direction keys already move directly in
                        // this engine. Movement remaining moves here from
                        // the old always-on sidebar line, unchanged
                        // content.
                        drawWrappedBarLine(
                            "MOVE (arrows/numpad)   VIEW (v)   AIM (a)   USE (u)   CAST (c)   FLEE (f)   DONE (d)",
                            sf::Color(190, 190, 200));
                        drawBarLine("Movement: " + std::to_string(combatSession.movementRemaining) + "/" +
                                        std::to_string(character::movementSquares(state.character)),
                                    sf::Color(160, 200, 230));
                        break;
                }
            } else {
                // Milestone 201: two Gold-Box-bordered cards (via the same
                // drawCardPanel combat's own card uses) in place of the old
                // flat sidebarBg fill -- a stat card sized to its own
                // content, and a separate log card below it filling the
                // rest of the column. Real SSI Gold Box exploration screens
                // use two boxes here (a status box, a message box) rather
                // than one merged panel, so this mirrors that instead of a
                // single full-height wrapper. Same content/data as before in
                // both boxes -- only the chrome and the split are new.
                constexpr float kCardPaddingY = 12.f;
                constexpr float kCardMarginTop = 8.f;
                const float cardX = mapWidth + 8.f;
                const float cardWidth = sidebarWidth - 16.f;
                const bool showIndoors = state.mode == game::Mode::Zone && currentZone;

                float statCardHeight = kCardPaddingY * 2.f + lineHeight * 3.f;
                if (showIndoors) {
                    statCardHeight += lineHeight;
                }
                drawCardPanel(cardX, kCardMarginTop, cardWidth, statCardHeight);

                lineY = kCardMarginTop + kCardPaddingY;
                drawLine(state.character.name + ", level " + std::to_string(state.character.level) + " " +
                              std::string(character::raceInfo(state.character.race).name) + " " +
                              std::string(character::classInfo(state.character.charClass).name),
                          sf::Color::White);
                drawLine("HP: " + std::to_string(state.character.currentHp) + "/" +
                              std::to_string(state.character.maxHp),
                          sf::Color(220, 90, 90));
                drawLine(formatDayTime(state.hoursElapsed), sf::Color(200, 200, 140));
                if (showIndoors) {
                    drawLine("Indoors -- " + currentZone->name(), sf::Color(150, 200, 230));
                }

                constexpr float kLogCardMarginTop = 8.f;
                const float logCardY = kCardMarginTop + statCardHeight + kLogCardMarginTop;
                const float logCardHeight = static_cast<float>(windowH) - logCardY - kCardMarginTop;
                drawCardPanel(cardX, logCardY, cardWidth, logCardHeight);

                lineY = logCardY + kCardPaddingY;
                drawLine("-- Log --", sf::Color(140, 140, 160));
                for (const std::string& entry : log) {
                    for (const std::string& wrapped : wrapToPixelWidth(font, kSidebarCharSize, entry, maxLineWidthPx)) {
                        drawLine(wrapped, sf::Color(190, 190, 200));
                    }
                }
            }

            if (spellbookOpen) {
                window.setView(uiView);
                drawSpellbookOverlay();
            } else if (sheetOpen) {
                window.setView(uiView);
                drawCharacterSheetOverlay();
            }

            if (combatSession.active && combatSession.uiState == CombatUiState::PickingSpell) {
                window.setView(uiView);
                drawCombatSpellPickerOverlay();
            } else if (combatSession.active && combatSession.uiState == CombatUiState::PickingItem) {
                window.setView(uiView);
                drawCombatItemPickerOverlay();
            } else if (combatSession.active && combatSession.uiState == CombatUiState::ViewPicking) {
                window.setView(uiView);
                drawCombatViewPickerOverlay();
            } else if (combatSession.active && combatSession.uiState == CombatUiState::ViewingCard) {
                window.setView(uiView);
                drawCombatViewCardOverlay();
            }

            // Gold-Box town menu (Milestone 206) -- drawn as the base scene
            // for a TOWN_MENU zone, ahead of dialogueSession/shopSession/
            // inventorySession below so a sub-interaction opened from a
            // menu row (talk/shop/bed) paints over it, exactly like it
            // already paints over an ordinary walkable zone's scene.
            if (state.mode == game::Mode::Zone && currentZone != nullptr && isEffectiveMenuTown()) {
                window.setView(uiView);
                drawTownMenuOverlay();
            }

            if (dialogueSession.active) {
                window.setView(uiView);
                drawDialogueOverlay();
            }

            if (shopSession.active) {
                window.setView(uiView);
                drawShopOverlay();
            }

            if (inventorySession.active) {
                window.setView(uiView);
                drawInventoryOverlay();
            }

            if (helpOpen) {
                window.setView(uiView);
                drawHelpOverlay();
            } else if (worldMapOpen) {
                window.setView(uiView);
                drawWorldMapOverlay();
            } else if (zonePlateOpen) {
                window.setView(uiView);
                drawZonePlateOverlay();
            } else if (journalOpen) {
                window.setView(uiView);
                drawJournalOverlay();
            } else if (logSession.active) {
                window.setView(uiView);
                drawLogOverlay();
            } else if (lookSession.active) {
                window.setView(uiView);
                drawLookOverlay();
            }

            if (restSession.active) {
                window.setView(uiView);
                drawRestOverlay();
            }

            if (quitConfirmOpen) {
                window.setView(uiView);
                drawQuitConfirmOverlay();
            }

            window.display();
        } catch (const std::exception& e) {
            std::cerr << "EXCEPTION: " << e.what() << std::endl;
            window.close();
        } catch (...) {
            std::cerr << "UNKNOWN EXCEPTION" << std::endl;
            window.close();
        }
    }

    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        // No save path given -> show the save-slot menu (and, from an
        // empty/overwritten slot, the character creation wizard) instead of
        // requiring one up front. A path is still accepted directly too
        // (unchanged), e.g. for a quick dev launch against a known save.
        return runPhase1(argc >= 2 ? argv[1] : "");
    } catch (const std::exception& e) {
        std::cerr << "FATAL EXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "FATAL UNKNOWN EXCEPTION" << std::endl;
        return 1;
    }
}
