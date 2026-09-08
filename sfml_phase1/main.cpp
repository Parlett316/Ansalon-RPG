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
// leveling, and knockout. Spellcasting, item use, thief backstab, and
// Fighter sweep are each a real chunk of new chooser UI or extra positional
// bookkeeping on top of `GameLoop::runCombat` -- deferred to a later phase,
// same "not yet in this build" convention Phase 1 already established for
// Look/Talk/Shop/etc. See docs/CURRENT_WORK.md for the full scope writeup.

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
#include <cmath>
#include <deque>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
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

// Renders an AttackOutcome's roll math, e.g. "[d20 14 +2 = 16 vs THAC0 18 -
// AC 6 (need 12)]" -- copied verbatim from game::describeToHit
// (GameLoop.cpp), same "not exported, GameLoop.cpp isn't linked here"
// reasoning as pluralMonsterName above.
std::string describeToHit(const combat::AttackOutcome& outcome) {
    std::ostringstream out;
    out << "[d20 " << outcome.naturalRoll;
    if (outcome.naturalRoll == 20) {
        out << " -- natural 20, automatic hit]";
        return out.str();
    }
    if (outcome.naturalRoll == 1) {
        out << " -- natural 1, automatic miss]";
        return out.str();
    }
    if (outcome.toHitBonus != 0) {
        out << (outcome.toHitBonus > 0 ? " +" : " ") << outcome.toHitBonus << " = "
            << (outcome.naturalRoll + outcome.toHitBonus);
    }
    out << " vs THAC0 " << outcome.attackerThac0 << " - AC " << outcome.defenderArmorClass << " (need "
        << outcome.targetNumber << ")]";
    return out.str();
}

// Renders the damage-roll math the same way describeToHit renders the
// attack roll -- copied verbatim from game::describeDamage (GameLoop.cpp).
std::string describeDamage(const combat::AttackOutcome& outcome) {
    std::ostringstream out;
    out << "[" << outcome.damageDiceCount << "d" << outcome.damageDiceSides << " "
        << (outcome.damageMultiplier != 1 ? outcome.damageRoll / outcome.damageMultiplier : outcome.damageRoll);
    if (outcome.damageMultiplier != 1) {
        out << " x" << outcome.damageMultiplier;
    }
    if (outcome.damageBonus != 0) {
        out << (outcome.damageBonus > 0 ? " +" : " ") << outcome.damageBonus;
    }
    if (outcome.damageBonus != 0 || outcome.damageMultiplier != 1) {
        out << " = " << (outcome.damageRoll + outcome.damageBonus);
    }
    out << "]";
    return out.str();
}

// A talk interaction's content -- trimmed local counterpart to
// game::Speech (GameLoop.h), which isn't linkable here (GameLoop.cpp is
// built on render::Console/render::MapRenderer, which this target
// deliberately excludes -- see this file's own top-of-file comment).
// Deliberately omits game::Speech's subjects/subjectUnknown/askLimit*/
// suppressAskHints/askLimitLocked* fields: free-text "ask about
// something else..." is deferred to a later phase (see docs/CURRENT_WORK.md),
// and askLimitLocked can only ever fire once that free-text flow has
// exhausted a daily question cap -- with no such flow here, it can never
// trigger, so it's dropped rather than carried as dead code.
struct DialogueSpeech {
    std::string greeting;                                          // plain SAY/TALK
    std::vector<std::pair<std::string, std::string>> conditional;  // SAY_IF
    std::string again;                                             // SAY_AGAIN/TALK_AGAIN
    std::vector<std::pair<std::string, std::string>> topics;       // TOPIC
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

// Adapts a timeline::PresenceWindow into DialogueSpeech -- trimmed local
// counterpart to game::speechFromWindow (GameLoop.cpp), dropping the
// subjects/subjectUnknown fields per DialogueSpeech's own comment above.
DialogueSpeech speechFromWindow(const timeline::PresenceWindow& window) {
    DialogueSpeech speech;
    speech.greeting = window.dialogue;
    speech.conditional = window.conditionalDialogue;
    speech.again = window.dialogueAgain;
    speech.topics = window.topics;
    return speech;
}

// Adapts a world::PointOfInterest into DialogueSpeech -- the zone-native
// counterpart to speechFromWindow above, trimmed local counterpart to
// game::speechFromPoi (GameLoop.cpp).
DialogueSpeech speechFromPoi(const world::PointOfInterest& poi) {
    DialogueSpeech speech;
    speech.greeting = poi.dialogue;
    speech.conditional = poi.conditionalDialogue;
    speech.again = poi.dialogueAgain;
    speech.topics = poi.topics;
    return speech;
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
// non-blockingly -- a deliberate scope cut, not an oversight.
enum class CombatUiState { AwaitContinue, Idle, PickingTarget, Won, Lost, Fled };

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
    // GameLoop::runCombat's own identically-named locals establish. Every
    // other this-fight buff/debuff in the console version (Bless, Prayer,
    // Slow, Haste, ...) is spell-driven and out of this phase's scope, so
    // there's nothing else to carry here yet.
    int playerThac0Bonus = 0;
    int playerDamageBonus = 0;
};

// Dialogue's own UI states -- non-blocking port of GameLoop::talkTo's
// "core conversation" slice (see docs/CURRENT_WORK.md's scope writeup for
// this phase; free-text ask/quest/boat/recruit choosers are deferred, each
// getting a single placeholder log line instead). PickingCandidate mirrors
// GameLoop::pickAndTalk's "more than one candidate here" picker; Greeting/
// TopicText/TopicPicker mirror talkTo's own greeting-then-topic-menu flow.
enum class DialogueUiState { PickingCandidate, Greeting, TopicText, TopicPicker };

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
    std::vector<std::string> topicLabels;  // topics + "Nothing, thanks"
    int topicSelected = 0;
};

int runPhase1(const std::string& savePath) {
    const unsigned windowW = 1280;
    const unsigned windowH = 800;
    const float sidebarWidth = 320.f;
    const float mapWidth = static_cast<float>(windowW) - sidebarWidth;

    std::cout << "step 0: starting, save = " << savePath << std::endl;

    world::OverworldGrid grid =
        world::OverworldGrid::loadFromFile("data/overworld.grid", "data/overworld_regions.grid");
    std::cout << "step 1: grid loaded " << grid.width() << "x" << grid.height() << std::endl;

    world::World world;
    world::WorldLoader::loadFromFile("data/locations.txt", world);
    std::cout << "step 2: world loaded, " << world.allLocations().size() << " locations" << std::endl;

    world::ZoneCatalog zones = world::ZoneCatalog::loadForWorld(world, "data/zones");
    std::cout << "step 2b: zones loaded, " << zones.allZones().size() << " zones" << std::endl;

    combat::MonsterCatalog monsterCatalog;
    combat::MonsterLoader::loadFromFile("data/monsters.txt", monsterCatalog);
    std::cout << "step 2c: monsters loaded, " << monsterCatalog.size() << " entries" << std::endl;

    timeline::Timeline timeline;
    timeline::TimelineLoader::loadFromFile("data/timeline.txt", timeline);
    std::cout << "step 2d: timeline loaded" << std::endl;

    game::GameState state = game::SaveGame::load(savePath);
    std::cout << "step 3: save loaded -- " << state.character.name << ", level "
              << state.character.level << " " << character::raceInfo(state.character.race).name << " "
              << character::classInfo(state.character.charClass).name << ", at (" << state.x << ", "
              << state.y << ")" << std::endl;

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(windowW, windowH)),
                             "Ansalon SFML Phase 1+2+3 -- Real Overworld + Zones + Combat (WIP)");
    window.setFramerateLimit(60);

    sf::Texture mapTexture;
    if (!mapTexture.loadFromFile("References/dragonlancemap2.png")) {
        std::cerr << "Failed to load References/dragonlancemap2.png\n";
        return 1;
    }
    const sf::Vector2u mapSize = mapTexture.getSize();
    std::cout << "step 4: map texture loaded, size " << mapSize.x << "x" << mapSize.y << std::endl;

    sf::Font font;
    if (!font.openFromFile("C:/Windows/Fonts/consola.ttf")) {
        std::cerr << "Failed to load placeholder UI font (C:/Windows/Fonts/consola.ttf) -- "
                     "this build uses a system font as a stand-in until this project has its own.\n";
        return 1;
    }

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
                    candidate.speech = speechFromWindow(*presence.window);
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
                candidate.speech = speechFromPoi(*poi);
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
                    candidate.speech = speechFromWindow(*presence.window);
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

    // Mirrors GameLoop::talkTo's greeting-resolution precedence exactly
    // (GameLoop.cpp:1002-1046), minus the askLimitLocked branch (see
    // DialogueSpeech's own comment for why) and minus boat/quest/recruit's
    // real handling (a single placeholder log line each instead).
    auto dialogueStartTalk = [&](const DialogueCandidate& candidate) {
        dialogueSession.current = candidate;
        const std::string afterId = candidate.id + ":after";
        const bool showAfter = !candidate.dialogueAfter.empty() && state.metCharacters.count(afterId) == 0;
        const bool alreadyMet = state.metCharacters.count(candidate.id) > 0;
        std::string text;
        if (showAfter) {
            text = candidate.dialogueAfter;
            state.metCharacters.insert(afterId);
        } else if (!candidate.dialogueBefore.empty()) {
            text = candidate.dialogueBefore;
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

        dialogueSession.topicLabels.clear();
        for (const auto& [label, topicText] : candidate.speech.topics) dialogueSession.topicLabels.push_back(label);
        dialogueSession.topicLabels.push_back("Nothing, thanks");
        dialogueSession.topicSelected = 0;
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

    // Enter on the "talk to whom?" picker.
    auto dialogueConfirmCandidate = [&]() {
        if (dialogueSession.candidateSelected < 0 ||
            dialogueSession.candidateSelected >= static_cast<int>(dialogueSession.candidates.size())) {
            return;
        }
        dialogueStartTalk(dialogueSession.candidates[static_cast<size_t>(dialogueSession.candidateSelected)]);
    };

    // Enter on the "ask <name> about..." picker -- "Nothing, thanks" is
    // always the last label (see dialogueStartTalk above).
    auto dialogueConfirmTopic = [&]() {
        const int nothingThanksIndex = static_cast<int>(dialogueSession.topicLabels.size()) - 1;
        if (dialogueSession.topicSelected == nothingThanksIndex) {
            dialogueEnd();
            return;
        }
        dialogueSession.bodyText =
            dialogueSession.current.speech.topics[static_cast<size_t>(dialogueSession.topicSelected)].second;
        dialogueSession.uiState = DialogueUiState::TopicText;
    };

    // Enter (or any key, per the console's "press any key to continue") on
    // a plain dialogue box -- Greeting goes to the topic menu if there is
    // one, else ends the conversation; TopicText always returns to the
    // topic menu (mirroring talkTo's own topic loop).
    auto dialogueContinue = [&]() {
        if (dialogueSession.uiState == DialogueUiState::Greeting) {
            if (!dialogueSession.current.speech.topics.empty()) {
                dialogueSession.uiState = DialogueUiState::TopicPicker;
            } else {
                dialogueEnd();
            }
        } else if (dialogueSession.uiState == DialogueUiState::TopicText) {
            dialogueSession.uiState = DialogueUiState::TopicPicker;
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
            std::string name = combatMonsterLabel(static_cast<int>(i));
            combat::AttackOutcome outcome = combat::resolveMonsterAttack(combatSession.monster, state.character);
            if (outcome.hit) {
                state.character.currentHp -= outcome.damage;
                combatSession.log.push_back("As you pull back, the " + name + " gets a free strike! It hits you for " +
                                             std::to_string(outcome.damage) + ". " + describeToHit(outcome) + " " +
                                             describeDamage(outcome));
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

    // Every alive companion's own turn, AI-controlled (player-directed
    // party control stays out of scope project-wide -- see
    // docs/COMBAT_NOTES.md's "Extending this later"): attacks the first
    // adjacent alive instance found, or takes one combat::stepToward step
    // toward the nearest alive instance if none is adjacent yet. No sweep
    // (Fighter-type sweep is deferred, see this file's top-of-file
    // comment) and no mid-round retarget-on-kill (this phase's one-target-
    // per-round simplification, see CombatUiState's own doc comment) --
    // both real simplifications versus GameLoop::companionActs.
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
            int attacks = character::meleeAttacksThisRound(companion.charClass, companion.level,
                                                             combatSession.roundNumber, false);
            for (int i = 0; i < attacks; ++i) {
                if (combatSession.instances[static_cast<size_t>(targetIndex)].hp <= 0) break;
                std::string targetName = combatMonsterLabel(targetIndex);
                combat::AttackOutcome outcome = combat::resolvePlayerAttack(companion, combatSession.monster);
                if (outcome.hit) {
                    combatSession.instances[static_cast<size_t>(targetIndex)].hp -= outcome.damage;
                    combatSession.log.push_back(companion.name + " hits the " + targetName + " for " +
                                                 std::to_string(outcome.damage) + ". " + describeToHit(outcome) +
                                                 " " + describeDamage(outcome));
                    if (combatSession.instances[static_cast<size_t>(targetIndex)].hp <= 0) {
                        if (combatHandleInstanceDeath(targetIndex)) return;
                    }
                } else {
                    combatSession.log.push_back(companion.name + " misses the " + targetName + ". " +
                                                 describeToHit(outcome));
                }
            }
        }
    };

    // Every alive monster instance's own turn: Bozak's Magic Missile and
    // Aurak's breath weapon (both real, sourced, passive specials that
    // fire on their own turn with no player choice involved -- see this
    // file's top-of-file comment for why these stay in scope despite
    // spellcasting itself being deferred) take priority over its plain
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
            const combat::Monster& monster = combatSession.monster;
            if (monster.castsMagicMissile && character::roll(1, 100) <= monster.magicMissileChancePercent) {
                int missileDamage = (character::roll(1, 4) + 1) + (character::roll(1, 4) + 1);
                state.character.currentHp -= missileDamage;
                combatSession.log.push_back("The " + name + " casts Magic Missile! It strikes you for " +
                                             std::to_string(missileDamage) + " -- no saving throw.");
                continue;
            }
            if (monster.hasBreathWeapon && character::roll(1, 100) <= monster.breathWeaponChancePercent) {
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
            std::string targetName = target.isPlayer ? "you" : target.character->name;
            combat::AttackOutcome outcome = combat::resolveMonsterAttack(monster, *target.character);
            if (outcome.hit) {
                target.character->currentHp -= outcome.damage;
                combatSession.log.push_back("The " + name + " hits " + targetName + " for " +
                                             std::to_string(outcome.damage) + ". " + describeToHit(outcome) + " " +
                                             describeDamage(outcome));
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
                combatSession.log.push_back("The " + name + " misses " + targetName + ". " + describeToHit(outcome));
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
        int attacks = character::meleeAttacksThisRound(state.character.charClass, state.character.level,
                                                         combatSession.roundNumber, state.character.specializedWeapon);
        for (int i = 0; i < attacks; ++i) {
            if (combatSession.instances[static_cast<size_t>(targetIndex)].hp <= 0) break;
            std::string targetName = combatMonsterLabel(targetIndex);
            combat::AttackOutcome outcome = combat::resolvePlayerAttack(
                state.character, combatSession.monster, combatSession.playerThac0Bonus, combatSession.playerDamageBonus);
            if (outcome.hit) {
                combatSession.instances[static_cast<size_t>(targetIndex)].hp -= outcome.damage;
                combatSession.log.push_back("You hit the " + targetName + " for " + std::to_string(outcome.damage) +
                                             ". " + describeToHit(outcome) + " " + describeDamage(outcome));
                if (combatSession.instances[static_cast<size_t>(targetIndex)].hp <= 0) {
                    if (combatHandleInstanceDeath(targetIndex)) return;
                }
            } else {
                combatSession.log.push_back("You miss the " + targetName + ". " + describeToHit(outcome));
            }
        }
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
        combatSession.pickCandidates = candidates;
        combatSession.pickSelected = 0;
        combatSession.uiState = CombatUiState::PickingTarget;
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
        for (size_t i = 0; i < combatSession.instances.size(); ++i) {
            if (combatSession.instances[i].hp > 0 && combatSession.instancePositions[i].x == destination.x &&
                combatSession.instancePositions[i].y == destination.y) {
                combatSession.log.push_back("Something's in the way.");
                return;
            }
        }
        for (size_t ci = 0; ci < state.companions.size(); ++ci) {
            if (combatCompanionAlive(ci) && combatSession.companionPositions[ci].x == destination.x &&
                combatSession.companionPositions[ci].y == destination.y) {
                combatSession.log.push_back("Something's in the way.");
                return;
            }
        }
        if (!combatRollGoFirstAndMaybeActMonsters()) return;
        combatTriggerOpportunityAttacks(destination);
        if (combatCheckPlayerDown(combatSession.monster.name)) return;
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
        combatResolveAttackAgainstTarget(target);
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
    // drawCharacterSheet (MapRenderer.cpp:689-840) field-for-field, minus
    // its 's' = full-spellbook drill-down (spellbook is its own separate
    // roadmap item, out of scope here) -- laid out in two real pixel-space
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
        // summary (no spellbook drill-down -- see this lambda's own top
        // comment), companions.
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

        sf::Text footer(font, "(press any key to return)", kSheetHeaderCharSize);
        footer.setFillColor(sf::Color(150, 150, 160));
        footer.setPosition(sf::Vector2f(kSheetMarginX, static_cast<float>(windowH) - 40.f));
        window.draw(footer);
    };

    // Dialogue -- a full-window overlay, same compositing approach as
    // drawCharacterSheetOverlay above (drawn as a final layer on top of
    // the map/sidebar, gated on dialogueSession.active). Pixel-space
    // equivalent of render::MapRenderer::drawDialogueFrame/drawPickerFrame,
    // reusing the character sheet's own color/size constants above for
    // visual consistency across overlays.
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
                drawLine(dialogueSession.current.name, sf::Color::White, kSheetTitleCharSize);
                y += 10.f;
                for (const std::string& wrapped : wrapToWidth(dialogueSession.bodyText, maxChars)) {
                    drawLine(wrapped, kSheetBodyColor, kSheetBodyCharSize);
                }
                y += 10.f;
                drawLine("(press Enter to continue)", sf::Color(150, 150, 160), kSheetHeaderCharSize);
                break;
            case DialogueUiState::PickingCandidate:
                drawLine("Talk to whom?", kSheetSectionColor, kSheetTitleCharSize);
                y += 10.f;
                for (int i = 0; i < static_cast<int>(dialogueSession.candidates.size()); ++i) {
                    const bool isSelected = i == dialogueSession.candidateSelected;
                    drawLine((isSelected ? "> " : "  ") + dialogueSession.candidates[static_cast<size_t>(i)].name,
                             isSelected ? sf::Color::White : kSheetBodyColor, kSheetBodyCharSize);
                }
                y += 10.f;
                drawLine("(up/down = select, Enter = talk)", sf::Color(150, 150, 160), kSheetHeaderCharSize);
                break;
            case DialogueUiState::TopicPicker:
                drawLine("Ask " + dialogueSession.current.name + " about...", kSheetSectionColor,
                         kSheetTitleCharSize);
                y += 10.f;
                for (int i = 0; i < static_cast<int>(dialogueSession.topicLabels.size()); ++i) {
                    const bool isSelected = i == dialogueSession.topicSelected;
                    drawLine((isSelected ? "> " : "  ") + dialogueSession.topicLabels[static_cast<size_t>(i)],
                             isSelected ? sf::Color::White : kSheetBodyColor, kSheetBodyCharSize);
                }
                y += 10.f;
                drawLine("(up/down = select, Enter = ask)", sf::Color(150, 150, 160), kSheetHeaderCharSize);
                break;
        }
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
                    switch (key) {
                        case sf::Keyboard::Key::W:
                        case sf::Keyboard::Key::Up: dy = -1; break;
                        case sf::Keyboard::Key::S:
                        case sf::Keyboard::Key::Down: dy = 1; break;
                        case sf::Keyboard::Key::A:
                        case sf::Keyboard::Key::Left: dx = -1; break;
                        case sf::Keyboard::Key::D:
                        case sf::Keyboard::Key::Right: dx = 1; break;
                        case sf::Keyboard::Key::Q:
                        case sf::Keyboard::Key::Escape: window.close(); break;
                        case sf::Keyboard::Key::L: placeholder = "Look: not yet implemented in this build."; break;
                        case sf::Keyboard::Key::T: wantsTalk = true; break;
                        case sf::Keyboard::Key::Enter: handleEnter = true; break;
                        case sf::Keyboard::Key::C: break; // handled explicitly below via sheetOpen
                        case sf::Keyboard::Key::P: placeholder = "Shop: not yet implemented in this build."; break;
                        case sf::Keyboard::Key::I:
                            placeholder = "Inventory: not yet implemented in this build.";
                            break;
                        case sf::Keyboard::Key::V:
                            placeholder = "Full log view: not yet implemented in this build.";
                            break;
                        case sf::Keyboard::Key::G:
                            placeholder = "Journal: not yet implemented in this build.";
                            break;
                        case sf::Keyboard::Key::F: placeholder = "Flee: not available outside combat."; break;
                        case sf::Keyboard::Key::M: placeholder = "Cast: not available outside combat."; break;
                        case sf::Keyboard::Key::R: placeholder = "Rest: not yet implemented in this build."; break;
                        case sf::Keyboard::Key::Z:
                            placeholder = "Bed rest: not yet implemented in this build.";
                            break;
                        default: break;
                    }

                    if (sheetOpen) {
                        // Dismiss on any key -- see sheetOpen's own comment
                        // above. Deliberately swallows dx/dy/handleEnter too,
                        // so the same keypress that closes the sheet never
                        // also moves the character or opens combat.
                        sheetOpen = false;
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
                            case CombatUiState::Won:
                            case CombatUiState::Lost:
                            case CombatUiState::Fled:
                                if (handleEnter) combatSession.active = false;
                                break;
                            case CombatUiState::Idle:
                                if (key == sf::Keyboard::Key::F) {
                                    combatBeginFlee();
                                } else if (key == sf::Keyboard::Key::M) {
                                    combatSession.log.push_back("Cast: not yet implemented in this build.");
                                } else if (key == sf::Keyboard::Key::I) {
                                    combatSession.log.push_back("Item use: not yet implemented in this build.");
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
                        // dispatch. Deliberately no cancel key: Q/Escape
                        // already close the whole window unconditionally
                        // (see the switch above), so -- same as combat's
                        // own PickingTarget -- there's no room to also mean
                        // "back out of this menu" here; the always-present
                        // "Nothing, thanks" entry is TopicPicker's only way
                        // out, and PickingCandidate has no cancel at all,
                        // matching PickingTarget's precedent.
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
                        drawLine("Press Enter to continue.", sf::Color(230, 220, 160));
                        break;
                    case CombatUiState::PickingTarget:
                        drawLine("Attack which enemy?", sf::Color(230, 220, 160));
                        drawLine("up/down=select   Enter=choose", sf::Color(150, 150, 160));
                        break;
                    case CombatUiState::Won:
                        drawLine("Victory! Press Enter to continue.", sf::Color(120, 220, 120));
                        break;
                    case CombatUiState::Lost:
                        drawLine("Press Enter to continue.", sf::Color(220, 120, 120));
                        break;
                    case CombatUiState::Fled:
                        drawLine("Press Enter to continue.", sf::Color(220, 190, 120));
                        break;
                    case CombatUiState::Idle:
                        drawLine("ATTACK (Enter)   MOVE (wasd)   FLEE (f)", sf::Color(190, 190, 200));
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

            if (sheetOpen) {
                window.setView(uiView);
                drawCharacterSheetOverlay();
            }

            if (dialogueSession.active) {
                window.setView(uiView);
                drawDialogueOverlay();
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
