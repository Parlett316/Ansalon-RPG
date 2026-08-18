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
    // roster. See docs/COMBAT_NOTES.md for the sourcing and how a failed
    // save is handled (knocked out, not the book's literal "die").
    bool poisonOnHit = false;

    int steelDiceCount = 0;
    int steelDiceSides = 0;
    int steelFlatBonus = 0;

    // Sourced from the Monster Manual's own "XP Value" field where
    // available; Baaz Draconian's real formula ("81 + 1/hp") is
    // simplified to a flat value near its average roll -- see
    // docs/COMBAT_NOTES.md.
    int xpValue = 0;
};

// A loaded roster of monsters, static content like timeline::Timeline --
// loaded fresh every run by MonsterLoader, never mutated during play.
class MonsterCatalog {
public:
    void addMonster(Monster monster);

    // Picks uniformly at random. Only call when size() > 0.
    const Monster& randomMonster() const;

    size_t size() const { return monsters_.size(); }

private:
    std::vector<Monster> monsters_;
};

} // namespace combat
