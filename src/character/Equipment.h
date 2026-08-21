#pragma once

#include "character/CharClass.h"

#include <array>
#include <string>
#include <vector>

namespace character {

struct Character;

// Armor tiers offered at the General Store -- see docs/CHARACTER_NOTES.md.
// A deliberate subset of the PHB's full armor list (Table 46, Armor Class
// Ratings, p.99), same "pick a few, not the whole book" discipline as this
// project's core-four-class scope: Leather/Chain Mail/Splint Mail cover a
// real, meaningfully-priced progression a level-1 character can actually
// reach for. Splint Mail is chosen over Banded/Bronze Plate for the same
// AC 4 tier because it's the cheapest real item at that AC (Table 47,
// Armor, p.92: Splint 80stl vs Banded 200stl vs Bronze Plate 400stl for
// identical protection -- prices are the PHB's own gold-piece numbers,
// applied here as Steel Pieces, Krynn's real currency, see
// docs/CHARACTER_NOTES.md's "Gold -> Steel" note) -- Plate Mail and
// heavier are priced far beyond any level-1 starting steel and aren't
// offered yet. SolamnicArmor is NOT one of these -- it's a quest reward
// (docs/QUEST_NOTES.md's solamnic_armor), never sold, deliberately absent
// from kBuyableArmor below.
enum class ArmorId {
    None,
    Leather,
    ChainMail,
    SplintMail,
    SolamnicArmor,
};

struct ArmorInfo {
    ArmorId id;
    const char* name;
    int armorClass; // Table 46 (p.99): armor alone, before Dexterity/shield
    int costStl;     // Table 47 (p.92), in Steel Pieces
};

const ArmorInfo& armorInfo(ArmorId id);

constexpr std::array<ArmorId, 3> kBuyableArmor = {
    ArmorId::Leather,
    ArmorId::ChainMail,
    ArmorId::SplintMail,
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

struct WeaponUpgrade {
    const char* name;
    int damageSides;
    int damageBonus; // Table 44 (p.94): several real weapons carry a flat +1
    int costStl;
};

// One real upgrade per class that can use one, sourced from Table 44
// (Weapons, p.94) -- nullptr for Mage/Tinker (no upgrade offered; their
// dagger/wrench stays as-is, consistent with wizards' traditionally short
// allowed-weapons list and their fragile-caster identity being intentional,
// not a gap).
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

// One purchasable line in the shop screen -- built fresh each time the
// shop is opened from the character's current class/steel/gear, so
// GameLoop::handleShop() only has to render+select, not decide what's on
// offer (same split of responsibility Spellcasting.h already established
// between castSpell() and GameLoop::runCombat()).
struct ShopItem {
    std::string label;   // e.g. "Chain Mail (AC 5)" or "Two-Handed Sword (1d10)"
    int costStl;
    bool alreadyOwned;
    bool buyable; // false if the class can't use this item at all (e.g. armor for a Mage)
};

std::vector<ShopItem> availableShopItems(const Character& character);

struct PurchaseResult {
    bool success;
    std::string message; // shown in the shop frame either way
};

// Applies buying availableShopItems(character)[index]: deducts steel,
// updates equippedArmor/hasShield/weaponName/weaponDamageSides/
// weaponDamageBonus/weaponMagicBonus, and calls recomputeArmorClass. Does
// nothing to the character if the purchase is rejected (insufficient
// steel, class can't use the item, or already owned) -- check
// PurchaseResult::success.
PurchaseResult purchaseItem(Character& character, int index);

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
// Spellcasting.h's hasSpellSlotAvailable, keyed off Character::lastBroochUseDay
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
