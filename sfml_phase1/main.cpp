// Full-migration Phase 1+2+3 -- see docs/CURRENT_WORK.md and the plans this
// was built from. A real, pixel-space overworld + zone-interior + combat
// screen driven by real save data: proves the rendering/collision/round-
// resolution approach end to end. Deliberately standalone rather than
// reusing game::GameLoop::run() -- see this file's CMakeLists.txt comment
// for why. Loads a save file read-only via game::SaveGame -- never writes
// back, never touches the real ansalon_rpg target's code path.
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

#include "character/Alignment.h"
#include "character/CharClass.h"
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
#include "timeline/Timeline.h"
#include "timeline/TimelineLoader.h"
#include "world/OverworldGrid.h"
#include "world/Terrain.h"
#include "world/World.h"
#include "world/WorldLoader.h"
#include "world/Zone.h"
#include "world/ZoneCatalog.h"
#include "world/ZoneTile.h"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <deque>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::size_t kMaxLogLines = 14;
constexpr unsigned kSidebarCharSize = 16;
// Rough monospace advance width for kSidebarCharSize in Consolas -- used
// only to wrap placeholder log text to the sidebar's pixel width, not for
// precise layout. Recalibrate if the font or character size changes.
constexpr float kSidebarCharWidth = 9.5f;
// Placeholder pixel-per-tile scale for zone interiors -- there's no real
// reference image for these like the overworld has, so this is a plain
// colored-tile placeholder, not art. Zones are capped at 44x16
// (docs/ZONE_NOTES.md), so at this scale every zone fits the map viewport
// with no scrolling needed.
constexpr float kZoneTilePx = 20.f;
// Combat's tactical grid -- same 15x9 dimensions render::MapRenderer::
// kCombatGridWidth/Height use for the console version (GameLoop::runCombat
// centers the player and spreads monster instances symmetrically around
// width/2, so width MUST stay odd). Redefined locally rather than including
// render/MapRenderer.h -- that header pulls in render::Console, which isn't
// linked into this target and shouldn't become a dependency just for two
// ints (this file's presentation code already stays independent of
// render::, same boundary Phase 1/2 established).
constexpr int kCombatGridWidth = 15;
constexpr int kCombatGridHeight = 9;
constexpr float kCombatTilePx = 56.f;

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
// reasoning as DialogueSpeech above. hasQuest/hasBoat/hasRecruit only
// record *that* this candidate would offer one of those (in the console
// build) so dialogueStartTalk can print a single placeholder line --
// not the full payload (quest id, boat destination, companion id) the
// console version's GameLoop::TalkCandidate carries, since none of those
// flows are wired up yet.
struct DialogueCandidate {
    std::string id;
    std::string name;
    DialogueSpeech speech;
    std::string dialogueAfter;
    std::string dialogueBefore;
    std::string grantsItemId;
    std::string grantsItemName;
    bool hasQuest = false;
    bool hasBoat = false;
    bool hasRecruit = false;
};

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

// Greedy word-wrap so sidebar log lines don't run off the panel -- the
// sidebar is placeholder-styled (per the approved plan), but unwrapped
// overflowing text would undermine even that, so this one bit of layout
// care is worth it.
std::vector<std::string> wrapToWidth(const std::string& text, std::size_t maxChars) {
    std::vector<std::string> lines;
    std::istringstream words(text);
    std::string word;
    std::string current;
    while (words >> word) {
        const std::string candidate = current.empty() ? word : current + " " + word;
        if (candidate.size() > maxChars && !current.empty()) {
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
// CombatSession::pickReason/TargetPickReason below), same single mechanism
// GameLoop::pickTarget already is for attack, spell, and Webnet targeting.
enum class CombatUiState { AwaitContinue, Idle, PickingTarget, PickingSpell, PickingItem, Won, Lost, Fled };

// Which of the three real reasons PickingTarget is ever open for -- an
// ordinary melee attack, a spell that needs a target, or Webnet's own
// "tangle which enemy?" (character::useWebnet succeeded, still needs to
// know who) -- since combatConfirmTarget and the footer text both need to
// know which action to actually resolve once a target is chosen. Replaces
// what used to be a plain bool (attack vs. spell) now that item use adds a
// third case.
enum class TargetPickReason { Attack, Spell, Webnet };

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
    std::vector<std::string> log;
    CombatUiState uiState = CombatUiState::AwaitContinue;

    // Target-picker state -- only meaningful while uiState == PickingTarget.
    std::vector<int> pickCandidates;
    int pickSelected = 0;
    // Whether the player's side acts before the monsters this round
    // (combat::playerActsFirst(), rolled once at the top of the round and
    // remembered here so it's still known once the picker -- which may
    // span several frames -- finally confirms a target).
    bool pendingGoFirst = true;

    int roundNumber = 1;

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
    // character::useWebnet call instead (see combatCommitItemChoice);
    // Attack means an ordinary melee attack, same as before this feature
    // existed.
    TargetPickReason pickReason = TargetPickReason::Attack;
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
};

// Dialogue's own UI states -- non-blocking port of GameLoop::talkTo's
// conversation loop (see docs/CURRENT_WORK.md's scope writeup; quest/boat/
// recruit choosers stay deferred, each getting a single placeholder log
// line instead). PickingCandidate mirrors GameLoop::pickAndTalk's "more
// than one candidate here" picker; Greeting/TopicText/TopicPicker mirror
// talkTo's own greeting-then-topic-menu flow. AskInput/AskResponse are the
// free-text "ask about something else..." flow -- AskInput is the typing
// box, AskResponse shows the resulting response (and, when today's ask
// limit is reached or extended, a queued 2nd message).
enum class DialogueUiState { PickingCandidate, Greeting, TopicText, TopicPicker, AskInput, AskResponse };

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
    std::string displaySpeaker;            // usually current.name; overridden by askLimitLocked's speaker
    std::vector<std::string> topicLabels;  // topics [+ "Ask about..."] + "Nothing, thanks"
    int topicSelected = 0;
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

int runPhase1(const std::string& savePath) {
    const unsigned windowW = 1280;
    const unsigned windowH = 800;
    const float sidebarWidth = 320.f;
    const float mapWidth = static_cast<float>(windowW) - sidebarWidth;

    std::cout << "step 0: starting, save = " << savePath << std::endl;

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(windowW, windowH)),
                             "Ansalon SFML Phase 1+2+3 -- Real Overworld + Zones + Combat (WIP)");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("C:/Windows/Fonts/consola.ttf")) {
        std::cerr << "Failed to load placeholder UI font (C:/Windows/Fonts/consola.ttf) -- "
                     "this build uses a system font as a stand-in until this project has its own.\n";
        return 1;
    }

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

    if (!drawLoadingScreen("Loading timeline...")) return 0;
    timeline::Timeline timeline;
    timeline::TimelineLoader::loadFromFile("data/timeline.txt", timeline);
    std::cout << "step 2d: timeline loaded" << std::endl;

    if (!drawLoadingScreen("Loading your character...")) return 0;
    game::GameState state = game::SaveGame::load(savePath);
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
    // Overworld, state.zoneX/zoneY for Zone) -- never written back to
    // savePath, but mutated in memory exactly like game::GameLoop does.
    const world::Zone* currentZone = nullptr;
    if (state.mode == game::Mode::Zone) {
        currentZone = zones.getZone(state.currentZoneId);
        if (!currentZone) {
            std::cerr << "Warning: save's zone '" << state.currentZoneId
                      << "' not found -- falling back to overworld.\n";
            state.mode = game::Mode::Overworld;
        }
    }

    sf::View mapView(sf::Vector2f(0.f, 0.f), sf::Vector2f(mapWidth, static_cast<float>(windowH)));
    mapView.setViewport(sf::FloatRect({0.f, 0.f}, {mapWidth / static_cast<float>(windowW), 1.f}));

    const sf::View uiView = window.getDefaultView();

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

    sf::RectangleShape sidebarBg(sf::Vector2f(sidebarWidth, static_cast<float>(windowH)));
    sidebarBg.setPosition(sf::Vector2f(mapWidth, 0.f));
    sidebarBg.setFillColor(sf::Color(20, 20, 28));

    // Combat grid + marker shapes -- same "declare once, mutate per cell/
    // entity" convention as zoneTileShape/locationMarker above. No sprite
    // art for combat either (same reasoning as zone interiors) -- flat
    // color plus a letter label distinguishes each monster/companion.
    sf::RectangleShape combatTileShape(sf::Vector2f(kCombatTilePx - 2.f, kCombatTilePx - 2.f));
    combatTileShape.setFillColor(sf::Color(70, 65, 55));
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
    pushLog("Movement, Enter (zones/combat), Flee (combat), Talk, and C (character sheet) work. Other keys are placeholders for now.");

    // --- Dialogue (core conversation): all state is DialogueSession above;
    // every lambda below is a non-blocking port of the matching piece of
    // GameLoop::handleTalk/talkTo (src/game/GameLoop.cpp:828-1306) -- see
    // this file's top-of-file comment and docs/CURRENT_WORK.md for the
    // scope this phase ports (greeting/again/aftermath/anticipation/
    // conditional-greeting resolution, the topic picker, GRANTS_ITEM) vs.
    // defers (free-text ask, quest turn-in, boat voyage, companion
    // recruit -- each of the latter three gets a single placeholder log
    // line via dialogueStartTalk's hasQuest/hasBoat/hasRecruit checks
    // instead of silently doing nothing).
    DialogueSession dialogueSession;
    ShopSession shopSession;
    InventorySession inventorySession;
    LogSession logSession;

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
                candidate.hasQuest = currentZone->questAt(state.zoneX, state.zoneY) != nullptr;
                candidate.hasBoat = currentZone->boatAt(state.zoneX, state.zoneY) != nullptr;
                candidate.hasRecruit = !poi->recruitCompanionId.empty();
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

    // Mirrors GameLoop::talkTo's greeting-resolution precedence exactly
    // (GameLoop.cpp:1002-1046), including the askLimitLocked override, minus
    // boat/quest/recruit's real handling (a single placeholder log line
    // each instead).
    auto dialogueStartTalk = [&](const DialogueCandidate& candidate) {
        dialogueSession.current = candidate;
        dialogueSession.displaySpeaker = candidate.name;
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
        dialogueSession.bodyText = text;
        state.metCharacters.insert(candidate.id);

        if (!candidate.grantsItemId.empty() &&
            character::findQuestItemIndex(state.character, candidate.grantsItemId) < 0) {
            state.character.inventory.push_back(character::InventoryItem{
                character::ItemKind::QuestItem, character::ArmorId::None, "", 0, 0, 0, candidate.grantsItemId,
                candidate.grantsItemName});
            pushLog("You've picked up " + candidate.grantsItemName + ".");
        }
        if (candidate.hasQuest) pushLog("(Quest content at this NPC isn't wired up in this build yet.)");
        if (candidate.hasBoat) pushLog("(Boat voyages aren't wired up in this build yet.)");
        if (candidate.hasRecruit) pushLog("(Recruiting companions isn't wired up in this build yet.)");

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
    // (GameLoop.cpp:1570-1596) verbatim, including its two pushLog
    // messages. shopLockAt (SHOP_LOCKED, docs/ZONE_NOTES.md) can't be
    // resolved here -- this build tracks no quest state at all yet, same
    // gap dialogueStartTalk's hasQuest/hasBoat/hasRecruit checks already
    // flag -- so a locked shop gets its own honest placeholder instead of
    // silently always-locking (reads as "no shop here") or silently
    // unlocking (lets the player buy before earning it).
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
        if (currentZone->shopLockAt(state.zoneX, state.zoneY) != nullptr) {
            pushLog("(This shop is quest-locked -- that isn't tracked in this build yet.)");
            return;
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
        dialogueSession.bodyText =
            dialogueSession.current.speech.topics[static_cast<size_t>(dialogueSession.topicSelected)].second;
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
        dialogueSession.bodyText = dialogueSession.askMessages.front();
        dialogueSession.uiState = DialogueUiState::AskResponse;
    };

    // Enter (or any key, per the console's "press any key to continue") on
    // a plain dialogue box. Greeting goes to the topic menu if there's
    // anything to pick (a topic or the ask-anything option), else ends the
    // conversation; TopicText always returns to the topic menu (mirroring
    // talkTo's own topic loop); AskResponse advances to the next queued
    // message if there is one, else ends the conversation or returns to a
    // freshly-rebuilt topic menu, per dialogueSubmitAsk's bookkeeping.
    auto dialogueContinue = [&]() {
        if (dialogueSession.uiState == DialogueUiState::Greeting) {
            if (!dialogueSession.current.speech.topics.empty() || dialogueSession.askAnythingIndex >= 0) {
                dialogueSession.uiState = DialogueUiState::TopicPicker;
            } else {
                dialogueEnd();
            }
        } else if (dialogueSession.uiState == DialogueUiState::TopicText) {
            dialogueSession.uiState = DialogueUiState::TopicPicker;
        } else if (dialogueSession.uiState == DialogueUiState::AskResponse) {
            ++dialogueSession.askMessageIndex;
            if (dialogueSession.askMessageIndex < static_cast<int>(dialogueSession.askMessages.size())) {
                dialogueSession.bodyText =
                    dialogueSession.askMessages[static_cast<size_t>(dialogueSession.askMessageIndex)];
            } else if (dialogueSession.askQueueEndsConversation) {
                dialogueEnd();
            } else {
                rebuildTopicLabels(dialogueSession.current);
                dialogueSession.uiState = DialogueUiState::TopicPicker;
            }
        }
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
    // fixed 1280x800, so a high-level caster's full list (a 20th-level Mage
    // has 9 spell levels' worth) needs the same real scroll treatment
    // logSession already has, unlike the console original. scrollOffset
    // resets to 0 each time the spellbook is (re)opened.
    bool spellbookOpen = false;
    int spellbookScrollOffset = 0;

    // Help ('/', the pixel-space bind for the console's '?'), World Map
    // ('o'), and Journal ('g') -- same transient-full-window-overlay
    // reasoning as sheetOpen above, dismissed by any key. Journal is real
    // (not a stub): it ports GameLoop::showJournal's own logic, but this
    // build never populates game::GameState::quests yet (quest offer/
    // accept dialogue is still deferred, see DialogueSession's own
    // hasQuest handling above), so its body says so explicitly rather than
    // silently rendering the console's "(no quests yet)" empty state,
    // which would misleadingly imply a working-but-empty quest log instead
    // of a not-yet-wired-up one.
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
        combatSession.log.push_back("You are struck down... and wake up back in " + refugeName +
                                     ", battered but alive.");
        if (refuge != nullptr) {
            state.x = refuge->x;
            state.y = refuge->y;
        }
        pushLog("You were knocked out by the " + cause + " and woke up back in " + refugeName + ".");
        combatSession.uiState = CombatUiState::Lost;
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

    // Awards steel/XP (and applies any resulting level-up) the moment one
    // instance's HP reaches 0, then Sivak's real death-burst (Dragonlance
    // Adventures p.75) if this monster has one. Returns true if the burst
    // just knocked the player out, so callers stop swinging immediately --
    // mirrors GameLoop::handleInstanceDeath exactly.
    auto combatHandleInstanceDeath = [&](int idx) -> bool {
        state.monsterKills[combatSession.monster.id] += 1;
        int steel = std::max(0, character::roll(combatSession.monster.steelDiceCount,
                                                  combatSession.monster.steelDiceSides) +
                                     combatSession.monster.steelFlatBonus);
        state.character.steelPieces += steel;
        std::string name = combatMonsterLabel(idx);
        if (combatSession.monster.id == "baaz") {
            combatSession.log.push_back("The " + name + " falls and its body crumbles to stone! You find " +
                                         std::to_string(steel) + " steel among the rubble.");
        } else {
            combatSession.log.push_back("The " + name + " falls! You find " + std::to_string(steel) + " steel.");
        }
        if (combatSession.monster.xpValue > 0) {
            state.character.experience += combatSession.monster.xpValue;
            combatSession.log.push_back("You gain " + std::to_string(combatSession.monster.xpValue) +
                                         " experience.");
            character::applyPendingLevelUps(state.character, combatSession.log);
        }
        if (combatSession.monster.burstsIntoFlameOnDeath) {
            int burstDamage = character::roll(2, 4);
            state.character.currentHp -= burstDamage;
            combatSession.log.push_back("As it falls, the " + name + " bursts into flame! You take " +
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
            bool leavingReach = combat::isAdjacent(combatSession.playerPos, combatSession.instancePositions[i]) &&
                                 !combat::isAdjacent(destination, combatSession.instancePositions[i]);
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
                combatSession.log.push_back("As you pull back, the " + name + " gets a free strike! It hits you for " +
                                             std::to_string(outcome.damage) + ".");
            } else {
                combatSession.log.push_back("The " + name + " lunges as you pull back, but misses.");
            }
        }
    };

    // Closes out a round: victory (every instance down), knockout (player
    // at 0 HP), or just advances to the next round -- mirrors the tail end
    // of GameLoop::runCombat's own `for(;;)` body.
    auto combatWrapUpRound = [&]() {
        if (combatAliveCount() == 0) {
            combatSession.log.push_back(combatSession.useLetters
                                             ? ("The " + pluralMonsterName(combatSession.monster.name) + " are defeated!")
                                             : ("You defeated the " + combatSession.monster.name + "."));
            pushLog(combatSession.useLetters
                        ? ("You defeated the " + pluralMonsterName(combatSession.monster.name) + ".")
                        : ("You defeated the " + combatSession.monster.name + "."));
            combatSession.uiState = CombatUiState::Won;
            return;
        }
        if (combatCheckPlayerDown(combatSession.monster.name)) return;
        ++combatSession.roundNumber;
        combatSession.uiState = CombatUiState::Idle;
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
            character::Character& companion = state.companions[ci].character;
            combat::GridPos& companionPos = combatSession.companionPositions[ci];
            int targetIndex = -1;
            for (size_t i = 0; i < combatSession.instances.size(); ++i) {
                if (combatSession.instances[i].hp > 0 &&
                    combat::isAdjacent(companionPos, combatSession.instancePositions[i])) {
                    targetIndex = static_cast<int>(i);
                    break;
                }
            }
            if (targetIndex < 0) {
                combat::GridPos nearest{};
                int nearestDist = -1;
                for (size_t i = 0; i < combatSession.instances.size(); ++i) {
                    if (combatSession.instances[i].hp <= 0) continue;
                    int dist = combat::chebyshevDistance(companionPos, combatSession.instancePositions[i]);
                    if (nearestDist < 0 || dist < nearestDist) {
                        nearestDist = dist;
                        nearest = combatSession.instancePositions[i];
                    }
                }
                if (nearestDist < 0) continue;
                std::vector<combat::GridPos> blocked{combatSession.playerPos};
                for (size_t i = 0; i < combatSession.instances.size(); ++i) {
                    if (combatSession.instances[i].hp > 0) blocked.push_back(combatSession.instancePositions[i]);
                }
                for (size_t oi = 0; oi < state.companions.size(); ++oi) {
                    if (oi != ci && combatCompanionAlive(oi)) blocked.push_back(combatSession.companionPositions[oi]);
                }
                combat::GridPos next =
                    combat::stepToward(companionPos, nearest, kCombatGridWidth, kCombatGridHeight, blocked);
                if (next.x != companionPos.x || next.y != companionPos.y) companionPos = next;
                continue;
            }
            // Identity used by combatBackstabBonus's firstAttackerId
            // tracking -- 0 is the player, so a companion is ci+1. Shared by
            // the sweep block below and the ordinary attack loop after it.
            int attackerId = static_cast<int>(ci) + 1;
            // Fighter-type companion sweep -- same rule and same
            // combatAdjacentWeakInstances helper as the player's own sweep
            // in combatBeginPlayerAttack. Ends this companion's turn
            // (continue) rather than falling into the single-target loop
            // below. Direct port of GameLoop.cpp:2347-2380.
            if (character::classGroupFor(companion.charClass) == character::ClassGroup::Warrior) {
                std::vector<int> weakTargets = combatAdjacentWeakInstances(companionPos);
                if (weakTargets.size() >= 2) {
                    combatSession.log.push_back(companion.name + " sweeps through the " +
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
                            combatSession.log.push_back(std::string(backstabMultiplier > 1 ? "Backstab! " : "") +
                                                         companion.name + " hits the " + targetName + " for " +
                                                         std::to_string(outcome.damage) + ".");
                            if (combatSession.instances[static_cast<size_t>(idx)].hp <= 0) {
                                if (combatHandleInstanceDeath(idx)) return;
                            }
                        } else {
                            combatSession.log.push_back(companion.name + " misses the " + targetName + ".");
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
                    combatSession.log.push_back(std::string(backstabMultiplier > 1 ? "Backstab! " : "") +
                                                 companion.name + " hits the " + targetName + " for " +
                                                 std::to_string(outcome.damage) + ".");
                    if (combatSession.instances[static_cast<size_t>(targetIndex)].hp <= 0) {
                        if (combatHandleInstanceDeath(targetIndex)) return;
                    }
                } else {
                    combatSession.log.push_back(companion.name + " misses the " + targetName + ".");
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
            std::string name = combatMonsterLabel(static_cast<int>(i));
            // Web/Hold-style block (character::SpellEffect::
            // BlockMonsterAttacks) -- checked before even Magic Missile/
            // breath weapon, same ordering as GameLoop.cpp:2438-2446.
            if (combatSession.incapacitatedRestOfFight[i]) {
                combatSession.log.push_back("The " + name + " is unable to act!");
                continue;
            }
            if (combatSession.blockedAttacksRemaining[i] > 0) {
                --combatSession.blockedAttacksRemaining[i];
                combatSession.log.push_back("The " + name + " can't bring itself to attack!");
                continue;
            }
            const combat::Monster& monster = combatSession.monster;
            // The Brooch's globe wards the player only (see the melee check
            // below) but Magic Missile/breath weapon always target the
            // player directly (Milestone 117 never taught either to pick a
            // companion instead), so a plain globeActive check is enough
            // here -- mirrors GameLoop.cpp:2463/2482.
            if (monster.castsMagicMissile && character::roll(1, 100) <= monster.magicMissileChancePercent) {
                if (combatSession.globeActive) {
                    combatSession.log.push_back("The globe of invulnerability absorbs the " + name +
                                                 "'s Magic Missile!");
                    continue;
                }
                int missileDamage = (character::roll(1, 4) + 1) + (character::roll(1, 4) + 1);
                state.character.currentHp -= missileDamage;
                combatSession.log.push_back("The " + name + " casts Magic Missile! It strikes you for " +
                                             std::to_string(missileDamage) + " -- no saving throw.");
                continue;
            }
            if (monster.hasBreathWeapon && character::roll(1, 100) <= monster.breathWeaponChancePercent) {
                if (combatSession.globeActive) {
                    combatSession.log.push_back("The globe of invulnerability absorbs the " + name +
                                                 "'s breath weapon!");
                    continue;
                }
                if (combat::rollSavingThrow(state.character, character::SaveCategory::BreathWeapon)) {
                    state.character.currentHp -= 10;
                    combatSession.log.push_back("The " + name + " breathes a noxious cloud! You resist -- 10 damage.");
                } else {
                    state.character.currentHp -= 20;
                    combatSession.playerThac0Bonus -= 4;
                    combatSession.log.push_back("The " + name +
                                                 " breathes a noxious cloud! It burns you for 20 damage and blinds you.");
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
                if (combat::isAdjacent(combatSession.instancePositions[i], party[pi].pos)) adjacentTargets.push_back(pi);
            }
            if (adjacentTargets.empty()) {
                size_t nearest = 0;
                int nearestDist = -1;
                for (size_t pi = 0; pi < party.size(); ++pi) {
                    int dist = combat::chebyshevDistance(combatSession.instancePositions[i], party[pi].pos);
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
                combat::GridPos next = combat::stepToward(combatSession.instancePositions[i], party[nearest].pos,
                                                            kCombatGridWidth, kCombatGridHeight, blocked);
                if (next.x != combatSession.instancePositions[i].x || next.y != combatSession.instancePositions[i].y) {
                    combatSession.instancePositions[i] = next;
                    combatSession.log.push_back("The " + name + " closes in.");
                }
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
                combatSession.log.push_back("The globe of invulnerability absorbs the blow from the " + name + "!");
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
                combatSession.log.push_back("The " + name + " hits " + targetName + " for " +
                                             std::to_string(outcome.damage) + ".");
                if (monster.poisonOnHit) {
                    if (combat::rollSavingThrow(*target.character, character::SaveCategory::ParalyzationPoisonDeath)) {
                        combatSession.log.push_back(target.isPlayer ? "You resist the poison."
                                                                     : targetName + " resists the poison.");
                    } else {
                        combatSession.log.push_back("The poison overwhelms " + targetName + "!");
                        target.character->currentHp = 0;
                    }
                }
                if (!target.isPlayer && target.character->currentHp <= 0) {
                    target.character->currentHp = 0;
                    combatSession.log.push_back(targetName + " is knocked out!");
                }
            } else {
                combatSession.log.push_back("The " + name + " misses " + targetName + ".");
            }
        }
    };

    // PHB p.124: rolls which side acts first, and if the monsters do, runs
    // their turn immediately (before the player has even chosen an
    // action) -- mirrors GameLoop::runCombat's own dispatch. Returns false
    // if that already ended the fight, so the caller (about to resolve a
    // move/attack) knows to stop.
    auto combatRollGoFirstAndMaybeActMonsters = [&]() -> bool {
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
            if (outcome.hit) {
                combatSession.instances[static_cast<size_t>(targetIndex)].hp -= outcome.damage;
                combatSession.log.push_back(std::string(backstabMultiplier > 1 ? "Backstab! " : "") + "You hit the " +
                                             targetName + " for " + std::to_string(outcome.damage) + ".");
                if (combatSession.instances[static_cast<size_t>(targetIndex)].hp <= 0) {
                    if (combatHandleInstanceDeath(targetIndex)) return;
                }
            } else {
                combatSession.log.push_back("You miss the " + targetName + ".");
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
                combatSession.log.push_back("Your " + result.spellName + " strikes the " + targetName + " for " +
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
                combatSession.log.push_back("Your " + result.spellName + " engulfs the " + names + " for " +
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
                combatSession.log.push_back("You cast " + result.spellName + " and heal " +
                                             std::to_string(healed) + " hit points.");
                break;
            }
            case character::SpellEffect::BlockMonsterAttacks:
                if (result.amount == character::kBlockRestOfFight) {
                    combatSession.incapacitatedRestOfFight[static_cast<size_t>(targetIndex)] = true;
                } else {
                    combatSession.blockedAttacksRemaining[static_cast<size_t>(targetIndex)] += result.amount;
                }
                combatSession.log.push_back("You cast " + result.spellName + " on the " + targetName + "!");
                break;
            case character::SpellEffect::BuffPlayerThac0:
                combatSession.playerThac0Bonus += result.amount;
                combatSession.log.push_back("You cast " + result.spellName + ".");
                break;
            case character::SpellEffect::BuffPlayerDamage:
                combatSession.playerDamageBonus += result.amount;
                combatSession.log.push_back("You cast " + result.spellName + ".");
                break;
            case character::SpellEffect::BuffPlayerAc:
                combatSession.playerAcBonus += result.amount;
                combatSession.log.push_back("You cast " + result.spellName + ".");
                break;
            case character::SpellEffect::DebuffMonsterThac0:
                combatSession.monsterThac0Penalty[static_cast<size_t>(targetIndex)] += result.amount;
                combatSession.log.push_back("You cast " + result.spellName + " on the " + targetName + "!");
                break;
            case character::SpellEffect::DebuffMonsterDamage:
                combatSession.monsterDamagePenalty[static_cast<size_t>(targetIndex)] += result.amount;
                combatSession.log.push_back("You cast " + result.spellName + " on the " + targetName + "!");
                break;
            case character::SpellEffect::BuffPlayerAndDebuffMonsterThac0:
                combatSession.playerThac0Bonus += result.amount;
                combatSession.monsterThac0Penalty[static_cast<size_t>(targetIndex)] += result.amount;
                combatSession.log.push_back("You cast " + result.spellName + ".");
                break;
            case character::SpellEffect::HastePlayer:
                combatSession.hasteAttackMultiplier = result.amount;
                combatSession.log.push_back("You cast " + result.spellName + "! Your attacks quicken.");
                break;
            case character::SpellEffect::DebuffMonsterThac0AndAc:
                combatSession.monsterThac0Penalty[static_cast<size_t>(targetIndex)] += result.amount;
                combatSession.monsterAcPenalty[static_cast<size_t>(targetIndex)] += result.amount;
                combatSession.log.push_back("You cast " + result.spellName + " on the " + targetName + "!");
                break;
            case character::SpellEffect::InstantDefeat:
                combatSession.log.push_back("Your " + result.spellName + " destroys the " + targetName +
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
        if (!combatRollGoFirstAndMaybeActMonsters()) return;
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
        // matching GameLoop.cpp:1935's own anyAlive filter for casting.
        std::vector<int> candidates;
        for (size_t i = 0; i < combatSession.instances.size(); ++i) {
            if (combatSession.instances[i].hp > 0) candidates.push_back(static_cast<int>(i));
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
            combatSession.log.push_back("You have no spell to cast.");
            return;
        }
        if (!character::hasMemorizedSpellsAvailable(state.character, state.hoursElapsed / 24)) {
            combatSession.log.push_back("You have no spells remaining today.");
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
        if (!combatRollGoFirstAndMaybeActMonsters()) return;
        const long long today = state.hoursElapsed / 24;
        switch (kind) {
            case character::CombatItemKind::Potion: {
                character::PurchaseResult result =
                    character::drinkPotion(state.character, character::firstPotionIndex(state.character));
                combatSession.log.push_back(result.message);
                combatFinishPlayerAction(combatSession.pendingGoFirst);
                break;
            }
            case character::CombatItemKind::Webnet: {
                character::PurchaseResult result =
                    character::useWebnet(state.character, character::firstWebnetIndex(state.character));
                combatSession.log.push_back(result.message);
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
                combatSession.log.push_back(result.message);
                if (result.success) combatSession.globeActive = true;
                combatFinishPlayerAction(combatSession.pendingGoFirst);
                break;
            }
            case character::CombatItemKind::StaffCure: {
                character::PurchaseResult result = character::useStaffCure(state.character, today);
                combatSession.log.push_back(result.message);
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
            combatSession.log.push_back("You have nothing to use.");
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

    // Enter pressed while Idle: commit to attacking this round. Melee-locked
    // (must be adjacent) unless wielding the Light Crossbow, which can hit
    // anyone on the grid but is disabled outright the instant an enemy
    // closes to melee range (DQoK.pdf's own manual) -- mirrors
    // GameLoop::playerAttacks. 0 eligible targets or 2+ both still consume
    // the round (a wasted swing costs your action just like a real one);
    // exactly 1 resolves immediately, 2+ opens the in-frame picker.
    auto combatBeginPlayerAttack = [&]() {
        if (!combatRollGoFirstAndMaybeActMonsters()) return;
        bool hasRangedWeapon = state.character.weaponName == character::kLightCrossbowName;
        bool adjacentToAny = false;
        for (size_t i = 0; i < combatSession.instances.size(); ++i) {
            if (combatSession.instances[i].hp > 0 &&
                combat::isAdjacent(combatSession.playerPos, combatSession.instancePositions[i])) {
                adjacentToAny = true;
                break;
            }
        }
        if (hasRangedWeapon && adjacentToAny) {
            combatSession.log.push_back("An enemy is too close to fire your crossbow!");
            combatFinishPlayerAction(combatSession.pendingGoFirst);
            return;
        }
        // Fighter-type sweep bypasses the picker and meleeAttacksThisRound's
        // progression entirely -- one swing per adjacent weak instance, no
        // to-hit/damage bonus (DQoK's own wording gives none; sweeping is an
        // action-economy ability only). See combatAdjacentWeakInstances's
        // doc comment. Direct port of GameLoop.cpp:2214-2245.
        if (character::classGroupFor(state.character.charClass) == character::ClassGroup::Warrior) {
            std::vector<int> weakTargets = combatAdjacentWeakInstances(combatSession.playerPos);
            if (weakTargets.size() >= 2) {
                combatSession.log.push_back("You sweep through the " +
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
                    if (outcome.hit) {
                        combatSession.instances[static_cast<size_t>(targetIndex)].hp -= outcome.damage;
                        combatSession.log.push_back(std::string(backstabMultiplier > 1 ? "Backstab! " : "") +
                                                     "You hit the " + targetName + " for " +
                                                     std::to_string(outcome.damage) + ".");
                        if (combatSession.instances[static_cast<size_t>(targetIndex)].hp <= 0) {
                            // A death-burst knockout mid-sweep -- combatFinishPlayerAction is still called
                            // unconditionally below (it and combatCompanionActs both no-op safely once
                            // uiState is Lost), same pattern candidates.size()==1's own call site uses.
                            if (combatHandleInstanceDeath(targetIndex)) break;
                        }
                    } else {
                        combatSession.log.push_back("You miss the " + targetName + ".");
                    }
                }
                combatFinishPlayerAction(combatSession.pendingGoFirst);
                return;
            }
        }
        std::vector<int> candidates;
        for (size_t i = 0; i < combatSession.instances.size(); ++i) {
            if (combatSession.instances[i].hp <= 0) continue;
            if (!hasRangedWeapon && !combat::isAdjacent(combatSession.playerPos, combatSession.instancePositions[i])) continue;
            candidates.push_back(static_cast<int>(i));
        }
        if (candidates.empty()) {
            combatSession.log.push_back("You're too far away to attack.");
            combatFinishPlayerAction(combatSession.pendingGoFirst);
            return;
        }
        if (candidates.size() == 1) {
            combatResolveAttackAgainstTarget(candidates.front());
            combatFinishPlayerAction(combatSession.pendingGoFirst);
            return;
        }
        combatSession.pickReason = TargetPickReason::Attack;
        combatSession.pickCandidates = candidates;
        combatSession.pickSelected = 0;
        combatSession.uiState = CombatUiState::PickingTarget;
    };

    // Shared by combatBeginPlayerMove below, called both before and after
    // monsters get a chance to act -- a monster that wasn't yet adjacent
    // when the move was declared can close in during its own turn and land
    // exactly on the cell the player is mid-step into, since nothing else
    // re-checks that cell once combatRollGoFirstAndMaybeActMonsters runs.
    // Left uncaught, that produces two combatants sharing one GridPos,
    // which combat::isAdjacent's own "never adjacent to itself" rule then
    // masks as both being permanently "too far away" to attack each other.
    auto combatCellOccupied = [&](combat::GridPos cell) {
        for (size_t i = 0; i < combatSession.instances.size(); ++i) {
            if (combatSession.instances[i].hp > 0 && combatSession.instancePositions[i].x == cell.x &&
                combatSession.instancePositions[i].y == cell.y) {
                return true;
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

    // A direction key pressed while Idle: commit to moving this round.
    // Validated up front (bounds/occupancy) before it ever costs a round,
    // same as the console version; a valid move still triggers opportunity
    // attacks from anyone being left adjacent.
    auto combatBeginPlayerMove = [&](int dx, int dy) {
        combat::GridPos destination{combatSession.playerPos.x + dx, combatSession.playerPos.y + dy};
        if (destination.x < 0 || destination.x >= kCombatGridWidth || destination.y < 0 ||
            destination.y >= kCombatGridHeight) {
            combatSession.log.push_back("You can't move that way.");
            return;
        }
        if (combatCellOccupied(destination)) {
            combatSession.log.push_back("Something's in the way.");
            return;
        }
        if (!combatRollGoFirstAndMaybeActMonsters()) return;
        combatTriggerOpportunityAttacks(destination);
        if (combatCheckPlayerDown(combatSession.monster.name)) return;
        // Re-check: whatever just acted above may have moved into
        // `destination` itself (see combatCellOccupied's own comment).
        // The opportunity attack and the monsters' turn already happened
        // either way -- only the player's own step is what's cancelled.
        if (combatCellOccupied(destination)) {
            combatSession.log.push_back("The way is blocked now.");
            combatFinishPlayerAction(combatSession.pendingGoFirst);
            return;
        }
        std::string dirLabel = destination.y < combatSession.playerPos.y   ? "north"
                                : destination.y > combatSession.playerPos.y ? "south"
                                : destination.x < combatSession.playerPos.x ? "west"
                                                                             : "east";
        combatSession.playerPos = destination;
        combatSession.log.push_back("You move " + dirLabel + ".");
        combatFinishPlayerAction(combatSession.pendingGoFirst);
    };

    // F pressed while Idle: an unconditional escape -- unlike attack/move,
    // this never goes through the initiative dispatch above (mirrors
    // GameLoop::runCombat, where Flee is checked and handled before
    // anything else in its round loop), so no monster gets a free action.
    auto combatBeginFlee = [&]() {
        combatSession.log.push_back("You break off and retreat.");
        pushLog("You fled from the " + combatSession.monster.name + ".");
        combatSession.uiState = CombatUiState::Fled;
    };

    // Enter pressed while PickingTarget: commit to the highlighted
    // candidate and resolve the rest of the round exactly as the
    // immediate (0/1-candidate) path in combatBeginPlayerAttack does.
    auto combatConfirmTarget = [&]() {
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
            case TargetPickReason::Attack:
                combatResolveAttackAgainstTarget(target);
                break;
        }
        if (combatSession.uiState == CombatUiState::Lost) return;
        combatFinishPlayerAction(combatSession.pendingGoFirst);
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
            combatSession.log.push_back("Your Frostreaver's edge bites keener than steel, sharpened by the glacier's own cold.");
        }
        if (state.character.charClass == character::ClassId::Fighter && state.character.specializedWeapon) {
            combatSession.playerThac0Bonus += character::kWeaponSpecializationToHitBonus;
            combatSession.playerDamageBonus += character::kWeaponSpecializationDamageBonus;
        }
        if (combatSession.useLetters) {
            combatSession.log.push_back(std::to_string(groupSize) + " " + pluralMonsterName(monster.name) +
                                         " appear! " + monster.description);
            pushLog(std::to_string(groupSize) + " " + pluralMonsterName(monster.name) + " appear!");
        } else {
            combatSession.log.push_back("A " + monster.name + " appears! " + monster.description);
            pushLog("A " + monster.name + " appears!");
        }
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
    constexpr unsigned kSheetTitleCharSize = 26;
    constexpr unsigned kSheetHeaderCharSize = 16;
    constexpr unsigned kSheetBodyCharSize = 15;
    const sf::Color kSheetSectionColor(230, 220, 160);
    const sf::Color kSheetBodyColor(210, 210, 210);
    const float kSheetMarginX = 40.f;
    const float kSheetRightX = static_cast<float>(windowW) / 2.f + 20.f;
    const float kSheetColumnWidth = static_cast<float>(windowW) / 2.f - kSheetMarginX - 20.f;
    const std::size_t kSheetMaxChars = static_cast<std::size_t>((kSheetColumnWidth - 10.f) / kSidebarCharWidth);

    auto drawCharacterSheetOverlay = [&]() {
        sf::RectangleShape sheetBg(sf::Vector2f(static_cast<float>(windowW), static_cast<float>(windowH)));
        sheetBg.setFillColor(sf::Color(18, 18, 24));
        window.draw(sheetBg);

        const character::Character& c = state.character;
        const auto& race = character::raceInfo(c.race);
        const auto& cls = character::classInfo(c.charClass);
        const character::SubraceInfo* sub = character::subraceInfo(c.subrace);
        const long long currentDay = state.hoursElapsed / 24;

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
        for (const std::string& wrapped : wrapToWidth(carriedLine.str(), kSheetMaxChars)) {
            drawAt(kSheetRightX, rightY, wrapped, kSheetBodyColor, kSheetBodyCharSize);
        }
        rightY += 14.f;

        if (character::canCastSpells(c.charClass)) {
            std::string spellsLine;
            if (character::maxAccessibleSpellLevel(c) == 0) {
                spellsLine = "Spells: cannot cast arcane magic";
            } else if (c.spellsCastDay != currentDay) {
                spellsLine = "Spells: not memorized today -- rest to prepare";
            } else if (c.memorizedSpellIds.empty()) {
                spellsLine = "Spells: none remaining today -- rest to re-prepare";
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
            for (const std::string& wrapped : wrapToWidth(spellsLine, kSheetMaxChars)) {
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
    // GameLoop.cpp's own callers). Extracted here because dialogue's
    // PickingCandidate/TopicPicker cases below were the first two real
    // call sites and already duplicated this exact title/cursor-list/
    // footer shape; picker-shaped screens added since (Shop, Inventory,
    // Spellbook) call this directly instead of re-deriving it again.
    // Deliberately NOT used by combat's PickingTarget -- that picker's
    // cursor is drawn embedded in the roster panel, a structurally
    // different visual shape from this full-window overlay.
    //
    // The optional trailing `message` (default "") is drawn between the
    // item list and the footer -- added for Shop's post-transaction
    // feedback (e.g. "Bought Chain Mail" / "You don't have enough steel
    // for that."), mirroring render::MapRenderer::drawShopFrame's own
    // message placement (MapRenderer.cpp:1089-1092). Dialogue's two
    // existing call sites are unaffected -- they just take the default.
    auto drawPickerOverlay = [&](const std::string& title, const std::vector<std::string>& items,
                                  int selectedIndex, const std::string& footer, const std::string& message = "") {
        sf::RectangleShape bg(sf::Vector2f(static_cast<float>(windowW), static_cast<float>(windowH)));
        bg.setFillColor(sf::Color(18, 18, 24));
        window.draw(bg);

        float y = 40.f;
        auto drawLine = [&](const std::string& text, sf::Color color, unsigned size) {
            sf::Text sfText(font, text, size);
            sfText.setFillColor(color);
            sfText.setPosition(sf::Vector2f(kSheetMarginX, y));
            window.draw(sfText);
            y += static_cast<float>(size) + 10.f;
        };

        drawLine(title, kSheetSectionColor, kSheetTitleCharSize);
        y += 10.f;
        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            const bool isSelected = i == selectedIndex;
            drawLine((isSelected ? "> " : "  ") + items[static_cast<size_t>(i)],
                      isSelected ? sf::Color::White : kSheetBodyColor, kSheetBodyCharSize);
        }
        if (!message.empty()) {
            y += 10.f;
            drawLine(message, sf::Color::White, kSheetBodyCharSize);
        }
        y += 10.f;
        drawLine(footer, sf::Color(150, 150, 160), kSheetHeaderCharSize);
    };

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
            "  wasd = move (no diagonals)",
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
            "  Enter = attack          m = cast (if a caster)",
            "  i = drink a potion      f = flee",
            "",
            "q / Esc = quit (asks to confirm) or leave the current screen",
        };
        drawPickerOverlay("Help", kHelpLines, -1, "(press any key to continue)");
    };

    // Journal ('g') -- a real overlay, not a silent stub, but see
    // journalOpen's own top comment: this build never populates
    // game::GameState::quests yet, so its body says so explicitly instead
    // of rendering render::MapRenderer::drawJournalFrame's own empty-state
    // "(no quests yet)" line (MapRenderer.cpp:1441), which would
    // misleadingly read as "you truly have no quests" rather than "this
    // build doesn't track quests yet."
    auto drawJournalOverlay = [&]() {
        static const std::vector<std::string> kJournalLines = {
            "Quest tracking isn't wired up in this build yet.",
        };
        drawPickerOverlay("Journal", kJournalLines, -1, "(press any key to continue)");
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
        const std::size_t maxLineChars =
            static_cast<std::size_t>((static_cast<float>(windowW) - 2.f * kSheetMarginX) / kSidebarCharWidth);
        std::vector<std::string> wrapped;
        for (const std::string& entry : log) {
            for (std::string& line : wrapToWidth(entry, maxLineChars)) wrapped.push_back(std::move(line));
        }
        const int total = static_cast<int>(wrapped.size());
        // Sized to comfortably fit this build's fixed 1280x800 window
        // alongside drawPickerOverlay's own title/status/footer lines --
        // this file hardcodes windowW/windowH elsewhere too (e.g.
        // kSheetRightX), so a fixed row count matches existing style
        // rather than measuring live text extents.
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
        sf::RectangleShape bg(sf::Vector2f(static_cast<float>(windowW), static_cast<float>(windowH)));
        bg.setFillColor(sf::Color(18, 18, 24));
        window.draw(bg);

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

    // Spellbook ('s' from the character sheet, casters only) -- the
    // sheet's own full-spellbook drill-down, pixel-space equivalent of
    // render::MapRenderer::drawSpellbookFrame (MapRenderer.cpp:842-885).
    // Purely informational (no selection at all, unlike every other
    // drawPickerOverlay caller), so it always passes selectedIndex -1 --
    // the same "no cursor, nothing to select" idiom drawInventoryOverlay
    // already uses for its own leading header rows.
    auto drawSpellbookOverlay = [&]() {
        const character::Character& c = state.character;
        const long long currentDay = state.hoursElapsed / 24;
        std::vector<std::string> rows;
        const int maxLevel = character::maxAccessibleSpellLevel(c);
        if (maxLevel == 0) {
            rows.push_back("Cannot cast arcane magic.");
        } else {
            const bool memorizedToday = c.spellsCastDay == currentDay;
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
                    if (memorizedToday) {
                        const int count = static_cast<int>(
                            std::count(c.memorizedSpellIds.begin(), c.memorizedSpellIds.end(), spell->id));
                        if (count > 0) {
                            line << " (memorized";
                            if (count > 1) line << " x" << count;
                            line << ")";
                        }
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
        sf::RectangleShape bg(sf::Vector2f(static_cast<float>(windowW), static_cast<float>(windowH)));
        bg.setFillColor(sf::Color(18, 18, 24));
        window.draw(bg);

        const std::size_t maxChars =
            static_cast<std::size_t>((static_cast<float>(windowW) - 2.f * kSheetMarginX) / kSidebarCharWidth);
        float y = 40.f;

        auto drawLine = [&](const std::string& text, sf::Color color, unsigned size) {
            sf::Text sfText(font, text, size);
            sfText.setFillColor(color);
            sfText.setPosition(sf::Vector2f(kSheetMarginX, y));
            window.draw(sfText);
            y += static_cast<float>(size) + 10.f;
        };

        switch (dialogueSession.uiState) {
            case DialogueUiState::Greeting:
            case DialogueUiState::TopicText:
            case DialogueUiState::AskResponse: {
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
                for (const std::string& wrapped : wrapToWidth(dialogueSession.bodyText, maxChars)) {
                    drawLine(wrapped, kSheetBodyColor, kSheetBodyCharSize);
                }
                y += 10.f;
                drawLine("(press Enter to continue)", sf::Color(150, 150, 160), kSheetHeaderCharSize);
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
            case DialogueUiState::AskInput:
                drawLine("Ask " + dialogueSession.current.name + " about...", kSheetSectionColor,
                         kSheetTitleCharSize);
                y += 10.f;
                if (!dialogueSession.askInputHints.empty()) {
                    std::string hintLine = "You could ask about: ";
                    for (std::size_t i = 0; i < dialogueSession.askInputHints.size(); ++i) {
                        if (i > 0) hintLine += ", ";
                        hintLine += dialogueSession.askInputHints[i];
                    }
                    for (const std::string& wrapped : wrapToWidth(hintLine, maxChars)) {
                        drawLine(wrapped, sf::Color(150, 150, 160), kSheetHeaderCharSize);
                    }
                    y += 10.f;
                }
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
                    bool wantsLog = false;
                    bool wantsJournal = false;
                    bool wantsWorldMap = false;
                    bool wantsHelp = false;
                    switch (key) {
                        case sf::Keyboard::Key::W:
                        case sf::Keyboard::Key::Up: dy = -1; break;
                        case sf::Keyboard::Key::S:
                        case sf::Keyboard::Key::Down: dy = 1; break;
                        case sf::Keyboard::Key::A:
                        case sf::Keyboard::Key::Left: dx = -1; break;
                        case sf::Keyboard::Key::D:
                        case sf::Keyboard::Key::Right: dx = 1; break;
                        // Deferred, not closed inline here -- while the ask-input
                        // text box is open, 'q' is an ordinary letter a player may
                        // need to type (see the ask-input guard below), not a quit
                        // key, and Escape cancels that box instead of the window.
                        case sf::Keyboard::Key::Q:
                        case sf::Keyboard::Key::Escape: wantsQuit = true; break;
                        case sf::Keyboard::Key::Backspace: wantsBackspace = true; break;
                        case sf::Keyboard::Key::L: placeholder = "Look: not yet implemented in this build."; break;
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
                        case sf::Keyboard::Key::M: placeholder = "Cast: not available outside combat."; break;
                        case sf::Keyboard::Key::R: placeholder = "Rest: not yet implemented in this build."; break;
                        case sf::Keyboard::Key::Z:
                            placeholder = "Bed rest: not yet implemented in this build.";
                            break;
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
                            case DialogueUiState::Greeting:
                            case DialogueUiState::TopicText:
                            case DialogueUiState::AskResponse:
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
                    } else if (combatSession.active &&
                               (combatSession.uiState == CombatUiState::PickingSpell ||
                                combatSession.uiState == CombatUiState::PickingItem) &&
                               wantsQuit) {
                        // Escape/Q cancels the spell/item choice itself,
                        // back to Idle, no round consumed -- mirrors
                        // GameLoop::runCombat's own blocking spell-choice and
                        // USE-menu loops, where Quit sets cancelled=true and
                        // the round loop `continue`s without ever reaching
                        // the initiative dispatch (GameLoop.cpp:2923-2928 for
                        // spells, GameLoop.cpp:2973-2978 for items). Checked
                        // ahead of the general quit branch below for the
                        // same reason every other overlay guard above is.
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
                                if (handleEnter) combatSession.uiState = CombatUiState::Idle;
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
                            case CombatUiState::Idle:
                                if (key == sf::Keyboard::Key::F) {
                                    combatBeginFlee();
                                } else if (key == sf::Keyboard::Key::M) {
                                    combatBeginCast();
                                } else if (key == sf::Keyboard::Key::I) {
                                    combatBeginUseItem();
                                } else if (handleEnter) {
                                    combatBeginPlayerAttack();
                                } else if (dx != 0 || dy != 0) {
                                    combatBeginPlayerMove(dx, dy);
                                }
                                break;
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
                    } else if (handleEnter) {
                        if (state.mode == game::Mode::Overworld) {
                            const world::Location* here = world.locationAt(state.x, state.y);
                            const world::Zone* zone = here ? zones.getZone(here->id) : nullptr;
                            if (zone) {
                                state.mode = game::Mode::Zone;
                                state.currentZoneId = here->id;
                                state.zoneX = zone->entryX();
                                state.zoneY = zone->entryY();
                                currentZone = zone;
                                pushLog("You step into " + zone->name() + ".");
                            } else {
                                pushLog("There's nothing to enter here.");
                            }
                        } else if (currentZone) {
                            if (const std::string* portalTarget = currentZone->portalAt(state.zoneX, state.zoneY)) {
                                const world::Zone* target = zones.getZone(*portalTarget);
                                if (target) {
                                    state.zoneStack.push_back({state.currentZoneId, state.zoneX, state.zoneY});
                                    state.currentZoneId = *portalTarget;
                                    state.zoneX = target->entryX();
                                    state.zoneY = target->entryY();
                                    currentZone = target;
                                    pushLog("You step into " + target->name() + ".");
                                } else {
                                    pushLog("That doorway doesn't lead anywhere in this build.");
                                }
                            } else if (state.zoneX == currentZone->entryX() &&
                                       state.zoneY == currentZone->entryY()) {
                                if (!state.zoneStack.empty()) {
                                    const game::ZoneReturnPoint back = state.zoneStack.back();
                                    state.zoneStack.pop_back();
                                    state.currentZoneId = back.zoneId;
                                    state.zoneX = back.x;
                                    state.zoneY = back.y;
                                    currentZone = zones.getZone(state.currentZoneId);
                                    pushLog("You step back out into " +
                                            (currentZone ? currentZone->name() : back.zoneId) + ".");
                                } else {
                                    pushLog("You step back outside.");
                                    state.mode = game::Mode::Overworld;
                                    currentZone = nullptr;
                                }
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
                    } else if (wantsLog) {
                        logSession.active = true;
                        logSession.scrollOffset = -1; // start at the bottom (most recent) every time it's opened
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
                            if (terrain.passable) {
                                state.x = nx;
                                state.y = ny;
                                const world::Location* here = world.locationAt(state.x, state.y);
                                if (here != nullptr) {
                                    pushLog("Arrived at " + here->name + ".");
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
                            } else {
                                pushLog("Blocked: cannot walk onto " + std::string(terrain.name) + ".");
                            }
                        } else if (currentZone) {
                            const int nx = state.zoneX + dx;
                            const int ny = state.zoneY + dy;
                            const bool isPoi = currentZone->poiAt(nx, ny) != nullptr;
                            const world::ZoneTileInfo& tile = world::zoneTileFor(currentZone->tileCodeAt(nx, ny));
                            if (isPoi || tile.passable) {
                                state.zoneX = nx;
                                state.zoneY = ny;
                                if (const world::PointOfInterest* poi = currentZone->poiAt(state.zoneX, state.zoneY)) {
                                    pushLog("Here: " + poi->name + ".");
                                }
                            } else {
                                pushLog("Blocked: cannot walk onto " + std::string(tile.name) + ".");
                            }
                        }
                    }
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

            window.clear(sf::Color::Black);
            window.setView(mapView);

            if (combatSession.active) {
                // Whole grid always fits the viewport, same "no scrolling
                // needed" approach as zone interiors -- kCombatGridWidth x
                // kCombatGridHeight is fixed and small (15x9). No sprite art
                // here either (same reasoning as zones): a uniform floor
                // color plus each entity's own marker color/letter is the
                // full visual vocabulary for this phase.
                const float combatPxW = static_cast<float>(kCombatGridWidth) * kCombatTilePx;
                const float combatPxH = static_cast<float>(kCombatGridHeight) * kCombatTilePx;
                mapView.setCenter(sf::Vector2f(combatPxW / 2.f, combatPxH / 2.f));
                window.setView(mapView);

                for (int gy = 0; gy < kCombatGridHeight; ++gy) {
                    for (int gx = 0; gx < kCombatGridWidth; ++gx) {
                        const float cx = (static_cast<float>(gx) + 0.5f) * kCombatTilePx;
                        const float cy = (static_cast<float>(gy) + 0.5f) * kCombatTilePx;
                        combatTileShape.setPosition(
                            sf::Vector2f(cx - (kCombatTilePx - 2.f) / 2.f, cy - (kCombatTilePx - 2.f) / 2.f));
                        window.draw(combatTileShape);
                    }
                }
                combatGridBorder.setSize(sf::Vector2f(combatPxW, combatPxH));
                combatGridBorder.setPosition(sf::Vector2f(0.f, 0.f));
                window.draw(combatGridBorder);

                if (combatSession.uiState == CombatUiState::PickingTarget && !combatSession.pickCandidates.empty()) {
                    const int pickedIdx = combatSession.pickCandidates[static_cast<size_t>(combatSession.pickSelected)];
                    const combat::GridPos pos = combatSession.instancePositions[static_cast<size_t>(pickedIdx)];
                    const float cx = (static_cast<float>(pos.x) + 0.5f) * kCombatTilePx;
                    const float cy = (static_cast<float>(pos.y) + 0.5f) * kCombatTilePx;
                    combatPickHighlight.setPosition(
                        sf::Vector2f(cx - (kCombatTilePx - 6.f) / 2.f, cy - (kCombatTilePx - 6.f) / 2.f));
                    window.draw(combatPickHighlight);
                }

                constexpr unsigned kCombatGlyphCharSize = 16;
                for (size_t i = 0; i < combatSession.instances.size(); ++i) {
                    if (combatSession.instances[i].hp <= 0) continue;
                    const combat::GridPos pos = combatSession.instancePositions[i];
                    const float cx = (static_cast<float>(pos.x) + 0.5f) * kCombatTilePx;
                    const float cy = (static_cast<float>(pos.y) + 0.5f) * kCombatTilePx;
                    combatMonsterMarker.setPosition(sf::Vector2f(cx, cy));
                    window.draw(combatMonsterMarker);
                    sf::Text glyph(font, std::string(1, static_cast<char>('A' + i)), kCombatGlyphCharSize);
                    glyph.setFillColor(sf::Color::White);
                    glyph.setPosition(sf::Vector2f(cx - 5.f, cy - 10.f));
                    window.draw(glyph);
                }
                for (size_t i = 0; i < combatSession.companionPositions.size(); ++i) {
                    if (!combatCompanionAlive(i)) continue;
                    const combat::GridPos pos = combatSession.companionPositions[i];
                    const float cx = (static_cast<float>(pos.x) + 0.5f) * kCombatTilePx;
                    const float cy = (static_cast<float>(pos.y) + 0.5f) * kCombatTilePx;
                    combatCompanionMarker.setPosition(sf::Vector2f(cx, cy));
                    window.draw(combatCompanionMarker);
                    sf::Text glyph(font, std::string(1, static_cast<char>('c' + i)), kCombatGlyphCharSize);
                    glyph.setFillColor(sf::Color::White);
                    glyph.setPosition(sf::Vector2f(cx - 5.f, cy - 10.f));
                    window.draw(glyph);
                }
                {
                    const float cx = (static_cast<float>(combatSession.playerPos.x) + 0.5f) * kCombatTilePx;
                    const float cy = (static_cast<float>(combatSession.playerPos.y) + 0.5f) * kCombatTilePx;
                    playerMarker.setPosition(sf::Vector2f(cx, cy));
                    window.draw(playerMarker);
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
            window.draw(sidebarBg);

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

            const std::size_t maxLineChars =
                static_cast<std::size_t>((sidebarWidth - 32.f) / kSidebarCharWidth);
            // The combat log below already wraps through this -- these
            // static/dynamic prompt lines (e.g. "ATTACK (Enter)   MOVE
            // (wasd)   FLEE (f)") didn't, so a long enough one ran past the
            // sidebar's own width and was clipped by the window's right
            // edge (found live, mid-fight). Wrapping them the same way.
            auto drawWrappedLine = [&](const std::string& text, sf::Color color) {
                for (const std::string& wrapped : wrapToWidth(text, maxLineChars)) drawLine(wrapped, color);
            };

            if (combatSession.active) {
                const world::TerrainInfo& floorTerrain = world::terrainFor(combatSession.floorTerrainCode);
                drawLine("Battlefield: " + std::string(floorTerrain.name), sf::Color(200, 200, 140));
                lineY += lineHeight * 0.3f;

                drawLine(state.character.name + " -- HP " + std::to_string(state.character.currentHp) + "/" +
                              std::to_string(state.character.maxHp) + "  AC " +
                              std::to_string(state.character.armorClass),
                          sf::Color(255, 215, 0));
                for (const game::RecruitedCompanion& rc : state.companions) {
                    const character::Character& c = rc.character;
                    std::string line = c.name + " -- HP " + std::to_string(std::max(0, c.currentHp)) + "/" +
                                        std::to_string(c.maxHp) + "  AC " + std::to_string(c.armorClass);
                    if (c.currentHp <= 0) line += " (knocked out)";
                    drawLine(line, sf::Color(80, 150, 220));
                }
                lineY += lineHeight * 0.3f;

                const bool showRosterCursor = combatSession.uiState == CombatUiState::PickingTarget;
                for (size_t i = 0; i < combatSession.instances.size(); ++i) {
                    std::string line;
                    if (showRosterCursor) {
                        const bool isSelected =
                            !combatSession.pickCandidates.empty() &&
                            combatSession.pickCandidates[static_cast<size_t>(combatSession.pickSelected)] ==
                                static_cast<int>(i);
                        line += isSelected ? "> " : "  ";
                    }
                    line += combatMonsterLabel(static_cast<int>(i)) + " -- HP " +
                            std::to_string(std::max(0, combatSession.instances[i].hp)) + "/" +
                            std::to_string(combatSession.instances[i].maxHp);
                    if (combatSession.instances[i].hp <= 0) line += " (defeated)";
                    drawLine(line, sf::Color(220, 100, 100));
                }
                lineY += lineHeight * 0.3f;

                drawLine("-- Log --", sf::Color(140, 140, 160));
                constexpr std::size_t kCombatLogTail = 10;
                const std::size_t combatLogStart =
                    combatSession.log.size() > kCombatLogTail ? combatSession.log.size() - kCombatLogTail : 0;
                for (std::size_t i = combatLogStart; i < combatSession.log.size(); ++i) {
                    for (const std::string& wrapped : wrapToWidth(combatSession.log[i], maxLineChars)) {
                        drawLine(wrapped, sf::Color(190, 190, 200));
                    }
                }
                lineY += lineHeight * 0.3f;

                switch (combatSession.uiState) {
                    case CombatUiState::AwaitContinue:
                        drawWrappedLine("Press Enter to continue.", sf::Color(230, 220, 160));
                        break;
                    case CombatUiState::PickingTarget: {
                        std::string prompt;
                        switch (combatSession.pickReason) {
                            case TargetPickReason::Spell:
                                prompt = "Cast " + combatSession.pendingSpellResult.spellName + " at which enemy?";
                                break;
                            case TargetPickReason::Webnet:
                                prompt = "Tangle which enemy?";
                                break;
                            case TargetPickReason::Attack:
                                prompt = "Attack which enemy?";
                                break;
                        }
                        drawWrappedLine(prompt, sf::Color(230, 220, 160));
                        drawWrappedLine("up/down=select   Enter=choose", sf::Color(150, 150, 160));
                        break;
                    }
                    case CombatUiState::PickingSpell:
                        // The real picker (drawCombatSpellPickerOverlay)
                        // draws on top of this whole frame -- this line is
                        // never actually seen, just here so the switch
                        // covers every CombatUiState value.
                        drawWrappedLine("Choose a spell...", sf::Color(230, 220, 160));
                        break;
                    case CombatUiState::PickingItem:
                        // Same "never actually seen" idiom as PickingSpell
                        // above -- drawCombatItemPickerOverlay draws on top.
                        drawWrappedLine("Choose an item...", sf::Color(230, 220, 160));
                        break;
                    case CombatUiState::Won:
                        drawWrappedLine("Victory! Press Enter to continue.", sf::Color(120, 220, 120));
                        break;
                    case CombatUiState::Lost:
                        drawWrappedLine("Press Enter to continue.", sf::Color(220, 120, 120));
                        break;
                    case CombatUiState::Fled:
                        drawWrappedLine("Press Enter to continue.", sf::Color(220, 190, 120));
                        break;
                    case CombatUiState::Idle:
                        drawWrappedLine("ATTACK (Enter)   MOVE (wasd)   FLEE (f)   CAST (m)   USE (i)",
                                         sf::Color(190, 190, 200));
                        break;
                }
            } else {
                drawLine(state.character.name + ", level " + std::to_string(state.character.level) + " " +
                              std::string(character::raceInfo(state.character.race).name) + " " +
                              std::string(character::classInfo(state.character.charClass).name),
                          sf::Color::White);
                drawLine("HP: " + std::to_string(state.character.currentHp) + "/" +
                              std::to_string(state.character.maxHp),
                          sf::Color(220, 90, 90));
                drawLine(formatDayTime(state.hoursElapsed), sf::Color(200, 200, 140));
                if (state.mode == game::Mode::Zone && currentZone) {
                    drawLine("Indoors -- " + currentZone->name(), sf::Color(150, 200, 230));
                }
                lineY += lineHeight * 0.5f;
                drawLine("-- Log --", sf::Color(140, 140, 160));

                for (const std::string& entry : log) {
                    for (const std::string& wrapped : wrapToWidth(entry, maxLineChars)) {
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
            } else if (journalOpen) {
                window.setView(uiView);
                drawJournalOverlay();
            } else if (logSession.active) {
                window.setView(uiView);
                drawLogOverlay();
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
    if (argc < 2) {
        std::cerr << "usage: ansalon_sfml_phase1 <path-to-save-file>\n"
                     "  run from the repo root, e.g.:\n"
                     "  .\\build\\Debug\\ansalon_sfml_phase1.exe build\\Debug\\save1.txt\n"
                     "Read-only -- never writes back to the save file.\n";
        return 1;
    }
    try {
        return runPhase1(argv[1]);
    } catch (const std::exception& e) {
        std::cerr << "FATAL EXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "FATAL UNKNOWN EXCEPTION" << std::endl;
        return 1;
    }
}
