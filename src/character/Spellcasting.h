#pragma once

#include "character/Character.h"

namespace character {

// One real, PHB-sourced spell per casting class -- not a spellbook/spell-
// selection system, same "one sourced thing, not a whole subsystem faked"
// spirit as ClassInfo's single placeholder weapon (CharClass.h). Mage
// knows Magic Missile (PHB p.176); Cleric knows Cure Light Wounds (PHB
// p.253). See docs/CHARACTER_NOTES.md's "Spellcasting" section.
bool canCastSpells(ClassId id);
const char* knownSpellName(ClassId id); // "" if canCastSpells is false

// Real per-day 1st-level-spell-slot count, sourced from the PHB (Table 21,
// Wizard Spell Progression, p.43; Table 24, Priest Spell Progression,
// p.47), plus (Cleric only) Wisdom bonus spells (Table 5, p.23). A Mage
// who can't actually cast arcane magic -- Kender, or a subrace with
// effectiveCanBeMage false (Kagonesti Elf, Hill/Mountain Dwarf) -- always
// gets 0, regardless of level: this is the real enforcement of a
// restriction that was flavor-only before spellcasting existed. Returns 0
// for non-casting classes.
int maxSpellSlotsPerDay(const Character& character);

// A pure query, no side effects: false unless character.spellsCastDay ==
// currentDay (the same hoursElapsed/24 "day" convention Timeline uses),
// i.e. the character has memorized spells today via memorizeSpells below
// -- see docs/CHARACTER_NOTES.md's "Rest and spell memorization".
bool hasSpellSlotAvailable(const Character& character, long long currentDay);

// PHB p.107 (Wizard)/p.111 (Priest): a caster needs a restful night's
// sleep before they can (re-)memorize their spells for the day. Sets
// spellsCastDay = currentDay and spellsCastToday = 0. Called only from
// game::GameLoop::handleRest ('r') -- rest is the sole precondition the
// book requires; with one known spell per caster there's no selection
// step to expose separately.
void memorizeSpells(Character& character, long long currentDay);

struct SpellCastResult {
    bool targetsMonster = false; // true: Magic Missile damages a monster; false: Cure Light Wounds heals the caster
    int amount = 0;              // damage or healing, already rolled
};

// Consumes one spell slot (increments spellsCastToday) and resolves the
// class's one known spell. Caller must have already confirmed
// hasSpellSlotAvailable.
SpellCastResult castSpell(Character& character);

} // namespace character
