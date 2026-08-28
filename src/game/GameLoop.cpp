#include "game/GameLoop.h"
#include "game/SaveGame.h"
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
        render::MapRenderer::drawCharacterSheet(state_.character, state_.hoursElapsed / 24);
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
    int monsterMaxHp = character::roll(monster.hpDiceCount, monster.hpDiceSides) + monster.hpFlatBonus;
    int monsterHp = monsterMaxHp;
    std::vector<std::string> log;
    log.push_back("A " + monster.name + " appears! " + monster.description);
    // Combat has its own local blow-by-blow log (above, shown on
    // drawCombatFrame's dedicated screen) -- this just leaves a short
    // continuity trail in the persistent exploration log_ so returning to
    // the overworld afterward isn't silent about what just happened.
    pushLog("A " + monster.name + " appears!");

    // This-fight-only spell buffs/debuffs (Bless, Prayer, Protection from
    // Evil, Strength, Slow, Bestow Curse, ... -- see
    // character/Spellcasting.h's SpellEffect) -- purely local to this one
    // runCombat call, same as monsterHp/log, never written into the
    // character's real saved armorClass/thac0.
    int playerThac0Bonus = 0;
    int playerDamageBonus = 0;
    int playerAcBonus = 0;
    int monsterThac0Penalty = 0;
    int monsterDamagePenalty = 0;
    // Frostreaver (see character::kFrostreaverName/kFrostreaverMagicBonus
    // and docs/CHARACTER_NOTES.md's "Magic items"): DLA p.94 says it only
    // holds its "+4" while it's glacier ice, not melted slush -- modeled
    // as a this-fight-only local bonus, same mechanism as a spell buff,
    // rather than a permanent Character stat. runCombat's only call site
    // is tryMoveOverworld, so state_.x/state_.y are always the tile this
    // fight is happening on.
    if (state_.character.weaponName == character::kFrostreaverName) {
        const world::TerrainInfo& hereTerrain = world::terrainFor(grid_.terrainCodeAt(state_.x, state_.y));
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
    auto playerAttacks = [&]() {
        int attacks = character::meleeAttacksThisRound(state_.character.charClass, state_.character.level,
                                                         roundNumber);
        for (int i = 0; i < attacks && monsterHp > 0; ++i) {
            combat::AttackOutcome outcome =
                combat::resolvePlayerAttack(state_.character, monster, playerThac0Bonus, playerDamageBonus);
            if (outcome.hit) {
                monsterHp -= outcome.damage;
                log.push_back("You hit the " + monster.name + " for " + std::to_string(outcome.damage) + ". " +
                               describeToHit(outcome) + " " + describeDamage(outcome));
            } else {
                log.push_back("You miss the " + monster.name + ". " + describeToHit(outcome));
            }
        }
    };
    // Webnet (consumed, blocks one attack), Brooch of Imog (reusable
    // once/day, blocks the rest of this fight -- see
    // character::useWebnet/activateBrooch and docs/CHARACTER_NOTES.md's
    // "Magic items"), and now spell-based crowd control (Sleep, Hold
    // Person, Charm, Confusion, Fear, ... -- character::SpellEffect::
    // BlockMonsterAttacks) all share one mechanism: blockedMonsterAttacks
    // counts down a finite number of blocked attacks,
    // monsterIncapacitatedRestOfFight covers the "until the fight ends"
    // spells (character::kBlockRestOfFight). Purely local to this one
    // runCombat call -- "does an attack land this fight" never needs to
    // survive to the save file, only the Brooch's daily charge does
    // (Character::lastBroochUseDay).
    int blockedMonsterAttacks = 0;
    bool monsterIncapacitatedRestOfFight = false;
    bool globeActive = false;
    auto monsterAttacks = [&]() {
        if (globeActive) {
            log.push_back("The globe of invulnerability absorbs the blow!");
            return;
        }
        if (monsterIncapacitatedRestOfFight) {
            log.push_back("The " + monster.name + " is unable to act!");
            return;
        }
        if (blockedMonsterAttacks > 0) {
            --blockedMonsterAttacks;
            log.push_back("The " + monster.name + " can't bring itself to attack!");
            return;
        }
        // Bozak Draconian: casts Magic Missile (Dragonlance Adventures
        // p.74, "as a 4th-level magic-user") instead of its weapon attack
        // some rounds -- same PHB p.176 math as the player's own
        // magic_missile spell (character::castSpell), fixed at "4th-level
        // caster" per the book's own framing. No attack roll, no saving
        // throw. See docs/COMBAT_NOTES.md for the invented per-round
        // chance (the book gives no real frequency).
        if (monster.castsMagicMissile && character::roll(1, 100) <= monster.magicMissileChancePercent) {
            int missileDamage = (character::roll(1, 4) + 1) + (character::roll(1, 4) + 1);
            state_.character.currentHp -= missileDamage;
            log.push_back("The " + monster.name + " casts Magic Missile! It strikes you for " +
                           std::to_string(missileDamage) + " -- no saving throw.");
            return;
        }
        // Aurak Draconian: noxious-cloud breath weapon (Dragonlance
        // Adventures p.73) instead of its weapon attack some rounds --
        // save vs. breath weapon for half of 20 damage, or full damage and
        // blinded (a -4 this-fight to-hit penalty; the book names the
        // condition but not a number, so this value is invented). The
        // book's real "three times per day" is compressed to "available
        // this whole fight" -- see docs/COMBAT_NOTES.md.
        if (monster.hasBreathWeapon && character::roll(1, 100) <= monster.breathWeaponChancePercent) {
            if (combat::rollSavingThrow(state_.character, character::SaveCategory::BreathWeapon)) {
                state_.character.currentHp -= 10;
                log.push_back("The " + monster.name + " breathes a noxious cloud! You resist -- 10 damage.");
            } else {
                state_.character.currentHp -= 20;
                playerThac0Bonus -= 4;
                log.push_back("The " + monster.name +
                               " breathes a noxious cloud! It burns you for 20 damage and blinds you.");
            }
            return;
        }
        combat::AttackOutcome outcome = combat::resolveMonsterAttack(
            monster, state_.character, playerAcBonus, monsterThac0Penalty, monsterDamagePenalty);
        if (outcome.hit) {
            state_.character.currentHp -= outcome.damage;
            log.push_back("The " + monster.name + " hits you for " + std::to_string(outcome.damage) + ". " +
                           describeToHit(outcome) + " " + describeDamage(outcome));
            // Giant Spider's real Type F poison bite (Monstrous Manual
            // p.329): a failed save is "immediate death" in the book, but
            // this project never permadeaths the player (see docs/
            // COMBAT_NOTES.md) -- setting currentHp to 0 here lets the
            // existing knockout check right after this lambda runs handle
            // it exactly like any other lethal hit, no separate logic.
            if (monster.poisonOnHit) {
                if (combat::rollSavingThrow(state_.character, character::SaveCategory::ParalyzationPoisonDeath)) {
                    log.push_back("You resist the poison.");
                } else {
                    log.push_back("The poison overwhelms you!");
                    state_.character.currentHp = 0;
                }
            }
        } else {
            log.push_back("The " + monster.name + " misses you. " + describeToHit(outcome));
        }
    };
    // Casts `spellId` (already confirmed memorized -- see the Cast key
    // handling below) as the round's action instead of attacking, and
    // dispatches on character::SpellEffect rather than the spell's name/id
    // so a new spell that reuses an existing category needs no change here.
    auto playerCasts = [&](const std::string& spellId) {
        character::SpellCastResult result = character::castSpell(state_.character, spellId);
        if (!result.success) return; // defensive -- shouldn't happen, caller already checked
        switch (result.effect) {
            case character::SpellEffect::DamageMonster:
                monsterHp -= result.amount;
                log.push_back("Your " + result.spellName + " strikes the " + monster.name + " for " +
                               std::to_string(result.amount) + ".");
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
                    monsterIncapacitatedRestOfFight = true;
                } else {
                    blockedMonsterAttacks += result.amount;
                }
                log.push_back("You cast " + result.spellName + " on the " + monster.name + "!");
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
                monsterThac0Penalty += result.amount;
                log.push_back("You cast " + result.spellName + " on the " + monster.name + "!");
                break;
            case character::SpellEffect::DebuffMonsterDamage:
                monsterDamagePenalty += result.amount;
                log.push_back("You cast " + result.spellName + " on the " + monster.name + "!");
                break;
            case character::SpellEffect::BuffPlayerAndDebuffMonsterThac0:
                playerThac0Bonus += result.amount;
                monsterThac0Penalty += result.amount;
                log.push_back("You cast " + result.spellName + ".");
                break;
            case character::SpellEffect::InstantDefeat:
                log.push_back("Your " + result.spellName + " destroys the " + monster.name + " outright!");
                monsterHp = 0;
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
    // playerDrinksPotion above. On success, sets the local flag
    // monsterAttacks() checks -- see character::useWebnet/activateBrooch
    // for what each function itself does and doesn't own.
    auto playerUsesWebnet = [&]() {
        int webnetIndex = character::firstWebnetIndex(state_.character);
        character::PurchaseResult result = character::useWebnet(state_.character, webnetIndex);
        log.push_back(result.message);
        if (result.success) ++blockedMonsterAttacks;
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
    // Knockout ending -- shared by the ordinary "an attack drops you to 0"
    // check at the bottom of the round loop and the Sivak death-burst
    // below, which can finish the player off even after they already
    // landed the killing blow. See docs/COMBAT_NOTES.md.
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
        render::MapRenderer::drawCombatFrame(state_.character, monster, std::max(0, monsterHp), monsterMaxHp, log,
                                              state_.hoursElapsed / 24);
        render::Console::readKey();
        pushLog("You were knocked out by the " + cause + " and woke up back in " + refugeName + ".");
    };

    for (;;) {
        render::MapRenderer::drawCombatFrame(state_.character, monster, monsterHp, monsterMaxHp, log, state_.hoursElapsed / 24);
        render::Key key = render::Console::readKey();

        if (key == render::Key::Flee) {
            log.push_back("You break off and retreat.");
            log.push_back("Press any key to continue.");
            render::MapRenderer::drawCombatFrame(state_.character, monster, monsterHp, monsterMaxHp, log, state_.hoursElapsed / 24);
            render::Console::readKey();
            pushLog("You fled from the " + monster.name + ".");
            return;
        }

        bool casting = false;
        std::string chosenSpellId;
        bool drinking = false;
        bool usingWebnet = false;
        bool activatingBrooch = false;
        bool usingStaffCure = false;
        if (key == render::Key::Cast) {
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
                for (;;) {
                    render::MapRenderer::drawPickerFrame("Cast which spell?", labels, selected,
                                                          "up/down=select   Enter=cast   q=cancel");
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
            // there). Potion takes priority (unchanged behavior), then
            // Webnet, then Brooch of Imog, then the Staff of Striking/
            // Curing's cure function -- same "first one found, nothing to
            // actually pick between" simplification already established
            // for potions themselves.
            if (character::firstPotionIndex(state_.character) >= 0) {
                drinking = true;
            } else if (character::firstWebnetIndex(state_.character) >= 0) {
                usingWebnet = true;
            } else if (character::broochAvailableToday(state_.character, state_.hoursElapsed / 24)) {
                activatingBrooch = true;
            } else if (character::staffCureAvailableToday(state_.character, state_.hoursElapsed / 24)) {
                usingStaffCure = true;
            } else {
                log.push_back("You have nothing to use.");
                continue;
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
                       : std::function<void()>(playerAttacks);
        if (combat::playerActsFirst()) {
            playerActs();
            if (monsterHp > 0) monsterAttacks();
        } else {
            monsterAttacks();
            if (state_.character.currentHp > 0) playerActs();
        }
        ++roundNumber;

        if (monsterHp <= 0) {
            // Lifetime kill tally, incremented regardless of any quest -- a
            // SLAY objective is a query over this, the same way VISIT/TALK
            // query visitedLocations/metCharacters. See docs/QUEST_NOTES.md.
            state_.monsterKills[monster.id] += 1;
            checkQuestReadiness(); // a SLAY objective may have just been satisfied
            int steel = std::max(0, character::roll(monster.steelDiceCount, monster.steelDiceSides) +
                                         monster.steelFlatBonus);
            state_.character.steelPieces += steel;
            // Baaz Draconians turn to stone on death -- their single most
            // iconic trait (Dragonlance Adventures, TSR 2021, p.75) -- worth
            // a special line rather than the generic victory message.
            if (monster.id == "baaz") {
                log.push_back("The Baaz Draconian falls and its body crumbles to stone! You find " +
                               std::to_string(steel) + " steel among the rubble.");
            } else {
                log.push_back("The " + monster.name + " falls! You find " + std::to_string(steel) + " steel.");
            }
            if (monster.xpValue > 0) {
                state_.character.experience += monster.xpValue;
                log.push_back("You gain " + std::to_string(monster.xpValue) + " experience.");
                character::applyPendingLevelUps(state_.character, log);
            }
            // Sivak Draconian: real death-burst (Dragonlance Adventures
            // p.75) -- the book's "killed by something larger than itself"
            // condition has no SIZE stat to check in this project, so it
            // always fires, dealing real retaliatory damage rather than a
            // flavor-only victory message like the other draconians' death
            // traits. The kill still counts (XP/steel/tally already
            // applied above) even if the burst then knocks you out.
            if (monster.burstsIntoFlameOnDeath) {
                int burstDamage = character::roll(2, 4);
                state_.character.currentHp -= burstDamage;
                log.push_back("As it falls, the Sivak Draconian bursts into flame! You take " +
                               std::to_string(burstDamage) + " damage.");
                if (state_.character.currentHp <= 0) {
                    knockedOutBy(monster.name);
                    return;
                }
            }
            log.push_back("Press any key to continue.");
            render::MapRenderer::drawCombatFrame(state_.character, monster, 0, monsterMaxHp, log, state_.hoursElapsed / 24);
            render::Console::readKey();
            pushLog("You defeated the " + monster.name + ".");
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
