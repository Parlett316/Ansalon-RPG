#pragma once

#include <string>
#include <vector>

namespace combat {

// A monster's stats. Sourced from real 2e rulebooks (the Monstrous Manual
// and, for Krynn-specific Draconians, Dragonlance Adventures) -- visually
// confirmed against rendered page images, not invented. See
// docs/COMBAT_NOTES.md for page citations and what's simplified.
struct Monster {
    std::string id;
    std::string name;
    std::string description; // shown when the encounter starts

    int hpDiceCount = 1;
    int hpDiceSides = 8;
    int hpFlatBonus = 0;

    int armorClass = 10;
    int thac0 = 20;

    int damageDiceCount = 1;
    int damageDiceSides = 6;
    int damageFlatBonus = 0;

    // True only for the Giant Spider (Type F poison, Monstrous Manual
    // p.329) -- a single flag rather than a general poison-type system,
    // since this is currently the only poison-bearing creature in the
    // roster whose real effect is death (reconciled here as a knockout,
    // not the book's literal "die"). The Kapak's and Giant Centipede's real
    // poisons both cause paralysis instead, a mechanically different
    // effect this flag would misrepresent -- see docs/COMBAT_NOTES.md.
    bool poisonOnHit = false;

    // True only for the Bozak Draconian -- casts Magic Missile (Dragonlance
    // Adventures p.74, "as a 4th-level magic-user") instead of its normal
    // weapon attack some rounds. See docs/COMBAT_NOTES.md.
    bool castsMagicMissile = false;
    int magicMissileChancePercent = 0; // invented pacing, book gives no frequency

    // A breath weapon instead of its normal weapon attack some rounds --
    // originally Aurak-only (a noxious cloud, Dragonlance Adventures p.73),
    // generalized at Milestone 190 so the Blue Dragon's differently-flavored
    // lightning bolt (Monster Manual p.66) can share the same chance-per-
    // round skeleton with its own real damage/name/effect. See
    // docs/COMBAT_NOTES.md.
    bool hasBreathWeapon = false;
    int breathWeaponChancePercent = 0; // invented pacing -- see docs/COMBAT_NOTES.md
    int breathDamageDiceCount = 0;
    int breathDamageDiceSides = 0;
    int breathDamageFlatBonus = 0;
    // True only for the Aurak -- the book names the condition but not a
    // number, so the THAC0 penalty applied alongside this is still invented
    // (see docs/COMBAT_NOTES.md). The Dragon's lightning breath has no such
    // secondary effect.
    bool breathWeaponBlindsOnFail = false;
    std::string breathWeaponName; // e.g. "noxious cloud", "bolt of lightning"

    // True only for the Sivak Draconian -- bursts into flame on death,
    // dealing real damage (Dragonlance Adventures p.75) instead of a
    // flavor-only victory message. See docs/COMBAT_NOTES.md.
    bool burstsIntoFlameOnDeath = false;

    // Terrain-specific encounter pools (see docs/COMBAT_NOTES.md). Both are
    // world::TerrainInfo::code characters. excludedTerrain is a real, sourced
    // Climate/Terrain hard restriction (only Gnoll has one); terrainBias is
    // invented flavor weighting, not sourced -- same honesty as
    // TerrainInfo::encounterChancePercent.
    std::vector<char> excludedTerrain;
    std::vector<char> terrainBias;

    // onlyTerrain is the inverse of excludedTerrain: a hard restriction to
    // ONLY the listed terrain codes, empty = no restriction. Currently only
    // Thanoi has one (glacier), a deliberate invented gameplay restriction,
    // not a sourced Climate/Terrain field -- see docs/COMBAT_NOTES.md.
    std::vector<char> onlyTerrain;

    // 0 = no restriction. Otherwise the monster is never eligible unless the
    // encounter tile is at least this many tiles (straight-line) from the
    // nearest civilian town (world::Location::isTown) -- invented gameplay
    // tuning, not sourced, same honesty as encounterChancePercent/
    // terrainBias. See docs/COMBAT_NOTES.md.
    int minTownDistance = 0;

    // How many of this monster appear in one encounter -- default 1/1 means
    // always solo (every monster's behavior before Milestone 113). When
    // greater than 1, sourced from the monster's real Monstrous Manual/
    // Dragonlance Adventures "No. Appearing" field, then clamped to this
    // project's own invented playability cap (see docs/COMBAT_NOTES.md's
    // "Monster encounter groups" section) -- real range, invented ceiling,
    // same "sourced number, tuned cap" honesty as MIN_TOWN_DISTANCE.
    int groupMin = 1;
    int groupMax = 1;

    int steelDiceCount = 0;
    int steelDiceSides = 0;
    int steelFlatBonus = 0;

    // Sourced from the Monster Manual's own "XP Value" field where
    // available; Baaz Draconian's real formula ("81 + 1/hp") is
    // simplified to a flat value near its average roll -- see
    // docs/COMBAT_NOTES.md.
    int xpValue = 0;

    // Grid footprint in cells, Milestone 190 -- default 1x1 (every monster
    // before this milestone). Not a 2e stat: the real Monstrous Manual's own
    // SIZE field (S/M/L/H/G + a printed dimension) never ties to a grid
    // footprint, that's a Gold Box rendering choice, not a book rule -- see
    // docs/COMBAT_NOTES.md. Consumed only by ansalon_sfml_phase1's combat
    // grid (occupancy/adjacency/pathing/rendering), same "shared field,
    // SFML-only consumer" precedent moveSquares below already set;
    // ansalon_rpg's console combat treats every monster as a single cell,
    // Dragon included, unaffected. A monster with a footprint area > 1 must
    // stay solo (groupMax == 1, MonsterLoader enforces this) -- this
    // project has no multi-cell BFS pathing or overlap resolution for two
    // big creatures at once.
    int footprintWidth = 1;
    int footprintHeight = 1;

    // How many grid squares this monster can move in one combat round
    // (Milestone 185's bigger battlefield + real per-round movement).
    // Sourced from the "Base Movement" stat in the three SSI Gold Box
    // Dragonlance games' own bestiaries where a matching entry exists
    // (References/*.html -- Champions/Death Knights/The Dark Queen of
    // Krynn), same "squares" unit already established for the combat grid
    // (DQoK.pdf's own Armor Table, p.51); 12 (this project's own invented
    // baseline, same as an unarmored human) for every monster none of the
    // three games list -- see docs/COMBAT_NOTES.md and data/monsters.txt's
    // own MOVE grammar comment for the full breakdown.
    int moveSquares = 12;
};

// True if `monster` counts as "weak" for a Fighter-type's sweep attack
// (DQoK.pdf's own manual: "Fighter-types may also 'sweep' through several
// weak opponents in one combat round... he automatically attacks all of the
// weak opponents.") -- the manual gives no numeric threshold for "weak" in
// the surviving text, so this project defines it as hpDiceCount <= 1, which
// is already how this project encodes a monster's real 2e Hit Dice (N HD
// rolls N HP dice). Confirmed against data/monsters.txt: Goblin/Kobold/
// Hobgoblin/Skeleton (real HD 1-1/~1-2/1+1/1) are exactly the "line troop"
// tier Milestone 113's GROUP feature already singled out. Since an
// encounter is always N copies of one Monster (never mixed types), this is
// a single check for the whole fight, not per-instance -- see
// docs/COMBAT_NOTES.md.
bool isSweepEligible(const Monster& monster);

// Rolls how many of `monster` show up this encounter -- a uniform pick in
// [groupMin, groupMax] via the same character::roll primitive every other
// dice roll in this project uses (roll(1, N) shifted into range), not a
// new dice mechanic. Always returns groupMin when groupMin == groupMax
// (every monster without a GROUP line in data/monsters.txt), so this is a
// safe no-op call for the vast majority of the roster. Extracted as its
// own function (rather than left inline in GameLoop::runCombat) so it's
// directly unit-testable -- see docs/COMBAT_NOTES.md's "Monster encounter
// groups" section.
int rollGroupSize(const Monster& monster);

// A loaded roster of monsters, static content like timeline::Timeline --
// loaded fresh every run by MonsterLoader, never mutated during play.
class MonsterCatalog {
public:
    void addMonster(Monster monster);

    // Picks a monster weighted for the given terrain and gated by distance
    // from the nearest town (see Monster::excludedTerrain/terrainBias/
    // onlyTerrain/minTownDistance and docs/COMBAT_NOTES.md). Only call when
    // size() > 0.
    const Monster& randomMonster(char terrainCode, int distanceToNearestTown) const;

    size_t size() const { return monsters_.size(); }

private:
    std::vector<Monster> monsters_;
};

} // namespace combat
