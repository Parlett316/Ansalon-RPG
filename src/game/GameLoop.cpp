#include "game/GameLoop.h"
#include "game/SaveGame.h"
#include "character/Companion.h"
#include "character/Dice.h"
#include "character/Equipment.h"
#include "character/Leveling.h"
#include "character/Spellcasting.h"
#include "combat/Combat.h"
#include "render/Console.h"
#include "render/MapRenderer.h"
#include "world/Terrain.h"
#include "world/ZoneTile.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <sstream>

namespace game {

namespace {

// Translates a zone file's plain SHOP catalog name into
// character::ShopCatalog -- lives here, not in world::, because world::
// stays decoupled from character:: (see docs/ARCHITECTURE.md); GameLoop
// already depends on both, same bridging role game::conditionMatches
// plays for SAY_IF/TOPIC condition strings. ZoneLoader already rejected
// anything outside these six names at load time, so the fallback below is
// unreachable in practice, not a silent default.
character::ShopCatalog shopCatalogFor(const std::string& name) {
    if (name == "armory") return character::ShopCatalog::Armory;
    if (name == "market") return character::ShopCatalog::MarketGoods;
    if (name == "salvage") return character::ShopCatalog::Salvage;
    if (name == "bazaar") return character::ShopCatalog::Bazaar;
    if (name == "harbor") return character::ShopCatalog::HarborTrade;
    return character::ShopCatalog::General;
}

// Grid y grows downward (row 0 is the top), so "north" is negative dy --
// easy to get backwards, worth calling out.
const char* compassDirection(int dx, int dy) {
    constexpr double kPi = 3.14159265358979323846;
    double angle = std::atan2(static_cast<double>(-dy), static_cast<double>(dx)); // 0 = east, increases counter-clockwise
    static const char* kDirs[8] = {"east", "northeast", "north", "northwest",
                                    "west", "southwest", "south", "southeast"};
    int index = static_cast<int>(std::lround(angle / (kPi / 4.0))) & 7;
    return kDirs[index];
}

// Renders the PHB p.119/121 to-hit math behind one attack roll as a
// bracketed suffix appended to the existing hit/miss combat-log line --
// e.g. "[d20 14 +2 = 16 vs THAC0 18 - AC 6 (need 12)]" or "[d20 20 --
// natural 20, automatic hit]" -- so the player can see *why* an attack
// landed or didn't, not just that it did. See docs/COMBAT_NOTES.md's
// "Showing the math" section.
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
// attack roll -- e.g. "[1d8 5 +2 = 7]" -- appended only when the attack
// actually hit.
std::string describeDamage(const combat::AttackOutcome& outcome) {
    std::ostringstream out;
    out << "[" << outcome.damageDiceCount << "d" << outcome.damageDiceSides << " " << outcome.damageRoll;
    if (outcome.damageBonus != 0) {
        out << (outcome.damageBonus > 0 ? " +" : " ") << outcome.damageBonus << " = "
            << (outcome.damageRoll + outcome.damageBonus);
    }
    out << "]";
    return out.str();
}

// Adapts a timeline::PresenceWindow (plus the timeline::CanonCharacter it
// belongs to, and the current in-game day) into the Speech shape
// GameLoop::talkTo works with -- see docs/TIMELINE_NOTES.md for the
// underlying grammar. `subjects`/`subjectUnknown` come from
// Timeline::subjectsFor/subjectUnknownFor (Milestone 72) rather than
// reading `window` directly, so a character's day-gated subject pool layers
// in underneath that window's own SUBJECT entries.
Speech speechFromWindow(const timeline::Timeline& timeline, const timeline::CanonCharacter& character,
                         const timeline::PresenceWindow& window, int day) {
    Speech speech;
    speech.greeting = window.dialogue;
    speech.conditional = window.conditionalDialogue;
    speech.again = window.dialogueAgain;
    speech.topics = window.topics;
    for (const auto& subject : timeline.subjectsFor(character, window, day)) {
        speech.subjects.push_back(Speech::SubjectEntry{subject.keywords, subject.text});
    }
    speech.subjectUnknown = timeline.subjectUnknownFor(character, window);
    return speech;
}

// Adapts a world::PointOfInterest into the Speech shape GameLoop::talkTo
// works with -- the zone-native counterpart to speechFromWindow above. See
// docs/ZONE_NOTES.md for the underlying SAY_IF/TOPIC grammar.
Speech speechFromPoi(const world::PointOfInterest& poi) {
    Speech speech;
    speech.greeting = poi.dialogue;
    speech.conditional = poi.conditionalDialogue;
    speech.again = poi.dialogueAgain;
    speech.topics = poi.topics;
    for (const auto& [keywords, text] : poi.subjects) {
        speech.subjects.push_back(Speech::SubjectEntry{keywords, text});
    }
    speech.subjectUnknown = poi.subjectUnknown;
    return speech;
}

// Pluralizes a monster's display name for the group-summary log lines
// runCombat prints when an encounter's group is bigger than one (see
// docs/COMBAT_NOTES.md's "Monster encounter groups" section) -- a plain
// "+s" suffix is wrong for two of the fourteen grouped monsters, so those
// get an explicit override rather than shipping "Timber Wolfs"/"Lizard
// Mans". Per-instance labels ("Timber Wolf A") never need this -- only the
// arrival/victory/flee summary lines that refer to the whole group at once.
std::string pluralMonsterName(const std::string& name) {
    if (name == "Timber Wolf") return "Timber Wolves";
    if (name == "Lizard Man") return "Lizard Men";
    return name + "s";
}

} // namespace

// Small, fixed condition vocabulary for SAY_IF (see docs/TIMELINE_NOTES.md):
// "good"/"evil" check Alignment's ethical axis, the 7 playable races and 5
// classes check by name. An unrecognized condition simply never matches --
// checked at talk-time against runtime character data, not something
// TimelineLoader can validate at load time, so failing safe (fall back to
// the plain greeting) beats a load-time error here. A free function (not
// anonymous-namespace-private) specifically so it's directly unit-testable.
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
    // Distinct from "knight" above: true only for a Knight who has already
    // reached the Sword (not merely Crown) -- gates solamnic_armor
    // (docs/QUEST_NOTES.md), which the source book ties to a rank this
    // project doesn't model ("Lord") and is instead scoped to the
    // already-shipped Sword rank. See docs/CHARACTER_NOTES.md's "Magic
    // items".
    if (condition == "sword_knight") return c.knightOrder == character::KnightOrder::Sword;
    // Compound eligibility check for Order of the Sword advancement, not a
    // single fact like the tokens above (see docs/QUEST_NOTES.md) -- true
    // once a Knight of the Crown has reached level 3 (this project's
    // Fighter-XP-chassis stand-in for the book's "2nd level with enough
    // XP banked for 3rd", the same threshold Leveling.cpp's own Crown
    // flavor message already fires on) and meets the Sword's ability
    // score minimums (character::Knighthood.h).
    if (condition == "sword_eligible") {
        return c.knightOrder == character::KnightOrder::Crown && c.level >= 3 &&
               character::meetsKnightOfSwordRequirements(c.scores);
    }
    // Same shape as sword_eligible above, one rank up: true once a Knight of
    // the Sword has reached level 4 (this project's resolution of the book's
    // own internally-inconsistent Rose level-threshold prose -- see
    // character::meetsKnightOfRoseRequirements and docs/CHARACTER_NOTES.md)
    // and meets the Rose's ability score minimums.
    if (condition == "rose_eligible") {
        return c.knightOrder == character::KnightOrder::Sword && c.level >= 4 &&
               character::meetsKnightOfRoseRequirements(c.scores);
    }
    // Gates frostreaver_salvage (docs/QUEST_NOTES.md) -- Dragonlance
    // Adventures p.94's own printed minimum to wield a Frostreaver, so the
    // quest is never offered to a character who couldn't use the reward.
    if (condition == "str_13") return c.scores.strength >= character::kFrostreaverMinStrength;
    return false;
}

// Lowercases and splits on anything that isn't a letter/digit/hyphen/
// apostrophe -- deliberately simple keyword tokenization, not NLP, matching
// this project's plain, fail-fast data-driven style. Hyphens/apostrophes
// are kept as word characters so a keyword like "half-sister" can still
// match a literally-hyphenated typed word, though authored SUBJECT keyword
// lists don't rely on that (see docs/TIMELINE_NOTES.md's "Ask about
// anything").
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

const Speech::SubjectEntry* matchSubject(const std::vector<Speech::SubjectEntry>& subjects, const std::string& raw) {
    std::vector<std::string> tokens = tokenizeAskInput(raw);
    for (const auto& subject : subjects) {
        for (const std::string& keyword : subject.keywords) {
            std::string lowerKeyword = keyword;
            std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(),
                           [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            for (const std::string& token : tokens) {
                if (token == lowerKeyword) return &subject;
            }
        }
    }
    return nullptr;
}

int objectiveProgress(const quest::Objective& objective, const GameState& state) {
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

bool objectiveMet(const quest::Objective& objective, const GameState& state) {
    return objectiveProgress(objective, state) >= objective.count;
}

bool allObjectivesMet(const quest::Quest& quest, const GameState& state) {
    for (const auto& objective : quest.objectives) {
        if (!objectiveMet(objective, state)) return false;
    }
    return true;
}

GameLoop::GameLoop(const world::World& world, const world::OverworldGrid& grid,
                    const world::ZoneCatalog& zones, const timeline::Timeline& timeline,
                    const combat::MonsterCatalog& monsters, const quest::QuestCatalog& quests,
                    GameState initialState, std::string savePath)
    : world_(world), grid_(grid), zones_(zones), timeline_(timeline), monsters_(monsters), quests_(quests),
      state_(std::move(initialState)), savePath_(std::move(savePath)) {}

void GameLoop::run() {
    // Parity with the very first frame -- previously the "standing here"
    // description was always-redrawn by MapRenderer regardless of how the
    // player got there; now it's a one-time log entry, so it needs an
    // explicit push before the loop's first render.
    if (state_.mode == Mode::Overworld) {
        announceOverworldTile();
    } else {
        announceZoneTile();
    }

    for (;;) {
        if (state_.mode == Mode::Overworld) {
            render::MapRenderer::drawOverworldFrame(grid_, world_, state_, log_);
        } else {
            const world::Zone* zone = zones_.getZone(state_.currentZoneId);
            render::MapRenderer::drawZoneFrame(*zone, state_, log_);
        }

        render::Key key = render::Console::readKey();
        bool inZone = state_.mode == Mode::Zone;
        bool quit = false;

        switch (key) {
            case render::Key::North:     inZone ? tryMoveZone(0, -1) : tryMoveOverworld(0, -1); break;
            case render::Key::South:     inZone ? tryMoveZone(0, 1)  : tryMoveOverworld(0, 1);  break;
            case render::Key::East:      inZone ? tryMoveZone(1, 0)  : tryMoveOverworld(1, 0);  break;
            case render::Key::West:      inZone ? tryMoveZone(-1, 0) : tryMoveOverworld(-1, 0); break;
            case render::Key::Look:      inZone ? lookZone() : lookOverworld(); break;
            case render::Key::Talk:      handleTalk(); break;
            case render::Key::Enter:     handleEnter(); break;
            case render::Key::Sheet:     showCharacterSheet(); break;
            case render::Key::Shop:      handleShop(); break;
            case render::Key::Inventory: handleInventory(); break;
            case render::Key::Log:       handleLog(); break;
            case render::Key::Journal:   showJournal(); break;
            case render::Key::Flee:      break; // only meaningful inside runCombat's own loop
            case render::Key::Cast:      break; // only meaningful inside runCombat's own loop
            case render::Key::Rest:      handleRest(); break;
            case render::Key::BedRest:   handleBedRest(); break;
            case render::Key::Help:      showHelp(); break;
            case render::Key::Quit:      quit = true; break;
            case render::Key::Unknown:   break;
        }

        // Autosaved after every processed keypress, unconditionally -- see
        // docs/ARCHITECTURE.md. Quit falls through to here too (rather than
        // returning directly above) so the very last action is captured
        // before the process exits.
        SaveGame::save(state_, savePath_);
        if (quit) return;
    }
}

void GameLoop::pushLog(std::string text) {
    // Capped so a long session's log_ doesn't grow unbounded -- only the
    // tail is ever shown (MapRenderer's log panel), so trimming the front
    // once the cap is hit loses nothing visible.
    constexpr size_t kMaxLogEntries = 300;
    log_.push_back(std::move(text));
    if (log_.size() > kMaxLogEntries) {
        log_.erase(log_.begin(), log_.begin() + static_cast<long>(log_.size() - kMaxLogEntries));
    }
}

void GameLoop::announceOverworldTile() {
    const world::Location* here = world_.locationAt(state_.x, state_.y);
    if (here == nullptr) return; // plain terrain: the map glyph already shows it, no log spam per step
    pushLog("== " + here->name + " (" + here->region + ") ==");
    pushLog(here->description);
    // Name only -- their full flavor text is shown via lookOverworld/
    // pickAndLook on demand ('l'), not dumped here. See Milestone 43.
    for (const timeline::Presence& presence :
         timeline_.presentAt(here->id, static_cast<int>(state_.hoursElapsed / 24))) {
        pushLog(presence.character->name + " is here.");
    }
    pushLog(""); // spacer so consecutive arrivals are visually separated in the log
}

void GameLoop::announceZoneTile() {
    const world::Zone* zone = zones_.getZone(state_.currentZoneId);
    if (const world::PointOfInterest* poi = zone->poiAt(state_.zoneX, state_.zoneY)) {
        // An NPC POI (non-empty dialogue, same test handleTalk uses) only
        // gets a name -- its description shows via lookZone/pickAndLook on
        // demand instead. Scenery (empty dialogue) is unaffected: there's
        // no other way to reveal it, so it still prints immediately.
        if (poi->dialogue.empty()) {
            pushLog(poi->name + ": " + poi->description);
        } else {
            pushLog(poi->name + " is here.");
        }
        if (poi->code == zone->timelineAnchorPoi()) {
            const std::string& effectiveId =
                zone->timelineLocationId().empty() ? state_.currentZoneId : zone->timelineLocationId();
            for (const timeline::Presence& presence :
                 timeline_.presentAt(effectiveId, static_cast<int>(state_.hoursElapsed / 24))) {
                pushLog(presence.character->name + " is here.");
            }
        }
        pushLog("");
    } else if (state_.zoneX == zone->entryX() && state_.zoneY == zone->entryY()) {
        pushLog("You stand at the way back out.");
    }
}

void GameLoop::handleRest() {
    character::Character& c = state_.character;
    long long currentDay = state_.hoursElapsed / 24;
    if (c.lastRestDay == currentDay) {
        pushLog("You've already rested today.");
        return;
    }

    state_.hoursElapsed += 8; // an overnight rest -- may cross into a new day
    long long dayAfterRest = state_.hoursElapsed / 24;
    c.lastRestDay = dayAfterRest;

    int healed = std::min(1, c.maxHp - c.currentHp); // DMG p.74: 1 hp per day of rest
    c.currentHp += healed;

    std::string message = "You settle in and rest through the night.";
    message += healed > 0 ? " You recover 1 hit point." : " You were already at full health.";
    message += performSpellMemorization(dayAfterRest);
    pushLog(message);
}

void GameLoop::handleBedRest() {
    if (state_.mode != Mode::Zone) {
        pushLog("There's no bed here.");
        return;
    }
    const world::Zone* zone = zones_.getZone(state_.currentZoneId);
    const world::PointOfInterest* poi = zone->poiAt(state_.zoneX, state_.zoneY);
    if (poi == nullptr || !poi->isBed) {
        pushLog("There's no bed here.");
        return;
    }

    character::Character& c = state_.character;
    long long currentDay = state_.hoursElapsed / 24;
    if (c.lastRestDay == currentDay) {
        pushLog("You've already rested today.");
        return;
    }

    state_.hoursElapsed += 8; // same overnight cost as ordinary Rest
    long long dayAfterRest = state_.hoursElapsed / 24;
    c.lastRestDay = dayAfterRest;

    bool alreadyFull = c.currentHp >= c.maxHp;
    c.currentHp = c.maxHp; // complete bed-rest: a real bed, a full night, fully healed

    std::string message = "You spend the night resting soundly in a real bed.";
    message += alreadyFull ? " You were already at full health." : " You wake fully healed.";
    message += performSpellMemorization(dayAfterRest);
    pushLog(message);
}

std::string GameLoop::performSpellMemorization(long long dayAfterRest) {
    character::Character& c = state_.character;
    if (character::maxAccessibleSpellLevel(c) == 0) return "";

    if (c.preferredSpellIds.empty()) {
        // Never memorized anything before -- nothing to "keep the same" as,
        // so go straight to the picker.
        chooseSpellLoadout();
    } else {
        std::vector<std::string> labels = {"Yes", "No -- choose new spells"};
        int selected = 0;
        bool keepSame = true;
        for (;;) {
            render::MapRenderer::drawPickerFrame("Keep the same spells memorized?", labels, selected,
                                                  "up/down=select   Enter=choose");
            render::Key key = render::Console::readKey();
            if (key == render::Key::North || key == render::Key::South) {
                selected = selected == 0 ? 1 : 0;
            } else if (key == render::Key::Enter) {
                keepSame = (selected == 0);
                break;
            }
        }
        if (!keepSame) {
            chooseSpellLoadout();
        } else {
            // A level-up since the last rest may have opened more slots
            // than the standing loadout fills -- top up with the lowest-
            // level implemented spell rather than silently wasting slots.
            int totalSlots = 0;
            for (int lvl = 1; lvl <= character::maxAccessibleSpellLevel(c); ++lvl) {
                totalSlots += character::spellSlotsPerDay(c, lvl);
            }
            const auto& roster = character::spellListFor(c.charClass);
            if (!roster.empty()) {
                while (static_cast<int>(c.preferredSpellIds.size()) < totalSlots) {
                    c.preferredSpellIds.push_back(roster.front().id);
                }
            }
        }
    }

    character::memorizeSpells(c, dayAfterRest, c.preferredSpellIds);
    // Cleric prays, Mage studies -- distinct flavor per class. Character::
    // charClass is a single value today, so the "both" branch below can't
    // actually trigger yet, but it's written to fall out naturally the day
    // this project ever gains dual/multi-classing rather than needing a
    // rewrite then.
    bool isCleric = c.charClass == character::ClassId::Cleric;
    bool isMage = c.charClass == character::ClassId::Mage;
    if (isCleric && isMage) {
        return " You spend a quiet hour re-memorizing your prayers and incantations.";
    }
    if (isCleric) return " You rememorize your prayers.";
    return " You memorize your incantations.";
}

void GameLoop::chooseSpellLoadout() {
    character::Character& c = state_.character;
    std::vector<std::string> loadout;
    int maxLevel = character::maxAccessibleSpellLevel(c);
    const auto& roster = character::spellListFor(c.charClass);

    for (int lvl = 1; lvl <= maxLevel; ++lvl) {
        int slots = character::spellSlotsPerDay(c, lvl);
        if (slots <= 0) continue;

        std::vector<const character::SpellInfo*> choices;
        for (const auto& spell : roster) {
            if (spell.level == lvl) choices.push_back(&spell);
        }
        if (choices.empty()) continue; // nothing implemented at this level yet

        std::vector<std::string> labels;
        for (const auto* spell : choices) labels.push_back(spell->name);

        for (int slot = 0; slot < slots; ++slot) {
            int selected = 0;
            for (;;) {
                std::ostringstream title;
                title << "Level " << lvl << " spell (" << (slot + 1) << "/" << slots << ")";
                render::MapRenderer::drawPickerFrame(title.str(), labels, selected,
                                                      "up/down=select   Enter=choose");
                render::Key key = render::Console::readKey();
                if (key == render::Key::North) {
                    selected = (selected - 1 + static_cast<int>(labels.size())) % static_cast<int>(labels.size());
                } else if (key == render::Key::South) {
                    selected = (selected + 1) % static_cast<int>(labels.size());
                } else if (key == render::Key::Enter) {
                    loadout.push_back(choices[static_cast<size_t>(selected)]->id);
                    break;
                }
            }
        }
    }

    c.preferredSpellIds = std::move(loadout);
}

void GameLoop::showCharacterSheet() {
    for (;;) {
        render::MapRenderer::drawCharacterSheet(state_.character, state_.hoursElapsed / 24,
                                                 state_.hasCompanion ? &state_.companion : nullptr);
        render::Key key = render::Console::readKey();
        // 's' (South, off the sheet's own dismiss-with-any-key convention)
        // drills into the full spell roster, then loops back to the sheet
        // -- only offered to a caster, matching drawCharacterSheet's own
        // "(s=view spells known...)" hint, which only appears for one.
        if (character::canCastSpells(state_.character.charClass) && key == render::Key::South) {
            showSpellbook();
            continue;
        }
        return;
    }
}

void GameLoop::showSpellbook() {
    render::MapRenderer::drawSpellbookFrame(state_.character, state_.hoursElapsed / 24);
    render::Console::readKey(); // block for one keypress to dismiss, any key
}

void GameLoop::showHelp() {
    render::MapRenderer::drawHelpFrame();
    render::Console::readKey(); // block for one keypress to dismiss, any key
}

const world::Location* GameLoop::nearestRefuge() const {
    const world::Location* best = nullptr;
    long long bestDistSq = 0;
    for (const world::Location& loc : world_.allLocations()) {
        if (!loc.isTown && !loc.seaLocked) continue;
        long long dx = loc.x - state_.x;
        long long dy = loc.y - state_.y;
        long long distSq = dx * dx + dy * dy;
        if (best == nullptr || distSq < bestDistSq) {
            best = &loc;
            bestDistSq = distSq;
        }
    }
    return best != nullptr ? best : world_.getLocation("solace");
}

void GameLoop::tryMoveOverworld(int dx, int dy) {
    int nx = state_.x + dx;
    int ny = state_.y + dy;
    const world::TerrainInfo& terrain = world::terrainFor(grid_.terrainCodeAt(nx, ny));
    if (!terrain.passable) {
        pushLog("You cannot cross " + std::string(terrain.name) + " on foot.");
        return;
    }
    state_.x = nx;
    state_.y = ny;
    state_.minutesElapsed += terrain.minutesToCross;
    state_.hoursElapsed += state_.minutesElapsed / 60;
    state_.minutesElapsed %= 60;
    const world::Location* here = world_.locationAt(nx, ny);
    if (here != nullptr) {
        state_.visitedLocations.insert(here->id);
        checkQuestReadiness(); // a VISIT objective may have just been satisfied
    }
    announceOverworldTile();

    // Random encounters: towns/named places stay safe (here == nullptr
    // guards that), everywhere else on the overworld has a per-terrain
    // chance per move -- see docs/COMBAT_NOTES.md.
    if (here == nullptr && monsters_.size() > 0 &&
        character::roll(1, 100) <= terrain.encounterChancePercent) {
        // Distance to the nearest civilian town, so MonsterCatalog can keep
        // high-danger monsters (Ogre, higher-tier Draconians) away from
        // starting towns -- see docs/COMBAT_NOTES.md's "Town-proximity
        // monster pools". Computed inline rather than sharing nearestRefuge()
        // (line ~540): that function also counts seaLocked locations for a
        // different purpose (post-knockout respawn), which have no bearing on
        // town-proximity monster weighting, and this is its only other call
        // site.
        long long bestDistSq = -1;
        for (const world::Location& loc : world_.allLocations()) {
            if (!loc.isTown) continue;
            long long ddx = loc.x - state_.x;
            long long ddy = loc.y - state_.y;
            long long distSq = ddx * ddx + ddy * ddy;
            if (bestDistSq < 0 || distSq < bestDistSq) bestDistSq = distSq;
        }
        int townDistance = bestDistSq < 0 ? 0
                                           : static_cast<int>(std::llround(std::sqrt(static_cast<double>(bestDistSq))));
        runCombat(monsters_.randomMonster(terrain.code, townDistance));
    }
}

void GameLoop::tryMoveZone(int dx, int dy) {
    const world::Zone* zone = zones_.getZone(state_.currentZoneId);
    int nx = state_.zoneX + dx;
    int ny = state_.zoneY + dy;
    // POIs are always passable, regardless of their glyph -- zoneTileFor
    // only knows the 5 base terrain codes, so a POI character (which is
    // deliberately something else) would otherwise fall through to the
    // "unknown tile" default of impassable. See docs/ZONE_NOTES.md.
    if (zone->poiAt(nx, ny) == nullptr) {
        const world::ZoneTileInfo& tile = world::zoneTileFor(zone->tileCodeAt(nx, ny));
        if (!tile.passable) {
            pushLog("You can't walk through " + std::string(tile.name) + ".");
            return;
        }
    }
    // Deliberately does not touch hoursElapsed -- indoor shuffling isn't
    // meaningful travel time; only overworld movement advances the clock.
    state_.zoneX = nx;
    state_.zoneY = ny;
    announceZoneTile();
}

void GameLoop::lookOverworld() {
    // NPCs present at the player's own tile take priority over the
    // landmark search below -- deliberately unfiltered by
    // window->dialogue.empty() (unlike handleTalk's candidate filter),
    // matching announceOverworldTile's own unfiltered iteration: Look
    // should reveal a presence window's flavor text even if that window
    // has no SAY line authored yet. See Milestone 43.
    const world::Location* hereForLook = world_.locationAt(state_.x, state_.y);
    if (hereForLook != nullptr) {
        std::vector<LookCandidate> candidates;
        for (const timeline::Presence& presence :
             timeline_.presentAt(hereForLook->id, static_cast<int>(state_.hoursElapsed / 24))) {
            candidates.push_back({presence.character->name, presence.window->flavorText});
        }
        if (!candidates.empty()) {
            pickAndLook(candidates);
            return;
        }
    }

    const world::Location* nearest = nullptr;
    int nearestDistSq = 0;
    for (const auto& loc : world_.allLocations()) {
        if (loc.x == state_.x && loc.y == state_.y) continue; // already described by the status line
        int dx = loc.x - state_.x;
        int dy = loc.y - state_.y;
        int distSq = dx * dx + dy * dy;
        if (nearest == nullptr || distSq < nearestDistSq) {
            nearest = &loc;
            nearestDistSq = distSq;
        }
    }
    if (nearest == nullptr) {
        pushLog("Nothing notable stands out on the horizon.");
        return;
    }
    const char* dir = compassDirection(nearest->x - state_.x, nearest->y - state_.y);
    std::ostringstream oss;
    oss << "You reckon " << nearest->name << " lies to the " << dir << ".";
    pushLog(oss.str());
}

void GameLoop::lookZone() {
    // As of Milestone 43, an NPC present at the player's tile (the zone POI
    // itself, and/or -- standing on the TIMELINE_ANCHOR -- any canon
    // character the timeline places here today) is lookable, mirroring
    // handleTalk's own zone-branch candidate gathering below, but
    // unfiltered by dialogue-emptiness (see lookOverworld's comment on why).
    const world::Zone* zone = zones_.getZone(state_.currentZoneId);
    const world::PointOfInterest* poi = zone->poiAt(state_.zoneX, state_.zoneY);
    std::vector<LookCandidate> candidates;
    if (poi != nullptr && !poi->dialogue.empty()) {
        candidates.push_back({poi->name, poi->description});
    }
    if (poi != nullptr && poi->code == zone->timelineAnchorPoi()) {
        const std::string& effectiveId =
            zone->timelineLocationId().empty() ? state_.currentZoneId : zone->timelineLocationId();
        for (const timeline::Presence& presence :
             timeline_.presentAt(effectiveId, static_cast<int>(state_.hoursElapsed / 24))) {
            candidates.push_back({presence.character->name, presence.window->flavorText});
        }
    }
    if (!candidates.empty()) {
        pickAndLook(candidates);
        return;
    }

    // Deliberately minimal otherwise: unlike the overworld (a scrolling
    // camera that hides everything outside the viewport, so "look around"
    // has real work to do finding the nearest hidden landmark), a zone is
    // always rendered in full -- every POI is already visible on screen.
    // There's nothing left for a "look" action to reveal.
    pushLog("Nothing else catches your eye here.");
}

void GameLoop::handleTalk() {
    std::vector<TalkCandidate> candidates;

    if (state_.mode == Mode::Overworld) {
        const world::Location* here = world_.locationAt(state_.x, state_.y);
        if (here != nullptr) {
            // Same query drawOverworldFrame already makes for the passive
            // flavor line -- see docs/TIMELINE_NOTES.md.
            int dayNow = static_cast<int>(state_.hoursElapsed / 24);
            for (const auto& presence : timeline_.presentAt(here->id, dayNow)) {
                if (!presence.window->dialogue.empty()) {
                    candidates.push_back({presence.character->id, presence.character->name,
                                           speechFromWindow(timeline_, *presence.character, *presence.window, dayNow)});
                }
            }
        }
    } else {
        const world::Zone* zone = zones_.getZone(state_.currentZoneId);
        const world::PointOfInterest* poi = zone->poiAt(state_.zoneX, state_.zoneY);
        // The location whose schedule this zone checks -- shared by the
        // TIMELINE_ANCHOR branch below and the aftermath-dialogue check
        // above it, since "have the Heroes moved on?" is a fact about the
        // whole zone, not any one POI. See docs/TIMELINE_NOTES.md /
        // docs/ZONE_NOTES.md's "Aftermath dialogue" section.
        const std::string& effectiveId =
            zone->timelineLocationId().empty() ? state_.currentZoneId : zone->timelineLocationId();
        int dayNow = static_cast<int>(state_.hoursElapsed / 24);
        if (poi != nullptr && !poi->dialogue.empty()) {
            const std::string* questId = zone->questAt(state_.zoneX, state_.zoneY);
            const world::BoatVoyage* boat = zone->boatAt(state_.zoneX, state_.zoneY);
            TalkCandidate candidate{state_.currentZoneId + ":" + std::string(1, poi->code), poi->name,
                                     speechFromPoi(*poi),
                                     boat != nullptr ? boat->destinationLocationId : std::string(),
                                     boat != nullptr ? boat->hours : 0,
                                     questId != nullptr ? *questId : std::string(),
                                     poi->grantsItemId, poi->grantsItemName};
            candidate.recruitsCompanion = poi->recruitsCompanion;
            if (!poi->dialogueAfter.empty()) {
                int latestDayEnd = timeline_.latestDayEnd(effectiveId);
                if (latestDayEnd >= 0 && dayNow > latestDayEnd) {
                    candidate.dialogueAfter = poi->dialogueAfter;
                }
            }
            if (!poi->dialogueBefore.empty()) {
                int earliestDayStart = timeline_.earliestDayStart(effectiveId);
                if (earliestDayStart >= 0 && dayNow < earliestDayStart) {
                    candidate.dialogueBefore = poi->dialogueBefore;
                }
            }
            candidates.push_back(std::move(candidate));
        }
        // Zone-interior encounters (Milestone 23): standing on this zone's
        // TIMELINE_ANCHOR tile also makes any canon character the timeline
        // places at its effective location today talkable -- using the
        // character's own stable id (not the zone-synthesized id above),
        // so "have I met them" carries over correctly from the overworld.
        // See docs/TIMELINE_NOTES.md.
        if (poi != nullptr && poi->code == zone->timelineAnchorPoi()) {
            for (const auto& presence : timeline_.presentAt(effectiveId, dayNow)) {
                if (!presence.window->dialogue.empty()) {
                    candidates.push_back({presence.character->id, presence.character->name,
                                           speechFromWindow(timeline_, *presence.character, *presence.window, dayNow)});
                }
            }
        }
    }

    pickAndTalk(candidates);
}

void GameLoop::pickAndTalk(const std::vector<TalkCandidate>& candidates) {
    if (candidates.empty()) {
        pushLog("There's no one here to talk to.");
        return;
    }
    if (candidates.size() == 1) {
        talkTo(candidates.front());
        return;
    }

    // More than one candidate at once (e.g. all 8 Heroes sharing a
    // schedule) -- ask which one rather than dumping every line at once.
    // Same nested-loop, local North/South/Enter/Quit reinterpretation
    // shape as handleShop.
    std::vector<std::string> names;
    for (const TalkCandidate& c : candidates) names.push_back(c.name);
    int selected = 0;
    for (;;) {
        render::MapRenderer::drawPickerFrame("Talk to whom?", names, selected,
                                              "up/down=select   Enter=talk   q=cancel");
        render::Key key = render::Console::readKey();
        if (key == render::Key::North) {
            selected = (selected - 1 + static_cast<int>(names.size())) % static_cast<int>(names.size());
        } else if (key == render::Key::South) {
            selected = (selected + 1) % static_cast<int>(names.size());
        } else if (key == render::Key::Enter) {
            // Deliberately no return here: talkTo runs its own dialogue/topic
            // loop and returns whenever the player backs out of it (topic
            // menu's "Nothing, thanks"/Quit, or no topics at all) -- looping
            // back to this same picker instead of falling through to the
            // explore screen lets the player talk to a second present NPC
            // without re-pressing Talk and re-triggering candidate lookup.
            talkTo(candidates[selected]);
        } else if (key == render::Key::Quit) {
            return;
        }
    }
}

void GameLoop::pickAndLook(const std::vector<LookCandidate>& candidates) {
    // Callers (lookOverworld/lookZone) only invoke this once they've
    // already confirmed candidates is non-empty.
    if (candidates.size() == 1) {
        const LookCandidate& c = candidates.front();
        render::MapRenderer::drawDialogueFrame({{c.name, c.description}});
        render::Console::readKey(); // block for one keypress to dismiss, any key
        return;
    }

    // Same nested-loop, local North/South/Enter/Quit reinterpretation
    // shape as pickAndTalk above.
    std::vector<std::string> names;
    for (const LookCandidate& c : candidates) names.push_back(c.name);
    int selected = 0;
    for (;;) {
        render::MapRenderer::drawPickerFrame("Look at whom?", names, selected,
                                              "up/down=select   Enter=look   q=cancel");
        render::Key key = render::Console::readKey();
        if (key == render::Key::North) {
            selected = (selected - 1 + static_cast<int>(names.size())) % static_cast<int>(names.size());
        } else if (key == render::Key::South) {
            selected = (selected + 1) % static_cast<int>(names.size());
        } else if (key == render::Key::Enter) {
            const LookCandidate& c = candidates[selected];
            render::MapRenderer::drawDialogueFrame({{c.name, c.description}});
            render::Console::readKey();
            return;
        } else if (key == render::Key::Quit) {
            return;
        }
    }
}

void GameLoop::talkTo(const TalkCandidate& candidate) {
    const std::string& id = candidate.id;
    const std::string& name = candidate.name;
    const Speech& speech = candidate.speech;
    std::string text;
    // Aftermath dialogue (see docs/ZONE_NOTES.md) is checked before the
    // ordinary alreadyMet branch below, and tracked under its own id, so it
    // fires the first time the Heroes' window has closed regardless of
    // whether this NPC was already met beforehand -- a player who talked to
    // Otik on day 0, before the Heroes ever arrived, still hears about their
    // departure the next time they visit after day 1.
    std::string afterId = id + ":after";
    bool showAfter = !candidate.dialogueAfter.empty() && state_.metCharacters.count(afterId) == 0;
    bool alreadyMet = state_.metCharacters.count(id) > 0;
    // Anticipation dialogue (see docs/ZONE_NOTES.md) takes the same
    // "shown instead of the ordinary flow" precedence as aftermath dialogue
    // above, but with no id of its own to track: it's an ongoing truth, not
    // a one-time event, so it's meant to repeat on every visit until the
    // condition (dayNow < earliestDayStart) stops holding on its own.
    if (showAfter) {
        text = candidate.dialogueAfter;
        state_.metCharacters.insert(afterId);
    } else if (!candidate.dialogueBefore.empty()) {
        text = candidate.dialogueBefore;
    } else if (alreadyMet) {
        text = !speech.again.empty() ? speech.again
                                      : (name + " catches your eye and gives a small nod of recognition.");
    } else {
        text = speech.greeting;
        for (const auto& [condition, conditionalText] : speech.conditional) {
            if (conditionMatches(condition, state_.character)) {
                text = conditionalText;
                break;
            }
        }
    }
    render::MapRenderer::drawDialogueFrame({{name, text}});
    render::Console::readKey(); // block for one keypress to dismiss, any key
    state_.metCharacters.insert(id);
    checkQuestReadiness(); // a TALK objective may have just been satisfied
    if (!candidate.grantsItemId.empty() &&
        character::findQuestItemIndex(state_.character, candidate.grantsItemId) < 0) {
        state_.character.inventory.push_back(character::InventoryItem{
            character::ItemKind::QuestItem, character::ArmorId::None, "", 0, 0, 0,
            candidate.grantsItemId, candidate.grantsItemName});
        pushLog("You've picked up " + candidate.grantsItemName + ".");
    }
    if (!candidate.questId.empty()) {
        offerOrTurnInQuest(candidate.questId, name);
    }
    // A scripted one-time voyage (Milestone 36, reworked; Milestone 92 added
    // a real decline option). Offered on every talk until actually boarded
    // -- gated on voyagesTaken rather than alreadyMet, since alreadyMet is
    // set unconditionally above and declining must not burn the offer.
    // Mirrors offerOrTurnInQuest's Accept/Decline picker below.
    if (!candidate.boatDestinationId.empty() && state_.voyagesTaken.count(id) == 0) {
        const world::Location* destination = world_.getLocation(candidate.boatDestinationId);
        if (destination == nullptr) return; // defensive -- ZoneCatalog::loadForWorld already validated this id
        std::vector<std::string> labels = {"Board", "Not yet"};
        int selected = 0;
        bool board = false;
        for (;;) {
            render::MapRenderer::drawPickerFrame("Depart for " + destination->name + "?", labels, selected,
                                                  "up/down=select   Enter=choose   q=cancel");
            render::Key key = render::Console::readKey();
            if (key == render::Key::North || key == render::Key::South) {
                selected = selected == 0 ? 1 : 0;
            } else if (key == render::Key::Enter) {
                board = (selected == 0);
                break;
            } else if (key == render::Key::Quit) {
                break;
            }
        }
        if (board) {
            state_.voyagesTaken.insert(id);
            // Computed before x/y are overwritten below -- see
            // compassDirection's own note on dy sign.
            const char* dir = compassDirection(destination->x - state_.x, destination->y - state_.y);
            state_.mode = Mode::Overworld;
            state_.currentZoneId.clear();
            state_.zoneStack.clear();
            state_.x = destination->x;
            state_.y = destination->y;
            state_.hoursElapsed += candidate.boatHours;
            state_.visitedLocations.insert(destination->id);
            pushLog("You board the ship, and it carries you " + std::string(dir) +
                    " across open water. Days pass before " + destination->name +
                    " finally rises out of the fog.");
            checkQuestReadiness(); // a VISIT objective may have just been satisfied
            announceOverworldTile();
            return;
        }
        // "Not yet"/q -- ends up here, falling through to the ordinary
        // topics/SUBJECT picker below, same as any other POI's declined
        // offer (see offerOrTurnInQuest).
    }
    // Milestone 116 Phase 1's one recruitable companion -- same Accept/
    // Decline picker shape as the BOAT block above, gated on state_.
    // hasCompanion (not a per-candidate "already offered" set, since there's
    // exactly one companion slot this phase) so a declined offer stays
    // re-offerable on a later visit, and an accepted one never re-offers.
    // See character::buildCompanion() and docs/COMBAT_NOTES.md's "Extending
    // this later".
    if (candidate.recruitsCompanion && !state_.hasCompanion) {
        std::vector<std::string> labels = {"Join me", "Not yet"};
        int selected = 0;
        bool join = false;
        for (;;) {
            render::MapRenderer::drawPickerFrame("Ask " + name + " to join your journey?", labels, selected,
                                                  "up/down=select   Enter=choose   q=cancel");
            render::Key key = render::Console::readKey();
            if (key == render::Key::North || key == render::Key::South) {
                selected = selected == 0 ? 1 : 0;
            } else if (key == render::Key::Enter) {
                join = (selected == 0);
                break;
            } else if (key == render::Key::Quit) {
                break;
            }
        }
        if (join) {
            state_.hasCompanion = true;
            state_.companion = character::buildCompanion();
            pushLog(name + " joins your party.");
        }
        // "Not yet"/q -- falls through to the ordinary topics/SUBJECT picker
        // below, same as BOAT's decline path above.
    }

    bool canAskAnything = !speech.subjects.empty();
    if (!speech.topics.empty() || canAskAnything) {
        std::vector<std::string> labels;
        for (const auto& [label, topicText] : speech.topics) labels.push_back(label);
        // "Ask about something else..." (free-text) sits between the
        // curated topics and "Nothing, thanks" -- only shown when this
        // window/POI has any SUBJECT content at all. See
        // docs/TIMELINE_NOTES.md / docs/ZONE_NOTES.md's "Ask about
        // anything".
        int askAnythingIndex = -1;
        if (canAskAnything) {
            askAnythingIndex = static_cast<int>(labels.size());
            labels.push_back("Ask about something else...");
        }
        int nothingThanksIndex = static_cast<int>(labels.size());
        labels.push_back("Nothing, thanks");
        int selected = 0;
        for (;;) {
            render::MapRenderer::drawPickerFrame("Ask " + name + " about...", labels, selected,
                                                  "up/down=select   Enter=ask   q=leave");
            render::Key key = render::Console::readKey();
            if (key == render::Key::North) {
                selected = (selected - 1 + static_cast<int>(labels.size())) % static_cast<int>(labels.size());
            } else if (key == render::Key::South) {
                selected = (selected + 1) % static_cast<int>(labels.size());
            } else if (key == render::Key::Enter) {
                if (selected == nothingThanksIndex) return;
                if (selected == askAnythingIndex) {
                    // Display-only hint labels so the player isn't guessing
                    // blind -- keywords are authored lowercase for
                    // case-insensitive matchSubject lookups (see
                    // docs/TIMELINE_NOTES.md's "Ask about anything"), not
                    // for display, so the canonical first keyword gets its
                    // first letter capitalized here.
                    std::vector<std::string> hints;
                    for (const auto& subject : speech.subjects) {
                        std::string hint = subject.keywords.front();
                        if (!hint.empty()) {
                            hint[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(hint[0])));
                        }
                        hints.push_back(hint);
                    }
                    render::MapRenderer::drawAskInputFrame(name, hints);
                    std::string input = render::Console::readLine(60);
                    if (!input.empty()) {
                        const Speech::SubjectEntry* match = matchSubject(speech.subjects, input);
                        std::string response;
                        if (match != nullptr) {
                            response = match->text;
                        } else if (!speech.subjectUnknown.empty()) {
                            response = speech.subjectUnknown;
                        } else {
                            response = name + " gives you a blank look. \"I'm not sure what you mean by that.\"";
                        }
                        render::MapRenderer::drawDialogueFrame({{name, response}});
                        render::Console::readKey();
                    }
                    continue;
                }
                render::MapRenderer::drawDialogueFrame({{name, speech.topics[selected].second}});
                render::Console::readKey();
            } else if (key == render::Key::Quit) {
                return;
            }
        }
    }
}

void GameLoop::offerOrTurnInQuest(const std::string& questId, const std::string& speakerName) {
    const quest::Quest* q = quests_.find(questId);
    if (q == nullptr) return; // defensive -- main.cpp already validated every zone QUEST id at startup

    auto it = state_.quests.find(questId);
    if (it == state_.quests.end()) {
        // Not started -- REQUIRE (if any) gates whether this quest is even
        // offered, same condition vocabulary as SAY_IF (see
        // docs/QUEST_NOTES.md). An unmet REQUIRE means this POI has nothing
        // to say about it -- the greeting shown just before this call is
        // all the player sees.
        if (!q->requirement.empty() && !conditionMatches(q->requirement, state_.character)) return;

        render::MapRenderer::drawDialogueFrame({{speakerName, q->offerText}});
        render::Console::readKey();

        std::vector<std::string> labels = {"Accept", "Decline"};
        int selected = 0;
        for (;;) {
            render::MapRenderer::drawPickerFrame(q->name, labels, selected, "up/down=select   Enter=choose   q=cancel");
            render::Key key = render::Console::readKey();
            if (key == render::Key::North || key == render::Key::South) {
                selected = selected == 0 ? 1 : 0;
            } else if (key == render::Key::Enter) {
                if (selected == 0) {
                    state_.quests[questId] = QuestStatus::Active;
                    render::MapRenderer::drawDialogueFrame({{speakerName, q->acceptText}});
                    render::Console::readKey();
                    pushLog("Quest accepted: " + q->name + ".");
                    // Catches the "already did it before being asked" case
                    // (see quest/Quest.h) -- without this, a quest accepted
                    // in a state that already satisfies every objective
                    // would sit at Active forever, since nothing else would
                    // ever trigger a readiness check for it again.
                    checkQuestReadiness();
                }
                return;
            } else if (key == render::Key::Quit) {
                return; // declined -- re-offerable next time, nothing recorded
            }
        }
    }

    if (it->second == QuestStatus::Complete) return; // already turned in -- nothing more to say here

    if (it->second == QuestStatus::Active) {
        // Not yet ready -- checkQuestReadiness (called after every visit/
        // talk/kill) would already have promoted this to ReadyToTurnIn if
        // every objective were met, so reaching here means it's genuinely
        // not done yet.
        render::MapRenderer::drawDialogueFrame({{speakerName, q->progressText}});
        render::Console::readKey();
        return;
    }

    // ReadyToTurnIn -- turn in.
    render::MapRenderer::drawDialogueFrame({{speakerName, q->completeText}});
    render::Console::readKey();
    // Hand over any delivered quest items -- the literal "give it to the
    // giver" for each Deliver objective, mirroring rewardSolamnicArmor's
    // add-to-inventory below in reverse. allObjectivesMet already confirmed
    // every one of these is actually carried before this branch is reached.
    for (const auto& objective : q->objectives) {
        if (objective.kind != quest::ObjectiveKind::Deliver) continue;
        for (int i = 0; i < objective.count; ++i) {
            int index = character::findQuestItemIndex(state_.character, objective.targetId);
            if (index >= 0) state_.character.inventory.erase(state_.character.inventory.begin() + index);
        }
    }
    state_.character.steelPieces += q->rewardSteel;
    if (q->rewardXp > 0) {
        state_.character.experience += q->rewardXp;
        // First call site of applyPendingLevelUps outside runCombat -- a
        // level-up can now happen mid-conversation. Its messages go to the
        // persistent log_, not the dialogue frame that just closed.
        character::applyPendingLevelUps(state_.character, log_);
    }
    state_.quests[questId] = QuestStatus::Complete;
    std::ostringstream rewardMsg;
    rewardMsg << "Quest complete: " << q->name << ".";
    if (q->rewardSteel > 0) rewardMsg << " +" << q->rewardSteel << " steel.";
    if (q->rewardXp > 0) rewardMsg << " +" << q->rewardXp << " XP.";
    pushLog(rewardMsg.str());
    if (q->rewardKnightSword) {
        state_.character.knightOrder = character::KnightOrder::Sword;
        pushLog("You are named a Knight of the Sword.");
    }
    if (q->rewardSolamnicArmor) {
        // See character::ArmorId::SolamnicArmor and docs/CHARACTER_NOTES.md's
        // "Magic items" -- an ordinary Shield accompanies it, a deliberate
        // simplification of the book's separate "shield +1" (this engine's
        // shield has no enchantment tiers of its own).
        state_.character.inventory.push_back(
            character::InventoryItem{character::ItemKind::Armor, character::ArmorId::SolamnicArmor, "", 0, 0});
        state_.character.inventory.push_back(
            character::InventoryItem{character::ItemKind::Shield, character::ArmorId::None, "", 0, 0});
        pushLog("You are granted Solamnic Armor and a Knight's shield. Press 'i' to equip them.");
    }
    if (q->rewardKnightRose) {
        state_.character.knightOrder = character::KnightOrder::Rose;
        pushLog("You are named a Knight of the Rose.");
    }
    if (q->rewardStaffOfStrikingCuring) {
        // See character::kStaffOfStrikingCuringName and docs/
        // CHARACTER_NOTES.md's "Magic items" -- granted to inventory, not
        // auto-equipped, same as Solamnic Armor above.
        state_.character.inventory.push_back(character::InventoryItem{
            character::ItemKind::Weapon, character::ArmorId::None, character::kStaffOfStrikingCuringName,
            character::kStaffDamageSides, 0, character::kStaffMagicBonus});
        pushLog("You are granted the Staff of Striking/Curing. Press 'i' to equip it.");
    }
    if (q->rewardFrostreaver) {
        // weaponMagicBonus is 0 here on purpose -- the Frostreaver's +4
        // only applies while standing on glacier terrain, applied as a
        // this-fight-only local bonus in runCombat, not baked into the
        // item itself. See character::kFrostreaverMagicBonus.
        state_.character.inventory.push_back(character::InventoryItem{
            character::ItemKind::Weapon, character::ArmorId::None, character::kFrostreaverName,
            character::kFrostreaverDamageSides, 0, 0});
        pushLog("You are granted a Frostreaver. Press 'i' to equip it.");
    }
}

void GameLoop::checkQuestReadiness() {
    for (auto& [questId, status] : state_.quests) {
        if (status != QuestStatus::Active) continue;
        const quest::Quest* q = quests_.find(questId);
        if (q == nullptr) continue; // defensive -- main.cpp already validated every zone QUEST id at startup
        if (!allObjectivesMet(*q, state_)) continue;
        status = QuestStatus::ReadyToTurnIn;
        pushLog(q->name + " is ready to turn in -- return to " + q->giver + " to collect your reward.");
    }
}

void GameLoop::showJournal() {
    std::vector<render::MapRenderer::JournalEntry> entries;
    for (const auto& [questId, status] : state_.quests) {
        const quest::Quest* q = quests_.find(questId);
        if (q == nullptr) continue; // defensive -- shouldn't happen, main.cpp validates at startup

        render::MapRenderer::JournalEntry entry;
        entry.title = q->name;
        entry.complete = status == QuestStatus::Complete;
        if (status == QuestStatus::ReadyToTurnIn) {
            entry.objectiveLines.push_back("Ready to turn in! Return to " + q->giver + ".");
        }
        for (const auto& objective : q->objectives) {
            bool met = entry.complete || status == QuestStatus::ReadyToTurnIn || objectiveMet(objective, state_);
            std::ostringstream line;
            line << (met ? "[x] " : "[ ] ") << objective.label;
            if (objective.kind == quest::ObjectiveKind::Slay && !met) {
                line << " (" << objectiveProgress(objective, state_) << "/" << objective.count << ")";
            }
            entry.objectiveLines.push_back(line.str());
        }
        entries.push_back(std::move(entry));
    }

    render::MapRenderer::drawJournalFrame(entries);
    render::Console::readKey(); // block for one keypress to dismiss, any key
}

void GameLoop::handleShop() {
    if (state_.mode != Mode::Zone) {
        pushLog("There's nothing to buy here.");
        return;
    }
    const world::Zone* zone = zones_.getZone(state_.currentZoneId);
    const world::PointOfInterest* poi = zone->poiAt(state_.zoneX, state_.zoneY);
    if (poi == nullptr || !poi->isShop) {
        pushLog("There's nothing to buy here.");
        return;
    }
    // SHOP_LOCKED (see docs/ZONE_NOTES.md) -- a shop can require a quest
    // to be Complete before it opens at all, e.g. Flint's Smithy waiting
    // on ore_for_the_forge. Checked here rather than baked into isShop
    // itself so the POI's own TALK/TOPIC content stays fully reachable
    // regardless of lock state.
    if (const std::string* requiredQuestId = zone->shopLockAt(state_.zoneX, state_.zoneY)) {
        auto it = state_.quests.find(*requiredQuestId);
        if (it == state_.quests.end() || it->second != QuestStatus::Complete) {
            pushLog("There's nothing to buy here yet.");
            return;
        }
    }
    // Translates the zone file's plain catalog name into
    // character::ShopCatalog once per shop visit -- see shopCatalogFor.
    character::ShopCatalog catalog = shopCatalogFor(poi->shopCatalog);

    int selected = 0;
    bool sellMode = false;
    std::string shopMessage;
    for (;;) {
        std::vector<character::ShopItem> buyItems = character::availableShopItems(state_.character, catalog);
        std::vector<character::SellItem> sellItems = character::sellableItems(state_.character);
        size_t activeSize = sellMode ? sellItems.size() : buyItems.size();
        render::MapRenderer::drawShopFrame(state_.character, poi->name, buyItems, sellItems, sellMode,
                                            selected, shopMessage);
        render::Key key = render::Console::readKey();

        // Reinterprets North/South/Enter/Inventory/Quit locally rather
        // than adding new Key values -- same trick runCombat already uses
        // for Flee inside its own nested loop (see docs/GOTCHAS.md).
        // Key::Quit here exits the shop, not the whole game; Key::Inventory
        // ('i') toggles the buy/sell view rather than opening the real
        // inventory screen.
        if (key == render::Key::North) {
            selected = activeSize == 0 ? 0 : static_cast<int>((selected - 1 + activeSize) % activeSize);
        } else if (key == render::Key::South) {
            selected = activeSize == 0 ? 0 : static_cast<int>((selected + 1) % activeSize);
        } else if (key == render::Key::Enter) {
            if (sellMode) {
                if (!sellItems.empty()) {
                    character::PurchaseResult result = character::sellItem(state_.character, selected);
                    shopMessage = result.message;
                }
            } else if (!buyItems.empty()) {
                character::PurchaseResult result = character::purchaseItem(state_.character, selected, catalog);
                shopMessage = result.message;
            }
        } else if (key == render::Key::Inventory) {
            sellMode = !sellMode;
            selected = 0;
            shopMessage.clear();
        } else if (key == render::Key::Quit) {
            return;
        }
    }
}

void GameLoop::handleInventory() {
    int selected = 0;
    for (;;) {
        auto& inventory = state_.character.inventory;
        render::MapRenderer::drawInventoryFrame(state_.character, selected);
        render::Key key = render::Console::readKey();

        // Same North/South/Enter/Quit local reinterpretation as handleShop.
        if (key == render::Key::North) {
            selected = inventory.empty() ? 0 : (selected - 1 + static_cast<int>(inventory.size())) % static_cast<int>(inventory.size());
        } else if (key == render::Key::South) {
            selected = inventory.empty() ? 0 : (selected + 1) % static_cast<int>(inventory.size());
        } else if (key == render::Key::Enter) {
            if (!inventory.empty()) {
                // A Potion is drunk, not equipped -- everything else keeps
                // the existing equip behavior. See character::drinkPotion.
                // Webnet/Brooch of Imog are combat-only (see
                // GameLoop::runCombat) -- neither drinkable nor equippable
                // here, so Enter just explains that instead of silently
                // no-opping through equipInventoryItem.
                character::ItemKind kind = inventory[static_cast<size_t>(selected)].kind;
                if (kind == character::ItemKind::Potion) {
                    character::PurchaseResult result = character::drinkPotion(state_.character, selected);
                    pushLog(result.message);
                } else if (kind == character::ItemKind::Webnet || kind == character::ItemKind::BroochOfImog) {
                    pushLog("That can only be used in combat.");
                } else if (kind == character::ItemKind::QuestItem) {
                    pushLog("That's meant for someone else -- you'll need to deliver it.");
                } else {
                    character::equipInventoryItem(state_.character, selected);
                }
                selected = 0; // the list just changed shape -- reset the cursor
            }
        } else if (key == render::Key::Quit) {
            return;
        }
    }
}

void GameLoop::handleLog() {
    // Scrolls by a fixed chunk rather than one physical line per keypress
    // -- a long session's log_ can wrap to hundreds of lines, and North/
    // South are the only scroll input available (no dedicated page keys).
    constexpr int kLogScrollStep = 10;
    int scrollOffset = -1; // sentinel: drawLogFrame starts at the bottom (most recent) on the first draw
    for (;;) {
        scrollOffset = render::MapRenderer::drawLogFrame(log_, scrollOffset);
        render::Key key = render::Console::readKey();

        // Same local-reinterpretation trick as handleShop/handleInventory
        // -- Key::Quit here returns to the live view, not the whole game;
        // Key::Log ('v') also closes it, so the key that opened it closes
        // it too.
        if (key == render::Key::North) {
            scrollOffset = std::max(0, scrollOffset - kLogScrollStep);
        } else if (key == render::Key::South) {
            scrollOffset += kLogScrollStep; // drawLogFrame clamps to the real max on the next call
        } else if (key == render::Key::Quit || key == render::Key::Log) {
            return;
        }
    }
}

void GameLoop::handleEnter() {
    if (state_.mode == Mode::Overworld) {
        const world::Location* here = world_.locationAt(state_.x, state_.y);
        if (here == nullptr) {
            pushLog("There is nothing here to step into.");
            return;
        }
        const world::Zone* zone = zones_.getZone(here->id);
        if (zone == nullptr) {
            pushLog("There is nothing to explore inside " + here->name + " yet.");
            return;
        }
        state_.mode = Mode::Zone;
        state_.currentZoneId = here->id;
        state_.zoneX = zone->entryX();
        state_.zoneY = zone->entryY();
        pushLog("You step into " + here->name + ".");
        announceZoneTile();
        return;
    }

    // Mode::Zone
    const world::Zone* zone = zones_.getZone(state_.currentZoneId);

    // Stepping onto a portal tile (e.g. the Inn's door) descends into a
    // nested zone. The current zone/position is remembered on zoneStack so
    // leaving the nested zone returns here, not to the overworld.
    if (const std::string* target = zone->portalAt(state_.zoneX, state_.zoneY)) {
        const world::Zone* targetZone = zones_.getZone(*target);
        state_.zoneStack.push_back({state_.currentZoneId, state_.zoneX, state_.zoneY});
        state_.currentZoneId = *target;
        state_.zoneX = targetZone->entryX();
        state_.zoneY = targetZone->entryY();
        pushLog("You step into " + targetZone->name() + ".");
        announceZoneTile();
        return;
    }

    if (state_.zoneX != zone->entryX() || state_.zoneY != zone->entryY()) {
        pushLog("You need to be at the entrance (marked '>') to leave.");
        return;
    }

    if (!state_.zoneStack.empty()) {
        ZoneReturnPoint back = state_.zoneStack.back();
        state_.zoneStack.pop_back();
        pushLog("You step back out into " + zones_.getZone(back.zoneId)->name() + ".");
        state_.currentZoneId = back.zoneId;
        state_.zoneX = back.x;
        state_.zoneY = back.y;
        announceZoneTile();
        return;
    }

    const world::Location* here = world_.getLocation(state_.currentZoneId);
    pushLog("You step back out into " + (here != nullptr ? here->name : "the world") + ".");
    state_.mode = Mode::Overworld;
    state_.currentZoneId.clear();
    announceOverworldTile();
}

void GameLoop::runCombat(const combat::Monster& monster) {
    // How many of this monster showed up this encounter -- see
    // combat::rollGroupSize and docs/COMBAT_NOTES.md's "Monster encounter
    // groups" section. Every monster without a GROUP line in
    // data/monsters.txt has groupMin == groupMax == 1, so this always
    // rolls exactly 1 for the vast majority of the roster.
    int groupSize = combat::rollGroupSize(monster);
    struct MonsterInstance {
        int hp;
        int maxHp;
    };
    std::vector<MonsterInstance> instances(static_cast<size_t>(groupSize));
    for (MonsterInstance& instance : instances) {
        instance.maxHp = character::roll(monster.hpDiceCount, monster.hpDiceSides) + monster.hpFlatBonus;
        instance.hp = instance.maxHp;
    }
    // Decided once from the STARTING group size, not the live alive count --
    // so a group fight's labels stay "Goblin A"/"Goblin B" consistently even
    // as members die, while every solo fight (groupSize == 1, still the
    // overwhelming majority of encounters) keeps the exact pre-Milestone-113
    // wording below with no letter at all.
    const bool useLetters = instances.size() > 1;

    // Terrain lookup, lifted out of the Frostreaver-only check further
    // below so the combat grid's backdrop can reuse it too (Milestone
    // 114) -- runCombat's only call site is tryMoveOverworld, so
    // state_.x/state_.y are always the tile this fight is happening on.
    const world::TerrainInfo& hereTerrain = world::terrainFor(grid_.terrainCodeAt(state_.x, state_.y));

    // Starting layout (Milestone 114, see docs/COMBAT_NOTES.md's
    // "Positional combat grid" section): player near the bottom-center,
    // monster instances spread evenly across a row a few tiles above --
    // close enough that a melee character reaches combat within a couple
    // of rounds, far enough that positioning/ranged options are real.
    // Purely local to this one runCombat call, never touching
    // GameState/SaveGame, same "combat isn't saved" precedent as
    // instances/log above.
    combat::GridPos playerPos{render::MapRenderer::kCombatGridWidth / 2, render::MapRenderer::kCombatGridHeight - 2};
    std::vector<combat::GridPos> instancePositions(instances.size());
    {
        constexpr int kMonsterSpacing = 2;
        const int centerX = render::MapRenderer::kCombatGridWidth / 2;
        const int count = static_cast<int>(instances.size());
        for (int i = 0; i < count; ++i) {
            int offsetIndex = i - (count - 1) / 2;
            instancePositions[static_cast<size_t>(i)] = {centerX + offsetIndex * kMonsterSpacing, 2};
        }
    }

    // Declared here (rather than down where it's first populated, below)
    // so pickTarget -- which needs to pass it to drawCombatFrame as of
    // Milestone 115's in-frame targeting -- can see it: a lambda body only
    // sees names already declared earlier in the source, same ordering
    // constraint runCombat's other lambdas already have to satisfy (see
    // docs/ARCHITECTURE.md). Left undeclared here, `log` inside pickTarget
    // would silently resolve to <cmath>'s std::log overload set instead of
    // this vector and fail to compile.
    std::vector<std::string> log;
    auto monsterLabel = [&](int idx) -> std::string {
        if (!useLetters) return monster.name;
        return monster.name + " " + std::string(1, static_cast<char>('A' + idx));
    };
    auto buildViews = [&]() {
        std::vector<render::MapRenderer::CombatMonsterView> views;
        for (size_t i = 0; i < instances.size(); ++i) {
            views.push_back({monsterLabel(static_cast<int>(i)), std::max(0, instances[i].hp), instances[i].maxHp,
                              monster.armorClass, instances[i].hp > 0, instancePositions[i],
                              static_cast<char>('A' + i)});
        }
        return views;
    };
    auto aliveCount = [&]() {
        int count = 0;
        for (const MonsterInstance& instance : instances) {
            if (instance.hp > 0) ++count;
        }
        return count;
    };
    // Target selection is driven by how many instances are CURRENTLY alive
    // (filtered further by `eligible`, e.g. adjacency for a melee weapon --
    // Milestone 114), not the group size chosen at the start -- so a solo
    // fight and a group fight fought down to its last eligible survivor
    // both auto-target the same way, and the picker only ever appears
    // while 2+ eligible instances are still standing. Returns -1 if
    // nothing is eligible, or if allowCancel and the player backs out via
    // Quit (used for spellcasting, where "cast at someone" can still be
    // reconsidered the same way choosing which spell already can);
    // attacking never allows cancellation, since Enter has always
    // committed to attacking.
    auto pickTarget = [&](const std::string& title, bool allowCancel, const std::function<bool(int)>& eligible) -> int {
        std::vector<int> candidates;
        for (size_t i = 0; i < instances.size(); ++i) {
            if (instances[i].hp <= 0) continue;
            if (!eligible(static_cast<int>(i))) continue;
            candidates.push_back(static_cast<int>(i));
        }
        if (candidates.empty()) return -1;
        if (candidates.size() == 1) return candidates.front();
        // Milestone 115: the grid itself is the picker (see
        // docs/COMBAT_NOTES.md's "In-frame combat actions" section, and
        // render::MapRenderer::CombatPrompt's own doc comment) -- this
        // redraws the ordinary drawCombatFrame with the candidate under
        // `selected` bracketed on the grid and marked in the HP roster,
        // instead of clearing the screen for a separate drawPickerFrame
        // list, so the map/roster/log stay visible the whole time. up/down
        // still just cycle `candidates` in the same fixed order as before;
        // only the rendering changed.
        int selected = 0;
        for (;;) {
            render::MapRenderer::CombatPrompt prompt;
            prompt.title = title;
            prompt.selected = selected;
            prompt.gridCursorIndex = candidates[static_cast<size_t>(selected)];
            prompt.footer = allowCancel ? "up/down=select   Enter=choose   q=cancel" : "up/down=select   Enter=choose";
            render::MapRenderer::drawCombatFrame(state_.character, buildViews(), log, state_.hoursElapsed / 24,
                                                  hereTerrain, playerPos, prompt);
            render::Key pickKey = render::Console::readKey();
            if (pickKey == render::Key::North) {
                selected = (selected - 1 + static_cast<int>(candidates.size())) % static_cast<int>(candidates.size());
            } else if (pickKey == render::Key::South) {
                selected = (selected + 1) % static_cast<int>(candidates.size());
            } else if (pickKey == render::Key::Enter) {
                return candidates[static_cast<size_t>(selected)];
            } else if (allowCancel && pickKey == render::Key::Quit) {
                return -1;
            }
        }
    };
    // Always-eligible filter for position-independent target selection
    // (spells, Webnet) -- see docs/COMBAT_NOTES.md's "Positional combat
    // grid" section on why those stay unaffected by adjacency.
    auto anyAlive = [](int) { return true; };

    if (useLetters) {
        log.push_back(std::to_string(instances.size()) + " " + pluralMonsterName(monster.name) + " appear! " +
                       monster.description);
    } else {
        log.push_back("A " + monster.name + " appears! " + monster.description);
    }
    // Combat has its own local blow-by-blow log (above, shown on
    // drawCombatFrame's dedicated screen) -- this just leaves a short
    // continuity trail in the persistent exploration log_ so returning to
    // the overworld afterward isn't silent about what just happened.
    pushLog(useLetters ? (std::to_string(instances.size()) + " " + pluralMonsterName(monster.name) + " appear!")
                        : ("A " + monster.name + " appears!"));

    // This-fight-only spell buffs/debuffs (Bless, Prayer, Protection from
    // Evil, Strength, Slow, Bestow Curse, ... -- see
    // character/Spellcasting.h's SpellEffect) -- purely local to this one
    // runCombat call, same as instances/log, never written into the
    // character's real saved armorClass/thac0.
    int playerThac0Bonus = 0;
    int playerDamageBonus = 0;
    int playerAcBonus = 0;
    // Per-instance versions of the this-fight debuffs above (Bestow Curse's
    // THAC0/damage penalties) -- a group fight's Debuff spells hit one
    // chosen enemy, not the whole group at once (see pickTarget above and
    // docs/COMBAT_NOTES.md's "one representative target" note). Every
    // monster without a GROUP line only ever has one instance, so this
    // behaves exactly like the old single int for the common case.
    std::vector<int> monsterThac0Penalty(instances.size(), 0);
    std::vector<int> monsterDamagePenalty(instances.size(), 0);
    // Same per-instance treatment for the "blocks the monster's attack(s)"
    // family (Webnet, Sleep/Hold/Charm/Confusion/Fear) -- each targets one
    // chosen enemy rather than the whole group.
    std::vector<int> blockedAttacksRemaining(instances.size(), 0);
    std::vector<bool> incapacitatedRestOfFight(instances.size(), false);
    // Webnet (consumed, blocks one attack), Brooch of Imog (reusable
    // once/day, blocks the rest of this fight -- see
    // character::useWebnet/activateBrooch and docs/CHARACTER_NOTES.md's
    // "Magic items"), and spell-based crowd control (Sleep, Hold Person,
    // Charm, Confusion, Fear, ... -- character::SpellEffect::
    // BlockMonsterAttacks) all share one mechanism: blockedAttacksRemaining
    // (above) counts down a finite number of blocked attacks per targeted
    // instance; incapacitatedRestOfFight[i] covers the "until the fight
    // ends" spells (character::kBlockRestOfFight). The Brooch's globe,
    // unlike those two, wards the PLAYER rather than debuffing one
    // monster, so it stays a single shared bool blocking every instance's
    // attack (and, as of Milestone 114, every opportunity attack too) --
    // purely local to this one runCombat call, same as the log/instances
    // above, never written into the character's real saved fields (only
    // the Brooch's daily charge itself, Character::lastBroochUseDay,
    // persists).
    bool globeActive = false;
    // Frostreaver (see character::kFrostreaverName/kFrostreaverMagicBonus
    // and docs/CHARACTER_NOTES.md's "Magic items"): DLA p.94 says it only
    // holds its "+4" while it's glacier ice, not melted slush -- modeled
    // as a this-fight-only local bonus, same mechanism as a spell buff,
    // rather than a permanent Character stat. runCombat's only call site
    // is tryMoveOverworld, so state_.x/state_.y are always the tile this
    // fight is happening on.
    if (state_.character.weaponName == character::kFrostreaverName) {
        if (std::string(hereTerrain.name) == "glacier") {
            playerThac0Bonus += character::kFrostreaverMagicBonus;
            playerDamageBonus += character::kFrostreaverMagicBonus;
            log.push_back("Your Frostreaver's edge bites keener than steel, sharpened by the glacier's own cold.");
        }
    }
    // roundNumber feeds character::meleeAttacksThisRound's 7-12-level
    // alternating pattern (PHB Table 15) -- incremented once per
    // for(;;) iteration below, since each iteration is exactly one round.
    int roundNumber = 1;
    // Set true by handleInstanceDeath below (a death-burst finishing the
    // player off after the killing blow) or by playerMoves further down
    // (an opportunity attack doing the same while retreating) -- every
    // caller checks this and stops, and the main loop returns immediately
    // once it's set, since knockedOutBy already rendered the "struck down"
    // ending itself. Named generically as of Milestone 114 since it now
    // covers more than just Sivak's burst.
    bool fightAlreadyEnded = false;
    // Knockout ending -- shared by the ordinary "an attack drops you to 0"
    // check at the bottom of the round loop, the Sivak death-burst below,
    // and an opportunity attack (Milestone 114) -- any of which can finish
    // the player off outside the normal per-round check. See
    // docs/COMBAT_NOTES.md.
    auto knockedOutBy = [&](const std::string& cause) {
        const world::Location* refuge = nearestRefuge();
        const std::string refugeName = refuge != nullptr ? refuge->name : "town";
        state_.character.currentHp = state_.character.maxHp;
        log.push_back("You are struck down... and wake up back in " + refugeName + ", battered but alive.");
        if (refuge != nullptr) {
            state_.x = refuge->x;
            state_.y = refuge->y;
        }
        log.push_back("Press any key to continue.");
        render::MapRenderer::drawCombatFrame(state_.character, buildViews(), log, state_.hoursElapsed / 24, hereTerrain,
                                              playerPos);
        render::Console::readKey();
        pushLog("You were knocked out by the " + cause + " and woke up back in " + refugeName + ".");
    };
    // Awards steel/XP/the quest kill-tally and shows this monster's death
    // flavor (Baaz's stone, or the generic fall message) the moment one
    // instance's hp reaches 0 -- not deferred to after the whole round,
    // so a group fight's log reads in the order things actually happened.
    // Returns true if a Sivak-style death-burst just knocked the player
    // out, ending the fight immediately (see knockedOutBy above); the kill
    // itself still counts either way, since XP/steel/tally are applied
    // before the burst roll, same as before Milestone 113.
    auto handleInstanceDeath = [&](int idx) -> bool {
        state_.monsterKills[monster.id] += 1;
        checkQuestReadiness(); // a SLAY objective may have just been satisfied
        int steel =
            std::max(0, character::roll(monster.steelDiceCount, monster.steelDiceSides) + monster.steelFlatBonus);
        state_.character.steelPieces += steel;
        std::string name = monsterLabel(idx);
        // Baaz Draconians turn to stone on death -- their single most
        // iconic trait (Dragonlance Adventures, TSR 2021, p.75) -- worth a
        // special line rather than the generic victory message.
        if (monster.id == "baaz") {
            log.push_back("The " + name + " falls and its body crumbles to stone! You find " +
                           std::to_string(steel) + " steel among the rubble.");
        } else {
            log.push_back("The " + name + " falls! You find " + std::to_string(steel) + " steel.");
        }
        if (monster.xpValue > 0) {
            state_.character.experience += monster.xpValue;
            log.push_back("You gain " + std::to_string(monster.xpValue) + " experience.");
            character::applyPendingLevelUps(state_.character, log);
        }
        // Sivak Draconian: real death-burst (Dragonlance Adventures p.75)
        // -- the book's "killed by something larger than itself" condition
        // has no SIZE stat to check in this project, so it always fires.
        if (monster.burstsIntoFlameOnDeath) {
            int burstDamage = character::roll(2, 4);
            state_.character.currentHp -= burstDamage;
            log.push_back("As it falls, the " + name + " bursts into flame! You take " +
                           std::to_string(burstDamage) + " damage.");
            if (state_.character.currentHp <= 0) {
                knockedOutBy(monster.name);
                return true;
            }
        }
        return false;
    };
    // A real, sourced opportunity attack (DQoK.pdf's own manual, found
    // while planning Milestone 114: "if you move away from an adjacent
    // enemy, he gets a free attack at your back and has an improved
    // chance to hit"). Triggers per-instance only when the player is
    // truly leaving THAT instance's reach (was adjacent, won't be at
    // `destination`) -- staying adjacent while sidestepping doesn't
    // provoke one. Left as an ordinary attack roll rather than inventing a
    // specific numeric bonus the manual doesn't print a value for. Skips
    // an instance that couldn't act anyway (globe/incapacitated/blocked),
    // same as monstersAct; doesn't replicate special-ability/poison checks
    // -- a deliberate simplification, see docs/COMBAT_NOTES.md.
    auto triggerOpportunityAttacks = [&](combat::GridPos destination) {
        for (size_t i = 0; i < instances.size() && state_.character.currentHp > 0; ++i) {
            if (instances[i].hp <= 0) continue;
            bool leavingReach =
                combat::isAdjacent(playerPos, instancePositions[i]) && !combat::isAdjacent(destination, instancePositions[i]);
            if (!leavingReach) continue;
            if (globeActive || incapacitatedRestOfFight[i] || blockedAttacksRemaining[i] > 0) continue;
            std::string name = monsterLabel(static_cast<int>(i));
            combat::AttackOutcome outcome = combat::resolveMonsterAttack(monster, state_.character, playerAcBonus,
                                                                           monsterThac0Penalty[i],
                                                                           monsterDamagePenalty[i]);
            if (outcome.hit) {
                state_.character.currentHp -= outcome.damage;
                log.push_back("As you pull back, the " + name + " gets a free strike! It hits you for " +
                               std::to_string(outcome.damage) + ". " + describeToHit(outcome) + " " +
                               describeDamage(outcome));
            } else {
                log.push_back("The " + name + " lunges as you pull back, but misses.");
            }
        }
    };
    auto playerAttacks = [&]() {
        bool hasRangedWeapon = state_.character.weaponName == character::kLightCrossbowName;
        bool adjacentToAny = false;
        for (size_t i = 0; i < instances.size(); ++i) {
            if (instances[i].hp > 0 && combat::isAdjacent(playerPos, instancePositions[i])) {
                adjacentToAny = true;
                break;
            }
        }
        // Sourced: "A character with a missile weapon... may not attack
        // when adjacent to an enemy" (DQoK.pdf's own manual) -- a ranged
        // weapon can hit anyone on the grid while unengaged, but is
        // disabled outright the instant an enemy closes to melee range.
        if (hasRangedWeapon && adjacentToAny) {
            log.push_back("An enemy is too close to fire your crossbow!");
            return;
        }
        std::function<bool(int)> eligible = hasRangedWeapon ? std::function<bool(int)>(anyAlive)
                                                              : std::function<bool(int)>([&](int idx) {
                                                                    return combat::isAdjacent(
                                                                        playerPos,
                                                                        instancePositions[static_cast<size_t>(idx)]);
                                                                });
        int targetIndex = pickTarget("Attack which enemy?", false, eligible);
        if (targetIndex < 0) {
            log.push_back("You're too far away to attack.");
            return;
        }
        int attacks = character::meleeAttacksThisRound(state_.character.charClass, state_.character.level,
                                                         roundNumber);
        for (int i = 0; i < attacks && !fightAlreadyEnded; ++i) {
            if (instances[static_cast<size_t>(targetIndex)].hp <= 0) {
                // Real Gold Box rule (DQoK.pdf's own manual, re-checked
                // while planning Milestone 114): "If the first target
                // goes down with the first attack, you can aim the
                // remaining attack at another target" -- retarget via the
                // same eligibility filter instead of wasting the swing
                // (Milestone 113's original, less accurate behavior).
                targetIndex = pickTarget("Attack which enemy?", false, eligible);
                if (targetIndex < 0) {
                    log.push_back("No targets remain for your last attack.");
                    break;
                }
            }
            std::string targetName = monsterLabel(targetIndex);
            combat::AttackOutcome outcome =
                combat::resolvePlayerAttack(state_.character, monster, playerThac0Bonus, playerDamageBonus);
            if (outcome.hit) {
                instances[static_cast<size_t>(targetIndex)].hp -= outcome.damage;
                log.push_back("You hit the " + targetName + " for " + std::to_string(outcome.damage) + ". " +
                               describeToHit(outcome) + " " + describeDamage(outcome));
                if (instances[static_cast<size_t>(targetIndex)].hp <= 0) {
                    if (handleInstanceDeath(targetIndex)) fightAlreadyEnded = true;
                }
            } else {
                log.push_back("You miss the " + targetName + ". " + describeToHit(outcome));
            }
        }
    };
    // One monster instance's turn -- called for every currently-alive
    // instance in fixed A->B->C order (see docs/COMBAT_NOTES.md). Stops
    // immediately if the player is dropped to 0 HP partway through, so a
    // downed player never takes further "hits" from monsters still queued
    // that round.
    auto monstersAct = [&]() {
        for (size_t i = 0; i < instances.size() && state_.character.currentHp > 0; ++i) {
            if (instances[i].hp <= 0) continue;
            std::string name = monsterLabel(static_cast<int>(i));
            if (globeActive) {
                log.push_back("The globe of invulnerability absorbs the blow from the " + name + "!");
                continue;
            }
            if (incapacitatedRestOfFight[i]) {
                log.push_back("The " + name + " is unable to act!");
                continue;
            }
            if (blockedAttacksRemaining[i] > 0) {
                --blockedAttacksRemaining[i];
                log.push_back("The " + name + " can't bring itself to attack!");
                continue;
            }
            // Bozak Draconian: casts Magic Missile (Dragonlance Adventures
            // p.74, "as a 4th-level magic-user") instead of its weapon
            // attack some rounds -- same PHB p.176 math as the player's own
            // magic_missile spell (character::castSpell), fixed at
            // "4th-level caster" per the book's own framing. No attack
            // roll, no saving throw. Rolled independently per instance, so
            // a future multi-Bozak fight would have each one roll its own
            // chance -- see docs/COMBAT_NOTES.md for the invented per-round
            // chance (the book gives no real frequency).
            if (monster.castsMagicMissile && character::roll(1, 100) <= monster.magicMissileChancePercent) {
                int missileDamage = (character::roll(1, 4) + 1) + (character::roll(1, 4) + 1);
                state_.character.currentHp -= missileDamage;
                log.push_back("The " + name + " casts Magic Missile! It strikes you for " +
                               std::to_string(missileDamage) + " -- no saving throw.");
                continue;
            }
            // Aurak Draconian: noxious-cloud breath weapon (Dragonlance
            // Adventures p.73) instead of its weapon attack some rounds --
            // save vs. breath weapon for half of 20 damage, or full damage
            // and blinded (a -4 this-fight to-hit penalty; the book names
            // the condition but not a number, so this value is invented).
            // The book's real "three times per day" is compressed to
            // "available this whole fight" -- see docs/COMBAT_NOTES.md.
            if (monster.hasBreathWeapon && character::roll(1, 100) <= monster.breathWeaponChancePercent) {
                if (combat::rollSavingThrow(state_.character, character::SaveCategory::BreathWeapon)) {
                    state_.character.currentHp -= 10;
                    log.push_back("The " + name + " breathes a noxious cloud! You resist -- 10 damage.");
                } else {
                    state_.character.currentHp -= 20;
                    playerThac0Bonus -= 4;
                    log.push_back("The " + name +
                                   " breathes a noxious cloud! It burns you for 20 damage and blinds you.");
                }
                continue;
            }
            // Melee-locked, same as the player (Milestone 114) -- no
            // monster in this roster has a ranged attack, so every
            // instance not yet adjacent spends its turn closing the
            // distance instead of attacking. combat::stepToward avoids
            // the player's own cell and every other still-alive instance.
            if (!combat::isAdjacent(instancePositions[i], playerPos)) {
                std::vector<combat::GridPos> blocked{playerPos};
                for (size_t j = 0; j < instances.size(); ++j) {
                    if (j != i && instances[j].hp > 0) blocked.push_back(instancePositions[j]);
                }
                combat::GridPos next =
                    combat::stepToward(instancePositions[i], playerPos, render::MapRenderer::kCombatGridWidth,
                                        render::MapRenderer::kCombatGridHeight, blocked);
                if (next.x != instancePositions[i].x || next.y != instancePositions[i].y) {
                    instancePositions[i] = next;
                    log.push_back("The " + name + " closes in.");
                }
                continue;
            }
            combat::AttackOutcome outcome = combat::resolveMonsterAttack(monster, state_.character, playerAcBonus,
                                                                           monsterThac0Penalty[i],
                                                                           monsterDamagePenalty[i]);
            if (outcome.hit) {
                state_.character.currentHp -= outcome.damage;
                log.push_back("The " + name + " hits you for " + std::to_string(outcome.damage) + ". " +
                               describeToHit(outcome) + " " + describeDamage(outcome));
                // Giant Spider's real Type F poison bite (Monstrous Manual
                // p.329): a failed save is "immediate death" in the book,
                // but this project never permadeaths the player (see
                // docs/COMBAT_NOTES.md) -- setting currentHp to 0 here lets
                // the existing knockout check after monstersAct returns
                // handle it exactly like any other lethal hit.
                if (monster.poisonOnHit) {
                    if (combat::rollSavingThrow(state_.character, character::SaveCategory::ParalyzationPoisonDeath)) {
                        log.push_back("You resist the poison.");
                    } else {
                        log.push_back("The poison overwhelms you!");
                        state_.character.currentHp = 0;
                    }
                }
            } else {
                log.push_back("The " + name + " misses you. " + describeToHit(outcome));
            }
        }
    };
    // Casts `spellId` (already confirmed memorized -- see the Cast key
    // handling below) as the round's action instead of attacking, and
    // dispatches on character::SpellEffect rather than the spell's name/id
    // so a new spell that reuses an existing category needs no change here.
    // Effects that touch a monster at all (damage, block, debuff, instant
    // defeat) ask which one via pickTarget first, same "one representative
    // target" simplification playerAttacks uses -- effects that touch only
    // the caster (heal, self-buffs) never open a picker.
    auto playerCasts = [&](const std::string& spellId) {
        character::SpellCastResult result = character::castSpell(state_.character, spellId);
        if (!result.success) return; // defensive -- shouldn't happen, caller already checked
        bool needsTarget = result.effect == character::SpellEffect::DamageMonster ||
                            result.effect == character::SpellEffect::BlockMonsterAttacks ||
                            result.effect == character::SpellEffect::DebuffMonsterThac0 ||
                            result.effect == character::SpellEffect::DebuffMonsterDamage ||
                            result.effect == character::SpellEffect::BuffPlayerAndDebuffMonsterThac0 ||
                            result.effect == character::SpellEffect::InstantDefeat;
        int targetIndex = needsTarget ? pickTarget("Cast at which enemy?", false, anyAlive) : -1;
        std::string targetName = targetIndex >= 0 ? monsterLabel(targetIndex) : monster.name;
        switch (result.effect) {
            case character::SpellEffect::DamageMonster:
                instances[static_cast<size_t>(targetIndex)].hp -= result.amount;
                log.push_back("Your " + result.spellName + " strikes the " + targetName + " for " +
                               std::to_string(result.amount) + ".");
                if (instances[static_cast<size_t>(targetIndex)].hp <= 0) {
                    if (handleInstanceDeath(targetIndex)) fightAlreadyEnded = true;
                }
                break;
            case character::SpellEffect::HealCaster: {
                int healed = std::min(result.amount, state_.character.maxHp - state_.character.currentHp);
                state_.character.currentHp += healed;
                log.push_back("You cast " + result.spellName + " and heal " + std::to_string(healed) +
                               " hit points.");
                break;
            }
            case character::SpellEffect::BlockMonsterAttacks:
                if (result.amount == character::kBlockRestOfFight) {
                    incapacitatedRestOfFight[static_cast<size_t>(targetIndex)] = true;
                } else {
                    blockedAttacksRemaining[static_cast<size_t>(targetIndex)] += result.amount;
                }
                log.push_back("You cast " + result.spellName + " on the " + targetName + "!");
                break;
            case character::SpellEffect::BuffPlayerThac0:
                playerThac0Bonus += result.amount;
                log.push_back("You cast " + result.spellName + ".");
                break;
            case character::SpellEffect::BuffPlayerDamage:
                playerDamageBonus += result.amount;
                log.push_back("You cast " + result.spellName + ".");
                break;
            case character::SpellEffect::BuffPlayerAc:
                playerAcBonus += result.amount;
                log.push_back("You cast " + result.spellName + ".");
                break;
            case character::SpellEffect::DebuffMonsterThac0:
                monsterThac0Penalty[static_cast<size_t>(targetIndex)] += result.amount;
                log.push_back("You cast " + result.spellName + " on the " + targetName + "!");
                break;
            case character::SpellEffect::DebuffMonsterDamage:
                monsterDamagePenalty[static_cast<size_t>(targetIndex)] += result.amount;
                log.push_back("You cast " + result.spellName + " on the " + targetName + "!");
                break;
            case character::SpellEffect::BuffPlayerAndDebuffMonsterThac0:
                playerThac0Bonus += result.amount;
                monsterThac0Penalty[static_cast<size_t>(targetIndex)] += result.amount;
                log.push_back("You cast " + result.spellName + ".");
                break;
            case character::SpellEffect::InstantDefeat:
                log.push_back("Your " + result.spellName + " destroys the " + targetName + " outright!");
                instances[static_cast<size_t>(targetIndex)].hp = 0;
                if (handleInstanceDeath(targetIndex)) fightAlreadyEnded = true;
                break;
        }
    };
    // Drinks the first carried Potion of Healing (see
    // character::firstPotionIndex/drinkPotion) as the round's action
    // instead of attacking -- same "replaces playerAttacks() in the
    // initiative-ordered exchange" shape as playerCasts above.
    auto playerDrinksPotion = [&]() {
        int potionIndex = character::firstPotionIndex(state_.character);
        character::PurchaseResult result = character::drinkPotion(state_.character, potionIndex);
        log.push_back(result.message);
    };
    // Same "replaces playerAttacks() for the round" shape as
    // playerDrinksPotion above. On success, asks which enemy gets tangled
    // (same pickTarget mechanism as a Block spell) and increments its own
    // blockedAttacksRemaining entry -- see character::useWebnet for what
    // the function itself does and doesn't own.
    auto playerUsesWebnet = [&]() {
        int webnetIndex = character::firstWebnetIndex(state_.character);
        character::PurchaseResult result = character::useWebnet(state_.character, webnetIndex);
        log.push_back(result.message);
        if (result.success) {
            int targetIndex = pickTarget("Tangle which enemy?", false, anyAlive);
            if (targetIndex >= 0) ++blockedAttacksRemaining[static_cast<size_t>(targetIndex)];
        }
    };
    auto playerActivatesBrooch = [&]() {
        character::PurchaseResult result = character::activateBrooch(state_.character, state_.hoursElapsed / 24);
        log.push_back(result.message);
        if (result.success) globeActive = true;
    };
    // Same "replaces playerAttacks() for the round" shape as the other
    // item-use lambdas above -- character::useStaffCure owns the heal math
    // and the once-per-day gate itself, same division of labor drinkPotion
    // already has.
    auto playerUsesStaffCure = [&]() {
        character::PurchaseResult result = character::useStaffCure(state_.character, state_.hoursElapsed / 24);
        log.push_back(result.message);
    };
    // Moves to an already-validated `destination` (the main loop's key
    // handling below checks bounds/occupancy before ever setting this
    // action up, same "reject before it costs a round" pattern Cast/
    // Inventory already use) -- Milestone 114. Triggers any opportunity
    // attacks first; if one knocks the player out, the move itself never
    // completes, same "an attack can end the fight before anything else
    // resolves" precedent the death-burst check already established.
    auto playerMoves = [&](combat::GridPos destination) {
        triggerOpportunityAttacks(destination);
        if (state_.character.currentHp <= 0) {
            knockedOutBy(monster.name);
            fightAlreadyEnded = true;
            return;
        }
        std::string dirLabel = destination.y < playerPos.y   ? "north"
                                : destination.y > playerPos.y ? "south"
                                : destination.x < playerPos.x ? "west"
                                                               : "east";
        playerPos = destination;
        log.push_back("You move " + dirLabel + ".");
    };

    for (;;) {
        render::MapRenderer::drawCombatFrame(state_.character, buildViews(), log, state_.hoursElapsed / 24, hereTerrain,
                                              playerPos);
        render::Key key = render::Console::readKey();

        if (key == render::Key::Flee) {
            log.push_back("You break off and retreat.");
            log.push_back("Press any key to continue.");
            render::MapRenderer::drawCombatFrame(state_.character, buildViews(), log, state_.hoursElapsed / 24, hereTerrain,
                                                  playerPos);
            render::Console::readKey();
            pushLog(useLetters ? ("You fled from the " + pluralMonsterName(monster.name) + ".")
                                : ("You fled from the " + monster.name + "."));
            return;
        }

        bool casting = false;
        std::string chosenSpellId;
        bool drinking = false;
        bool usingWebnet = false;
        bool activatingBrooch = false;
        bool usingStaffCure = false;
        bool moving = false;
        combat::GridPos moveTarget;
        if (key == render::Key::North || key == render::Key::South || key == render::Key::East ||
            key == render::Key::West) {
            // Milestone 114: validated up front, same "reject before it
            // costs a round" pattern Cast/Inventory already use below --
            // an invalid move never enters the initiative-ordered
            // playerActs dispatch at all.
            int dx = key == render::Key::East ? 1 : key == render::Key::West ? -1 : 0;
            int dy = key == render::Key::South ? 1 : key == render::Key::North ? -1 : 0;
            combat::GridPos destination{playerPos.x + dx, playerPos.y + dy};
            if (destination.x < 0 || destination.x >= render::MapRenderer::kCombatGridWidth || destination.y < 0 ||
                destination.y >= render::MapRenderer::kCombatGridHeight) {
                log.push_back("You can't move that way.");
                continue;
            }
            bool occupied = false;
            for (size_t i = 0; i < instances.size(); ++i) {
                if (instances[i].hp > 0 && instancePositions[i].x == destination.x &&
                    instancePositions[i].y == destination.y) {
                    occupied = true;
                    break;
                }
            }
            if (occupied) {
                log.push_back("Something's in the way.");
                continue;
            }
            moveTarget = destination;
            moving = true;
        } else if (key == render::Key::Cast) {
            if (!character::canCastSpells(state_.character.charClass)) {
                log.push_back("You have no spell to cast.");
                continue;
            }
            if (!character::hasMemorizedSpellsAvailable(state_.character, state_.hoursElapsed / 24)) {
                log.push_back("You have no spells remaining today.");
                continue;
            }
            // Distinct remaining spell ids, in memorized order -- if
            // exactly one, cast it directly (preserves the original
            // one-spell UX exactly); otherwise open a picker.
            std::vector<std::string> distinctIds;
            for (const auto& id : state_.character.memorizedSpellIds) {
                if (std::find(distinctIds.begin(), distinctIds.end(), id) == distinctIds.end()) {
                    distinctIds.push_back(id);
                }
            }
            if (distinctIds.size() == 1) {
                chosenSpellId = distinctIds.front();
            } else {
                std::vector<std::string> labels;
                for (const auto& id : distinctIds) {
                    const character::SpellInfo* spell =
                        character::findSpell(state_.character.charClass, id);
                    int count = static_cast<int>(std::count(state_.character.memorizedSpellIds.begin(),
                                                              state_.character.memorizedSpellIds.end(), id));
                    labels.push_back((spell != nullptr ? spell->name : id) +
                                      (count > 1 ? " (x" + std::to_string(count) + ")" : ""));
                }
                int selected = 0;
                bool cancelled = false;
                // Milestone 115: in-frame chooser (see docs/COMBAT_NOTES.md's
                // "In-frame combat actions" section) -- same shape as
                // pickTarget's own drawCombatFrame loop above, just with
                // `options` set (spell selection has no grid representation
                // of its own) instead of a gridCursorIndex.
                for (;;) {
                    render::MapRenderer::CombatPrompt prompt;
                    prompt.title = "Cast which spell?";
                    prompt.options = labels;
                    prompt.selected = selected;
                    prompt.footer = "up/down=select   Enter=cast   q=cancel";
                    render::MapRenderer::drawCombatFrame(state_.character, buildViews(), log,
                                                          state_.hoursElapsed / 24, hereTerrain, playerPos, prompt);
                    render::Key pickKey = render::Console::readKey();
                    if (pickKey == render::Key::North) {
                        selected = (selected - 1 + static_cast<int>(labels.size())) % static_cast<int>(labels.size());
                    } else if (pickKey == render::Key::South) {
                        selected = (selected + 1) % static_cast<int>(labels.size());
                    } else if (pickKey == render::Key::Enter) {
                        chosenSpellId = distinctIds[static_cast<size_t>(selected)];
                        break;
                    } else if (pickKey == render::Key::Quit) {
                        cancelled = true;
                        break;
                    }
                }
                if (cancelled) continue;
            }
            casting = true;
        } else if (key == render::Key::Inventory) {
            // Reinterpreted locally as "use a consumable" -- same "local
            // key reinterpretation instead of a new Key value" trick
            // handleShop already uses for this exact key (buy/sell toggle
            // there). Milestone 115: a real in-frame USE menu over every
            // usable item (character::availableCombatItems) instead of the
            // old fixed-priority pick-the-first-one behavior, which meant a
            // character carrying both a Potion and a Webnet could never
            // reach the Webnet at all. Auto-selects when exactly one item
            // is usable, same "no picker needed for one candidate" rule
            // pickTarget already follows.
            std::vector<character::CombatItem> usableItems =
                character::availableCombatItems(state_.character, state_.hoursElapsed / 24);
            character::CombatItemKind chosenItemKind = character::CombatItemKind::Potion;
            if (usableItems.empty()) {
                log.push_back("You have nothing to use.");
                continue;
            } else if (usableItems.size() == 1) {
                chosenItemKind = usableItems.front().kind;
            } else {
                std::vector<std::string> itemLabels;
                for (const character::CombatItem& item : usableItems) itemLabels.push_back(item.label);
                int selected = 0;
                bool cancelled = false;
                for (;;) {
                    render::MapRenderer::CombatPrompt prompt;
                    prompt.title = "Use which item?";
                    prompt.options = itemLabels;
                    prompt.selected = selected;
                    prompt.footer = "up/down=select   Enter=use   q=cancel";
                    render::MapRenderer::drawCombatFrame(state_.character, buildViews(), log,
                                                          state_.hoursElapsed / 24, hereTerrain, playerPos, prompt);
                    render::Key pickKey = render::Console::readKey();
                    if (pickKey == render::Key::North) {
                        selected = (selected - 1 + static_cast<int>(itemLabels.size())) %
                                   static_cast<int>(itemLabels.size());
                    } else if (pickKey == render::Key::South) {
                        selected = (selected + 1) % static_cast<int>(itemLabels.size());
                    } else if (pickKey == render::Key::Enter) {
                        chosenItemKind = usableItems[static_cast<size_t>(selected)].kind;
                        break;
                    } else if (pickKey == render::Key::Quit) {
                        cancelled = true;
                        break;
                    }
                }
                if (cancelled) continue;
            }
            switch (chosenItemKind) {
                case character::CombatItemKind::Potion: drinking = true; break;
                case character::CombatItemKind::Webnet: usingWebnet = true; break;
                case character::CombatItemKind::Brooch: activatingBrooch = true; break;
                case character::CombatItemKind::StaffCure: usingStaffCure = true; break;
            }
        } else if (key != render::Key::Enter) {
            continue;
        }

        // PHB p.124: one d10 per side, lower goes first. Whoever acts
        // second is skipped if the first attacker already ended the fight.
        std::function<void()> playerActs =
            casting ? std::function<void()>([&]() { playerCasts(chosenSpellId); })
            : drinking ? std::function<void()>(playerDrinksPotion)
            : usingWebnet ? std::function<void()>(playerUsesWebnet)
            : activatingBrooch ? std::function<void()>(playerActivatesBrooch)
            : usingStaffCure ? std::function<void()>(playerUsesStaffCure)
            : moving ? std::function<void()>([&]() { playerMoves(moveTarget); })
                       : std::function<void()>(playerAttacks);
        if (combat::playerActsFirst()) {
            playerActs();
            if (fightAlreadyEnded) return;
            if (aliveCount() > 0) monstersAct();
        } else {
            monstersAct();
            if (state_.character.currentHp > 0) playerActs();
        }
        if (fightAlreadyEnded) return; // an opportunity attack or death-burst already rendered its own ending
        ++roundNumber;

        if (aliveCount() == 0) {
            // Every reward (steel/XP/the quest kill-tally) was already
            // applied per-instance by handleInstanceDeath as each one fell
            // -- this is just the fight's closing line and pause, same
            // "Press any key to continue" beat every earlier milestone's
            // single-monster victory already had.
            log.push_back(useLetters ? ("The " + pluralMonsterName(monster.name) + " are defeated!")
                                      : ("You defeated the " + monster.name + "."));
            log.push_back("Press any key to continue.");
            render::MapRenderer::drawCombatFrame(state_.character, buildViews(), log, state_.hoursElapsed / 24, hereTerrain,
                                                  playerPos);
            render::Console::readKey();
            pushLog(useLetters ? ("You defeated the " + pluralMonsterName(monster.name) + ".")
                                : ("You defeated the " + monster.name + "."));
            return;
        }
        if (state_.character.currentHp <= 0) {
            // Knocked out, not killed -- see docs/COMBAT_NOTES.md. Full-healed
            // and carried to the nearest refuge rather than a real death.
            knockedOutBy(monster.name);
            return;
        }
    }
}

} // namespace game
