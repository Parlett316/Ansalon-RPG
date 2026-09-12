#pragma once

#include "character/CharClass.h"

#include <array>
#include <string>
#include <vector>

namespace character {

struct Character;

// Armor tiers offered across the game's shops -- see docs/CHARACTER_NOTES.md.
// A deliberate subset of the PHB's full armor list (Table 46, Armor Class
// Ratings, p.99), same "pick a few, not the whole book" discipline as this
// project's core-four-class scope: Leather/Studded Leather/Chain Mail/
// Splint Mail/Plate Mail cover a real, meaningfully-priced progression from
// a level-1 character's starting steel up through banked quest/kill
// rewards. Splint Mail is chosen over Banded/Bronze Plate for the same
// AC 4 tier because it's the cheapest real item at that AC (Table 47,
// Armor, p.92: Splint 80stl vs Banded 200stl vs Bronze Plate 400stl for
// identical protection -- prices are the PHB's own gold-piece numbers,
// applied here as Steel Pieces, Krynn's real currency, see
// docs/CHARACTER_NOTES.md's "Gold -> Steel" note). Full Plate (AC1/AC0)
// is still not offered -- even at a recalibrated price it would just
// duplicate Field Plate/Solamnic Armor's niche one slot up, and Solamnic
// Armor (see below) already fills the AC0 spot for a Sword Knight.
// SolamnicArmor itself is NOT one of these buyable tiers -- it's a quest
// reward (docs/QUEST_NOTES.md's solamnic_armor), never sold, deliberately
// absent from kBuyableArmor below.
//
// HideArmor and FieldPlate (ordinals 7-8) were appended here rather than
// inserted in AC order, for the exact reason docs/GOTCHAS.md's raw-
// enum-int save fragility note warns about -- StuddedLeather/PlateMail
// were already appended after SolamnicArmor rather than in AC order when
// they were added, for the same reason (see docs/CHARACTER_NOTES.md and
// SaveGame.cpp's ARMOR bound check, now 9). Display order (kBuyableArmor
// below) is independent of enum ordinal, same as today.
enum class ArmorId {
    None,
    Leather,
    ChainMail,
    SplintMail,
    SolamnicArmor,
    StuddedLeather,
    PlateMail,
    HideArmor,
    FieldPlate,
};

struct ArmorInfo {
    ArmorId id;
    const char* name;
    int armorClass; // Table 46 (p.99): armor alone, before Dexterity/shield
    int costStl;     // Table 47 (p.92), in Steel Pieces

    // How many grid squares a character wearing this armor can move in one
    // combat round (Milestone 185's bigger battlefield + real per-round
    // movement -- see docs/COMBAT_NOTES.md). Sourced from DQoK.pdf's own
    // Armor Table (p.51, visually confirmed via a rendered page image),
    // matched by armor name where one exists (None/Leather/Studded/Chain
    // Mail/Splint Mail/Plate all appear there verbatim); HideArmor/
    // FieldPlate/SolamnicArmor have no DQoK counterpart at all (both are
    // this project's own PHB-table additions -- see this enum's own doc
    // comment) and use an invented, flagged value instead -- see
    // Equipment.cpp's kArmorTable and docs/COMBAT_NOTES.md.
    int maxMovementSquares;
};

const ArmorInfo& armorInfo(ArmorId id);

// Resolves character.equippedArmor into its maxMovementSquares (see
// ArmorInfo above) -- a Shield does NOT reduce this (DQoK.pdf p.51's own
// footnote: "A Shield subtracts 1 AC from any armor it is used with,"
// nothing about movement, and its own Shield row has no movement entry at
// all), so hasShield is deliberately not consulted here.
int movementSquares(const Character& character);

// Real AC order (independent of the enum's own ordinal -- see ArmorId
// above): Hide Armor slots in at AC6, a real gap between Studded Leather
// (AC7) and Chain Mail (AC5) this project used to skip entirely -- see
// docs/CHARACTER_NOTES.md. Field Plate is the new top tier above Plate
// Mail; see MagicWeapon's sibling comment below and docs/CHARACTER_NOTES.md
// for why its price deviates from the real PHB number.
constexpr std::array<ArmorId, 7> kBuyableArmor = {
    ArmorId::Leather,
    ArmorId::StuddedLeather,
    ArmorId::HideArmor,
    ArmorId::ChainMail,
    ArmorId::SplintMail,
    ArmorId::PlateMail,
    ArmorId::FieldPlate,
};

// Table 47 (p.92): a Medium shield is 7stl. Table 46 confirms a shield always
// improves AC by exactly 1 over the same armor without one (e.g. Leather
// alone is AC8, Leather+shield is AC7) -- modeled as a flat -1, applied in
// recomputeArmorClass below.
constexpr int kShieldCostStl = 7;

// PHB, Money and Equipment / Wizards chapter: "Wizards cannot wear any
// armor, for several reasons." A real, sourced restriction -- false only
// for Mage and Tinker (the Mage-analogy class; same fragile-caster
// archetype already established for its saves/steel/hit die, see
// CharClass.cpp). Mirrors the existing Kender/subrace arcane-magic-block
// precedent (Race.h's effectiveCanBeMage) as a genuine class-differentiating
// rule, not an oversight.
bool canWearArmor(ClassId classId);

// Milestone 119, thief backstab: DQoK.pdf's own manual, "The thief may not
// 'back stab' if he has readied armor heavier than leather." True for a
// Thief-group character (see Leveling.h's ClassGroup::Rogue) wearing
// ArmorId::None or ArmorId::Leather; false for anything at or above
// StuddedLeather, and for every non-Thief class. See
// docs/COMBAT_NOTES.md's "Backstab" section for the full positional rule
// this armor gate is one part of.
bool canBackstab(const Character& character);

struct WeaponUpgrade {
    const char* name;
    int damageSides;
    int damageBonus; // Table 44 (p.94): several real weapons carry a flat +1
    int costStl;
};

// One real upgrade per class, sourced from Table 44 (Weapons, p.94-95).
// Mage gets a Quarterstaff (1d6) -- the PHB prices it as "--" (a cut
// length of wood, no real cost), so its shop price is a small invented
// number, flagged in Equipment.cpp the same way kWebnetCostStl is. Tinker
// gets a Light Crossbow (1d4, reusing the Light Quarrel's damage since
// this engine doesn't track ammunition separately from the weapon
// itself) -- a mechanical weapon fitting the class's gadgeteer identity
// rather than a sword-and-board reskin. Never nullptr for any class as of
// the equipment-expansion milestone (see docs/CHARACTER_NOTES.md) -- this
// used to return nullptr for Mage/Tinker, a documented gap that pass
// closed.
const WeaponUpgrade* weaponUpgradeFor(ClassId classId);

// A "+1" enchanted weapon, one per class, sold alongside the mundane
// upgrade above -- see docs/CHARACTER_NOTES.md's "Magic items" section.
// Sourced from the 2nd ed. DMG's "Magical Item Tables," Table 109 (Attack
// Roll Adjustment, p.140, visually confirmed): a "+1" weapon carries XP
// Value 400 (Sword) or 500 (Other Weapon), reused directly as the Steel
// Piece price -- same "reuse the DMG number as the price" convention
// already used for the Potion of Healing (kHealingPotionCostStl below),
// flagged here too since Table 109 gives an XP crafting cost, not a
// separate market-value column the way the Potion's own table does.
// magicBonus applies to BOTH the attack roll and damage (real 2e
// convention) -- see combat::resolvePlayerAttack. Unlike weaponUpgradeFor,
// this is never nullptr: every class gets one, including Mage and Tinker,
// who have no mundane upgrade at all (their first-ever weapon upgrade,
// via magic rather than smithing).
struct MagicWeapon {
    const char* name;
    int damageSides;
    int damageBonus; // the class's own mundane weapon bonus, if any (e.g.
                      // the Cleric's flail) -- stacks with magicBonus below
    int magicBonus;   // +1 to both the attack roll and damage
    int costStl;
};

const MagicWeapon& magicWeaponFor(ClassId classId);

// The Hoopak (sling-staff) -- a real Kender-only weapon, unlike
// weaponUpgradeFor above (one per ClassId): this one is race-gated (any
// class, Kender only), and buyable *in addition to* a Kender's own class
// upgrade, not instead of it -- see docs/CHARACTER_NOTES.md's "Equipment"
// section. Neither the PHB nor Dragonlance Adventures gives the hoopak
// any game stats at all (both only mention it in passing, e.g. DLA p.53's
// "his hoopak or other weapon") -- real numbers come from References/
// DQoK.pdf (Dark Queen of Krynn, the official TSR/SSI Dragonlance computer
// game manual already used for this project's spell census -- see
// "Spellcasting" in docs/CHARACTER_NOTES.md), whose Weapons Table (printed
// p.51, visually confirmed via a rendered page image) gives the hoopak two
// distinct profiles -- "Hoopak (Melee)" 3-8 damage vs. man-sized (1d6+2)
// and "Hoopak (Missile)" 2-5 (1d4+1) -- both footnoted "Only usable by
// kender characters." This engine has no ranged/melee distinction for any
// weapon (see the Tinker's Light Crossbow above), so only the
// higher-damage Melee profile is modeled, the same "pick the number that
// matters, flag what's lost" simplification as Meteor Swarm's uniform
// damage (docs/CHARACTER_NOTES.md's Spellcasting census). DQoK's manual
// has no in-game currency to reuse (unlike the Potion/magic weapons
// above, which reuse real DMG gp values), so costStl is an invented,
// flagged number -- calibrated to the Fighter's Two-Handed Sword, this
// project's closest real peer by average damage (1d6+2 and 1d10 both
// average 5.5).
inline constexpr const char* kHoopakName = "Hoopak";
constexpr int kHoopakDamageSides = 6;
constexpr int kHoopakDamageBonus = 2;
constexpr int kHoopakCostStl = 50;

// A carried, not-currently-equipped item -- see Character::inventory below.
// Only the fields relevant to `kind` are meaningful; the others stay at
// their default.
enum class ItemKind {
    Armor,
    Shield,
    Weapon,
    Potion,
    Webnet,
    BroochOfImog,
    QuestItem,
};

struct InventoryItem {
    ItemKind kind = ItemKind::Armor;
    ArmorId armorId = ArmorId::None;     // valid when kind == Armor
    std::string weaponName;              // valid when kind == Weapon
    int weaponDamageSides = 0;           // valid when kind == Weapon
    int weaponDamageBonus = 0;           // valid when kind == Weapon
    int weaponMagicBonus = 0;            // valid when kind == Weapon
    std::string questItemId;             // valid when kind == QuestItem --
                                          // stable id, matched against
                                          // quest::Objective::targetId for a
                                          // DELIVER objective and against a
                                          // zone POI's GRANTS_ITEM id
    std::string questItemName;           // valid when kind == QuestItem --
                                          // display label, e.g. "Raw
                                          // Tharkadan Ore"
};

// Human-readable label for a carried item, same format as ShopItem::label
// (e.g. "Chain Mail (AC 5)", "Shield (-1 AC)", "Two-Handed Sword (1d10)").
std::string inventoryItemLabel(const InventoryItem& item);

// Equips character.inventory[index]: removes it from inventory and swaps it
// into the matching slot (equippedArmor/hasShield/weapon*), pushing
// whatever was previously equipped in that slot back into inventory (gear
// is swapped, never destroyed). Recomputes armorClass when armor/shield
// changes. Returns false (no-op) if index is out of range. There is
// deliberately no "unequip to nothing" action -- equipping is always a
// swap; going fully bare-handed/unarmored isn't offered, same spirit as
// the shop's no-sell-back scope cut.
bool equipInventoryItem(Character& character, int index);

// Recomputes Character::armorClass from equippedArmor/hasShield/Dexterity.
// Base AC is armorInfo(equippedArmor).armorClass (10 for ArmorId::None,
// exactly matching the pre-equipment unarmored default), so for a
// character who never buys anything this is a strict no-op equivalent to
// the original `10 - acAdjustmentForDexterity(dex)` formula.
void recomputeArmorClass(Character& character);

// Which "slot" a catalog line represents -- purchaseItem dispatches on
// this directly instead of recomputing a position from a fixed,
// unfiltered sequence the way it used to, since different shops now
// offer different subsets of these slots (see ShopCatalog below).
enum class ShopItemKind {
    ArmorTier,
    Shield,
    WeaponUpgrade,
    KenderWeapon,
    MagicWeapon,
    Potion,
    Webnet,
    Brooch,
};

// One purchasable line in the shop screen -- built fresh each time the
// shop is opened from the character's current class/steel/gear and the
// shop's own catalog (see ShopCatalog below), so GameLoop::handleShop()
// only has to render+select, not decide what's on offer (same split of
// responsibility Spellcasting.h already established between castSpell()
// and GameLoop::runCombat()).
struct ShopItem {
    ShopItemKind kind;
    ArmorId armorId = ArmorId::None; // meaningful only when kind == ArmorTier
    std::string label;   // e.g. "Chain Mail (AC 5)" or "Two-Handed Sword (1d10)"
    int costStl;
    bool alreadyOwned;
    bool buyable; // false if the class can't use this item at all (e.g. armor for a Mage)
};

// Which shop this is -- decides which ShopItemKind slots appear at all
// (see catalogDef in Equipment.cpp) -- an honestly invented gameplay-
// tuning decision, not sourced content, same flagged status as
// docs/COMBAT_NOTES.md's encounterChancePercent/kBiasWeight. Set via a
// zone file's SHOP <char> <catalog-name> line -- world::ZoneLoader
// stores the name as a plain, validated string on PointOfInterest and
// never references this enum, keeping world:: decoupled from
// character:: per docs/ARCHITECTURE.md; game::GameLoop::handleShop does
// the string->enum translation right before calling into this file. See
// docs/CHARACTER_NOTES.md's "Six shops, six catalogs".
enum class ShopCatalog {
    General,
    Armory,
    MarketGoods,
    Salvage,
    Bazaar,
    HarborTrade,
    Magic,
};

std::vector<ShopItem> availableShopItems(const Character& character, ShopCatalog catalog = ShopCatalog::General);

struct PurchaseResult {
    bool success;
    std::string message; // shown in the shop frame either way
};

// Applies buying availableShopItems(character, catalog)[index]: deducts
// steel and adds the matching InventoryItem (dispatched on
// ShopItem::kind, not a recomputed position -- see ShopItemKind above).
// Does nothing to the character if the purchase is rejected (insufficient
// steel, class can't use the item, or already owned) -- check
// PurchaseResult::success. catalog must match whatever availableShopItems
// call produced the index being purchased (GameLoop::handleShop always
// passes the same poi->shopCatalog to both).
PurchaseResult purchaseItem(Character& character, int index, ShopCatalog catalog = ShopCatalog::General);

// One line in the shop's "sell" view -- see sellableItems below.
struct SellItem {
    std::string label;
    int valueStl;
    bool sellable; // false for a starting weapon that was never bought --
                    // see sellableItems
};

// Built fresh each time the shop is opened, same "compute on demand, don't
// cache" split as availableShopItems -- one entry per
// character.inventory[i], same index alignment as inventory itself, so
// GameLoop can sell-by-index directly. There is no printed 2e resale-price
// rule for mundane equipment (checked the actual PHB/DMG text -- see
// docs/CHARACTER_NOTES.md's "Selling gear back"), so the value here is a
// deliberately flagged, invented convention: half of the item's real shop
// price (armorInfo/kShieldCostStl/WeaponUpgrade::costStl/MagicWeapon::costStl),
// floored. A starting weapon (the class's original dagger/mace/shortsword/
// longsword/wrench, which can land in inventory after an upgrade is
// equipped) and SolamnicArmor (a quest reward, never sold -- see
// docs/QUEST_NOTES.md's solamnic_armor) were never sold in any shop and
// have no established price, so both are marked unsellable rather than
// assigned an invented number.
std::vector<SellItem> sellableItems(const Character& character);

// Applies selling character.inventory[index]: removes it from inventory
// and adds its resale value to steelPieces. Reuses PurchaseResult's shape
// (success/message) rather than a duplicate type. Does nothing if index is
// out of range or the item isn't sellable -- check PurchaseResult::success.
PurchaseResult sellItem(Character& character, int index);

// Potion of Healing (2nd ed. DMG p.142, "Healing": "the potion restores
// 2d4+2 hit points of damage") -- sold at every shop (see
// docs/CHARACTER_NOTES.md's "Potions"), framed as a scavenged
// pre-Cataclysm relic rather than a merchant's own brew: real clerical
// healing magic doesn't return to Krynn until Goldmoon's Disks of
// Mishakal, early in Dragons of Autumn Twilight's own timeline, so an
// ordinary shop stocking freshly-made magic healing would contradict the
// setting. Priced straight from the DMG's own Magical Items table (p.134,
// "Healing" row, Value 200gp), applied as Steel Pieces per this project's
// established Gold -> Steel convention.
constexpr int kHealingPotionCostStl = 200;
constexpr int kHealingPotionDiceCount = 2;
constexpr int kHealingPotionDiceSides = 4;
constexpr int kHealingPotionFlatBonus = 2;

// -1 if the character carries no Potion, otherwise the character.inventory
// index of the first one found -- lets GameLoop::runCombat drink "a"
// potion with no picker UI, the same "nothing to actually select"
// simplification Spellcasting's one-known-spell already established
// (every carried potion is identical, so which one doesn't matter).
int firstPotionIndex(const Character& character);

// Removes character.inventory[index] and heals kHealingPotionDiceCount d
// kHealingPotionDiceSides + kHealingPotionFlatBonus hit points, capped at
// maxHp. Returns {false, ...} if index is out of range or isn't a Potion.
// Reuses PurchaseResult's shape, same "generic action outcome" reuse
// sellItem already established rather than a duplicate result type.
PurchaseResult drinkPotion(Character& character, int index);

// Webnet and Brooch of Imog (Dragonlance Adventures p.93/p.92, visually
// confirmed via rendered page images) -- see docs/CHARACTER_NOTES.md's
// "Magic items" section. Both items are Mage-only per their own text
// ("This item is only useful to a magic-user"). Neither has a printed
// Steel Piece price in the source, unlike the Potion/magic weapons above
// (reused from the DMG's own tables) -- these two numbers are invented
// and flagged, calibrated relative to the Potion (200stl, one-shot) and a
// "+1" weapon (400-500stl, permanent).
constexpr int kWebnetCostStl = 150;
constexpr int kBroochOfImogCostStl = 500;

// -1 if the character carries no Webnet, otherwise the character.inventory
// index of the first one found -- same "nothing to actually select"
// simplification as firstPotionIndex.
int firstWebnetIndex(const Character& character);

// -1 if the character carries no Brooch of Imog, otherwise the
// character.inventory index of the first one found.
int firstBroochIndex(const Character& character);

// True if the character owns a Brooch of Imog AND hasn't already used its
// daily charge today -- same day-gate shape as
// Spellcasting.h's hasMemorizedSpellsAvailable, keyed off Character::lastBroochUseDay
// (see Character.h) the same way Rest keys off lastRestDay.
bool broochAvailableToday(const Character& character, long long today);

// Removes character.inventory[index] (consumed, one-shot -- see docs/
// CHARACTER_NOTES.md). Returns {false, ...} if index is out of range or
// isn't a Webnet. Deliberately does NOT touch any "does the monster's next
// attack land" state -- that's game::GameLoop::runCombat's own local flag,
// the same division of labor drinkPotion already has between the HP math
// it owns and the combat log lines GameLoop owns.
PurchaseResult useWebnet(Character& character, int index);

// Sets character.lastBroochUseDay = today (does NOT remove anything from
// inventory -- the Brooch is worn/carried, not consumed). Returns
// {false, ...} if the character doesn't own one or broochAvailableToday
// is already false. Like useWebnet, does not itself decide whether the
// monster's attacks land this fight -- GameLoop::runCombat's own local
// flag handles that.
PurchaseResult activateBrooch(Character& character, long long today);

// Staff of Striking/Curing (Dragonlance Adventures p.91, visually
// confirmed via a rendered page image) -- see docs/CHARACTER_NOTES.md's
// "Magic items" section. A quest reward, Cleric-only, never sold (see
// data/quests.txt's staff_of_striking_curing) -- granted as an ordinary
// ItemKind::Weapon InventoryItem, so equipping/unequipping/resale-blocking
// all reuse the existing weapon machinery with no extra code (see
// magicWeaponFor's doc comment above for the to-hit/damage convention this
// follows). kStaffCureDiceSides is an invented-and-flagged number, not a
// DMG-sourced one -- see the .cpp for why.
inline constexpr const char* kStaffOfStrikingCuringName = "Staff of Striking/Curing";
constexpr int kStaffMagicBonus = 3;    // "it strikes as a +3 weapon"
constexpr int kStaffDamageSides = 6;   // 1d6+3 = the book's "4-9 points of damage"
constexpr int kStaffCureDiceSides = 8; // invented -- see the .cpp comment on useStaffCure

// True if character.weaponName or any carried InventoryItem matches
// kStaffOfStrikingCuringName -- i.e. the character owns the staff at all,
// equipped or not. Used by staffCureAvailableToday below and by
// GameLoop::offerOrTurnInQuest to avoid granting a duplicate on repeat
// turn-in (can't currently happen -- a quest only turns in once -- but
// matches the defensive style already used elsewhere in this file).
bool ownsStaffOfStrikingCuring(const Character& character);

// True if the character owns the staff AND hasn't already used its cure
// function today -- same day-gate shape as broochAvailableToday, keyed off
// Character::lastStaffCureDay. See Character.h and the "Magic items"
// section of docs/CHARACTER_NOTES.md for why this project models curing as
// a flat once-per-day action rather than the book's 50-charge pool (the
// pool can never actually bind under this engine's constraints).
bool staffCureAvailableToday(const Character& character, long long today);

// Heals kStaffCureDiceSides (1d8) hit points, capped at maxHp -- same
// "heal self, capped" shape as drinkPotion, but doesn't consume anything
// (the staff is worn/carried, not a one-shot item) -- sets
// lastStaffCureDay instead, same as activateBrooch. Returns {false, ...}
// if the character doesn't own the staff or staffCureAvailableToday is
// already false.
PurchaseResult useStaffCure(Character& character, long long today);

// Which of the four "press i in combat" consumables/items this entry is --
// see availableCombatItems below. Order here is the same fixed priority
// GameLoop::runCombat's old single-choice 'i' handling used to hard-code
// (potion, then Webnet, then Brooch, then Staff), preserved as the natural
// list order rather than re-litigated.
enum class CombatItemKind { Potion, Webnet, Brooch, StaffCure };

// One in-frame USE-menu entry (Milestone 115) -- `label` is already the
// full display string (e.g. "Potion of Healing", "Webnet", "Brooch of
// Imog", "Staff of Striking (cure)"), so game::GameLoop::runCombat and
// render::MapRenderer both just print it, no further name lookup needed.
struct CombatItem {
    CombatItemKind kind;
    std::string label;
};

// Every consumable/item usable as this round's action right now, in fixed
// order (Potion, Webnet, Brooch, Staff) -- built fresh each call from
// firstPotionIndex/firstWebnetIndex/broochAvailableToday/
// staffCureAvailableToday above, the same "no new state, just combine
// what's already tracked" shape ShopCatalog uses for its own listings.
// Replaces the old fixed-priority-pick-the-first-one 'i' behavior, which
// meant a character carrying both a Potion and a Webnet could never reach
// the Webnet at all -- both GameLoop::runCombat (to build the chooser) and
// MapRenderer::drawCombatFrame (to name the command row's one-line hint)
// call this, so the two can never drift out of sync the way the old
// hand-duplicated priority chain could.
std::vector<CombatItem> availableCombatItems(const Character& character, long long today);

// Frostreaver (Dragonlance Adventures p.94, visually confirmed via a
// rendered page image) -- see docs/CHARACTER_NOTES.md's "Magic items"
// section. A quest reward (data/quests.txt's frostreaver_salvage,
// REQUIRE str_13), tied to the already-shipped Ice Wall Castle location
// and Thanoi monster (data/monsters.txt, TERRAIN_BIAS toward glacier).
// DLA's own text: "the equivalent of a heavy battle axe +4... can only be
// wielded by a character with a Strength of 13 or greater." The PHB's
// Table 44 (Weapons, p.94) has no separate "heavy battle axe" line -- its
// one axe entry, plain "Battle axe," is 1d8 -- so that's the base damage
// die the "+4" applies on top of.
//
// DLA also gives the item a real weakness -- above-freezing temperatures
// melt it useless within a day (1d6 hours in a warm environment). This
// project has no "item destroyed by environment" mechanic anywhere and
// won't build one just for this weapon (see CLAUDE.md's "no premature
// abstraction") -- simplified instead to "only carries its +4 bonus while
// standing on glacier terrain," which is why kFrostreaverMagicBonus below
// is NOT baked into the granted InventoryItem's weaponMagicBonus (that
// stays 0, its true off-glacier baseline -- just a mundane heavy battle
// axe). game::GameLoop::runCombat applies the +4 as a this-fight-only
// local bonus when the terrain is glacier, the same mechanism already
// used for spell buffs (see combat::AttackOutcome's doc comment on
// thac0Bonus/damageBonus) -- not a permanent character stat.
inline constexpr const char* kFrostreaverName = "Frostreaver";
constexpr int kFrostreaverDamageSides = 8; // PHB Table 44, "Battle axe": 1d8
constexpr int kFrostreaverMagicBonus = 4;  // "+4" to both hit and damage, glacier-only
constexpr int kFrostreaverMinStrength = 13;

// PHB p.73, "Effects of Specialization": "+1 bonus to all his attack
// rolls... and a +2 bonus to all damage rolls (in addition to bonuses for
// Strength and magic)." Applied whenever Character::specializedWeapon is
// true (see Character.h), regardless of which tier of the class's own
// weapon lineage is currently equipped, and stacking with
// kFrostreaverMagicBonus if both apply -- this project doesn't track
// weapon-type identity beyond the single equipped-weapon fields, so a
// specialist wielding the (unrelated, axe-type) Frostreaver still gets
// both bonuses rather than the RAW "wrong weapon, no specialization"
// restriction. A deliberate simplification, not an oversight -- see
// docs/CHARACTER_NOTES.md's "Weapon Specialization" section.
constexpr int kWeaponSpecializationToHitBonus = 1;
constexpr int kWeaponSpecializationDamageBonus = 2;

// The Tinker's Light Crossbow upgrade (see kTinkerUpgrade in
// Equipment.cpp) -- this project's one real ranged weapon as of Milestone
// 114's positional combat grid (docs/COMBAT_NOTES.md). Same
// name-string-compare pattern as kFrostreaverName above: no new
// character::Character field, no threading through the purchase/equip
// path, just a plain weaponName check at the one call site
// (GameLoop::runCombat) that needs to know.
inline constexpr const char* kLightCrossbowName = "Light Crossbow";

// A quest item (ItemKind::QuestItem) -- a real, granted-in-the-world
// object carried toward a quest::ObjectiveKind::Deliver objective, never
// sold and never equipped (see docs/QUEST_NOTES.md's "DELIVER"). Unlike
// Potion/Webnet/BroochOfImog, there is no fixed catalog: a zone POI's
// GRANTS_ITEM line supplies both `questItemId` and `questItemName`
// directly, the same "id + display text" shape InventoryItem::weaponName
// already carries for a Weapon.
//
// -1 if the character carries no QuestItem with this id, otherwise the
// character.inventory index of the first one found -- used both by
// game::GameLoop::talkTo (to avoid granting a duplicate on repeat talk)
// and by game::objectiveProgress's Deliver case.
int findQuestItemIndex(const Character& character, const std::string& questItemId);

} // namespace character
