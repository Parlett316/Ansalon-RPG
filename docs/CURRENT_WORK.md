# Current work

Nothing in flight.

The Dragonlance magical items milestone just shipped: real magic items,
sourced from *Dragonlance Adventures* (TSR 2021)'s own "Magical Items of
Krynn" chapter (pp.91-99, visually confirmed via rendered page images)
and the 2nd ed. DMG's "Magical Item Tables" (also page-image confirmed).
Asked the user how items should be obtained before building anything --
the answer was both a shop item and a quest reward, so this ships exactly
one of each, no generic item subsystem underneath.

A "+1" enchanted weapon per class (`character::magicWeaponFor`, new
`Character::weaponMagicBonus` field wired into `combat::
resolvePlayerAttack`'s to-hit and damage, priced from DMG Table 109:
400stl sword-family, 500stl other), sold at every shop alongside the
existing mundane upgrade -- Mage and Tinker, who had no mundane upgrade
at all, get their first-ever weapon upgrade this way. Solamnic Armor
(`character::ArmorId::SolamnicArmor`, AC 0, sourced directly from DLA
p.93-94) is a new quest reward, `data/quests.txt`'s `solamnic_armor` --
this project's first item-granting quest reward, via a new
`REWARD_SOLAMNIC_ARMOR` bare flag mirroring `REWARD_KNIGHT_SWORD`'s exact
shape (not a generic string-driven item mapping, which `docs/
QUEST_NOTES.md` had already deliberately rejected). Gated by a new
`sword_knight` condition (currently *is* a Sword Knight) since the book
ties this to the title "Lord," which this project doesn't model. Offered
by a new POI, `data/zones/high_clerist_tower.txt`'s `L` ("A Knight of the
Circle") -- `K` and `S` there already carry a quest each. The chapter's
unique, canon-owned artifacts (Wyrmslayer, Staff of Magius, the Hammer of
Kharas, the Dragonlances themselves) were deliberately left out of player
reach, same restraint as every off-stage major canon character/event in
this project.

`SaveGame.cpp` touches: `ArmorId`'s bound widened 4->5 (append-only-safe).
The equipped/inventory weapon line migrated from `WEAPON sides bonus
name` to `MAGICWEAPON sides bonus magicBonus name` (the old format's
greedy to-end-of-line name couldn't take a new field without corrupting
it) -- `load()` still accepts the legacy `WEAPON` keyword too, same
migration shape as `GOLD`->`STEEL`.

Verified via a throwaway self-test (shop-catalog shape/index arithmetic
per class, purchase/equip/sell round-trip including newly-unsellable
`SolamnicArmor`, `resolvePlayerAttack`'s magic-bonus wiring -- both a
deterministic damage check and a statistical to-hit check, `QuestLoader`
against the real seven-quest `data/quests.txt` including
`REWARD_SOLAMNIC_ARMOR`'s fail-fast case, and a save round-trip covering
`MAGICWEAPON`, legacy `WEAPON`, and the widened `ARMOR` bound), a clean
`/W4` rebuild, a direct check that the user's real `save.txt` (a level-1
Human Fighter) still loads cleanly under the new save format (both before
moving it aside and after restoring it), and the standard piped
character-creation smoke test.

**Not yet verified**: real interactive playthrough (buying/equipping the
magic weapon and seeing the to-hit/damage difference in a real fight;
reaching Sword rank and completing `solamnic_armor` to receive Solamnic
Armor) -- `_getch()` can't be piped, the same limitation flagged for
every quest milestone so far. Needs the user's own keyboard before
calling the UI path fully done.

Docs updated: `docs/CHARACTER_NOTES.md` (new "Magic items" section, a
cross-reference note in "Knights of Solamnia", the "Extending this later"
Equipment bullet), `docs/QUEST_NOTES.md` (grammar table's `REWARD_*` row
and `REQUIRE` vocabulary, "Shipped quests", "Deliberately not in v1"'s
item-rewards bullet reworded rather than reversed, "Extending this
later"), `docs/ZONE_NOTES.md` (the new POI, both the quest-POI list and
the High Clerist's Tower zone writeup), `docs/MILESTONES.md` (new entry +
NEXT UP reordered/expanded), `README.md` Status paragraph.

NEXT UP (`docs/MILESTONES.md`) now leads with Order of the Rose
advancement (unchanged by this milestone -- `solamnic_armor` deliberately
gates on the already-shipped Sword rank instead of waiting on Rose), then
more of the DLA magic items chapter (Rods/Staves/Wands, Crystals and
Gems, Miscellaneous Magic -- real, sourced, unused), then the
still-unused `DELIVER` objective kind, terrain-specific monster pools,
and more monsters. Ask the user before starting any of them.
