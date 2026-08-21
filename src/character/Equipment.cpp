#include "character/Equipment.h"
#include "character/Ability.h"
#include "character/Character.h"
#include "character/Dice.h"

#include <algorithm>

namespace character {

namespace {

// Table 46 (Armor Class Ratings, p.99) for the AC column, Table 47
// (Armor, p.92) for cost -- both visually confirmed against the rendered
// PHB page, not OCR text (this scan's OCR badly garbles table columns).
// SolamnicArmor's AC 0 is instead from Dragonlance Adventures (TSR 2021)
// p.93-94, visually confirmed: "Solamnic armor is equal to AC 0 (plate +1
// and shield +1)" -- a quest reward (see docs/QUEST_NOTES.md's
// solamnic_armor), cost 0 since it's never sold or sellable (see
// resaleValueStl below).
constexpr std::array<ArmorInfo, 5> kArmorTable = {{
    {ArmorId::None, "No Armor", 10, 0},
    {ArmorId::Leather, "Leather Armor", 8, 5},
    {ArmorId::ChainMail, "Chain Mail", 5, 75},
    {ArmorId::SplintMail, "Splint Mail", 4, 80},
    {ArmorId::SolamnicArmor, "Solamnic Armor", 0, 0},
}};

// Table 44 (Weapons, p.94), visually confirmed: Two-Handed Sword 1d10/50stl
// (no flat bonus), Footman's Flail 1d6+1/15stl, Long Sword 1d8/15stl (no
// flat bonus). Mage/Tinker deliberately have no entry -- see Equipment.h.
const WeaponUpgrade kFighterUpgrade{"Two-Handed Sword", 10, 0, 50};
const WeaponUpgrade kClericUpgrade{"Footman's Flail", 6, 1, 15};
const WeaponUpgrade kThiefUpgrade{"Long Sword", 8, 0, 15};

// "+1" enchanted weapons -- see Equipment.h's MagicWeapon for sourcing
// (DMG Table 109, p.140: Sword +1 = 400stl, Other Weapon +1 = 500stl).
// Mage's and Tinker's base weapon (dagger/wrench, CharClass.cpp) carries no
// mundane upgrade at all, so these are their first-ever weapon upgrade.
const MagicWeapon kFighterMagicWeapon{"Ensorcelled Two-Handed Sword", 10, 0, 1, 400};
const MagicWeapon kClericMagicWeapon{"Ensorcelled Footman's Flail", 6, 1, 1, 500};
const MagicWeapon kThiefMagicWeapon{"Ensorcelled Long Sword", 8, 0, 1, 400};
const MagicWeapon kMageMagicWeapon{"Ensorcelled Dagger", 4, 0, 1, 500};
const MagicWeapon kTinkerMagicWeapon{"Ensorcelled Wrench", 4, 0, 1, 500};

} // namespace

const ArmorInfo& armorInfo(ArmorId id) {
    for (const auto& info : kArmorTable) {
        if (info.id == id) return info;
    }
    return kArmorTable[0]; // unreachable given ArmorId only has the values above
}

std::string inventoryItemLabel(const InventoryItem& item) {
    switch (item.kind) {
        case ItemKind::Armor: {
            const ArmorInfo& info = armorInfo(item.armorId);
            return std::string(info.name) + " (AC " + std::to_string(info.armorClass) + ")";
        }
        case ItemKind::Shield:
            return "Shield (-1 AC)";
        case ItemKind::Weapon: {
            int totalDamageBonus = item.weaponDamageBonus + item.weaponMagicBonus;
            std::string label = item.weaponName + " (1d" + std::to_string(item.weaponDamageSides) +
                                 (totalDamageBonus > 0 ? "+" + std::to_string(totalDamageBonus) : "") + ")";
            if (item.weaponMagicBonus > 0) {
                label += ", +" + std::to_string(item.weaponMagicBonus) + " to hit";
            }
            return label;
        }
        case ItemKind::Potion:
            return "Potion of Healing (2d4+2 hp)";
        case ItemKind::Webnet:
            return "Webnet";
        case ItemKind::BroochOfImog:
            return "Brooch of Imog";
        case ItemKind::QuestItem:
            return item.questItemName;
    }
    return "";
}

bool equipInventoryItem(Character& character, int index) {
    if (index < 0 || index >= static_cast<int>(character.inventory.size())) return false;

    InventoryItem item = character.inventory[index];
    character.inventory.erase(character.inventory.begin() + index);

    switch (item.kind) {
        case ItemKind::Armor: {
            ArmorId old = character.equippedArmor;
            character.equippedArmor = item.armorId;
            if (old != ArmorId::None) {
                character.inventory.push_back(InventoryItem{ItemKind::Armor, old, "", 0, 0});
            }
            recomputeArmorClass(character);
            break;
        }
        case ItemKind::Shield: {
            bool hadShield = character.hasShield;
            character.hasShield = true;
            if (hadShield) {
                // Can't normally happen -- purchaseItem blocks buying a
                // second shield while one is equipped or already carried --
                // but degrade gracefully rather than losing the swapped item
                // silently if this state is ever reached (e.g. a hand-edited
                // save file).
                character.inventory.push_back(InventoryItem{ItemKind::Shield, ArmorId::None, "", 0, 0});
            }
            recomputeArmorClass(character);
            break;
        }
        case ItemKind::Weapon: {
            InventoryItem old{ItemKind::Weapon, ArmorId::None, character.weaponName,
                               character.weaponDamageSides, character.weaponDamageBonus,
                               character.weaponMagicBonus};
            character.weaponName = item.weaponName;
            character.weaponDamageSides = item.weaponDamageSides;
            character.weaponDamageBonus = item.weaponDamageBonus;
            character.weaponMagicBonus = item.weaponMagicBonus;
            character.inventory.push_back(std::move(old));
            break;
        }
        case ItemKind::Potion:
        case ItemKind::Webnet:
        case ItemKind::BroochOfImog:
        case ItemKind::QuestItem:
            // Not equippable -- GameLoop::handleInventory branches on
            // ItemKind before ever calling here for these; using them is a
            // different action (see drinkPotion/useWebnet/activateBrooch
            // below, or simply carrying a QuestItem toward a DELIVER
            // objective), unreachable here in practice.
            break;
    }
    return true;
}

bool canWearArmor(ClassId classId) {
    return classId != ClassId::Mage && classId != ClassId::Tinker;
}

const WeaponUpgrade* weaponUpgradeFor(ClassId classId) {
    switch (classId) {
        case ClassId::Fighter: return &kFighterUpgrade;
        case ClassId::Cleric: return &kClericUpgrade;
        case ClassId::Thief: return &kThiefUpgrade;
        case ClassId::Mage: return nullptr;
        case ClassId::Tinker: return nullptr;
    }
    return nullptr;
}

const MagicWeapon& magicWeaponFor(ClassId classId) {
    switch (classId) {
        case ClassId::Fighter: return kFighterMagicWeapon;
        case ClassId::Cleric: return kClericMagicWeapon;
        case ClassId::Thief: return kThiefMagicWeapon;
        case ClassId::Mage: return kMageMagicWeapon;
        case ClassId::Tinker: return kTinkerMagicWeapon;
    }
    return kFighterMagicWeapon; // unreachable given ClassId only has the values above
}

void recomputeArmorClass(Character& character) {
    int base = armorInfo(character.equippedArmor).armorClass;
    int ac = base - acAdjustmentForDexterity(character.scores.dexterity);
    if (character.hasShield) ac -= 1;
    character.armorClass = ac;
}

namespace {

bool ownsArmor(const Character& character, ArmorId id) {
    if (character.equippedArmor == id) return true;
    for (const auto& item : character.inventory) {
        if (item.kind == ItemKind::Armor && item.armorId == id) return true;
    }
    return false;
}

bool ownsShield(const Character& character) {
    if (character.hasShield) return true;
    for (const auto& item : character.inventory) {
        if (item.kind == ItemKind::Shield) return true;
    }
    return false;
}

bool ownsWeapon(const Character& character, const std::string& name) {
    if (character.weaponName == name) return true;
    for (const auto& item : character.inventory) {
        if (item.kind == ItemKind::Weapon && item.weaponName == name) return true;
    }
    return false;
}

// Unlike the Webnet/Potion (consumed on use, so buying duplicates is
// meaningful), a Brooch of Imog is never destroyed -- a second one would
// grant nothing, since the daily charge is tracked per-character (see
// Character::lastBroochUseDay), not per-item. Blocked from a duplicate
// purchase the same way armor/shield/weapons already are.
bool ownsBrooch(const Character& character) {
    for (const auto& item : character.inventory) {
        if (item.kind == ItemKind::BroochOfImog) return true;
    }
    return false;
}

} // namespace

std::vector<ShopItem> availableShopItems(const Character& character) {
    std::vector<ShopItem> items;
    bool wearsArmor = canWearArmor(character.charClass);

    for (ArmorId id : kBuyableArmor) {
        const ArmorInfo& info = armorInfo(id);
        ShopItem item;
        item.label = std::string(info.name) + " (AC " + std::to_string(info.armorClass) + ")";
        item.costStl = info.costStl;
        item.alreadyOwned = ownsArmor(character, id);
        item.buyable = wearsArmor && !item.alreadyOwned;
        items.push_back(std::move(item));
    }

    ShopItem shield;
    shield.label = "Shield (-1 AC)";
    shield.costStl = kShieldCostStl;
    shield.alreadyOwned = ownsShield(character);
    shield.buyable = wearsArmor && !shield.alreadyOwned;
    items.push_back(std::move(shield));

    if (const WeaponUpgrade* upgrade = weaponUpgradeFor(character.charClass)) {
        ShopItem weapon;
        weapon.label = std::string(upgrade->name) + " (1d" + std::to_string(upgrade->damageSides) +
                        (upgrade->damageBonus > 0 ? "+" + std::to_string(upgrade->damageBonus) : "") + ")";
        weapon.costStl = upgrade->costStl;
        weapon.alreadyOwned = ownsWeapon(character, upgrade->name);
        weapon.buyable = !weapon.alreadyOwned;
        items.push_back(std::move(weapon));
    }

    // The class's "+1" enchanted weapon -- see Equipment.h's MagicWeapon.
    // Unlike weaponUpgradeFor, this is never absent: every class (including
    // Mage/Tinker, who have no mundane upgrade above) gets one.
    {
        const MagicWeapon& magic = magicWeaponFor(character.charClass);
        int totalDamageBonus = magic.damageBonus + magic.magicBonus;
        ShopItem weapon;
        weapon.label = std::string(magic.name) + " (1d" + std::to_string(magic.damageSides) +
                        (totalDamageBonus > 0 ? "+" + std::to_string(totalDamageBonus) : "") +
                        ", +" + std::to_string(magic.magicBonus) + " to hit)";
        weapon.costStl = magic.costStl;
        weapon.alreadyOwned = ownsWeapon(character, magic.name);
        weapon.buyable = !weapon.alreadyOwned;
        items.push_back(std::move(weapon));
    }

    // Potion of Healing -- see Equipment.h for the pre-Cataclysm-relic
    // framing. Stackable (alreadyOwned always false, unlike armor/weapons
    // which block owning a duplicate) and open to every class (no
    // canWearArmor-style restriction).
    ShopItem potion;
    potion.label = "Potion of Healing (2d4+2 hp)";
    potion.costStl = kHealingPotionCostStl;
    potion.alreadyOwned = false;
    potion.buyable = true;
    items.push_back(std::move(potion));

    // Webnet and Brooch of Imog -- Mage-only per their own DLA text (see
    // Equipment.h), always listed (buyable=false for other classes) same
    // "show it, but grey it out" precedent as armor/shield for a Mage.
    // Stackable like the Potion above -- alreadyOwned always false.
    bool isMage = character.charClass == ClassId::Mage;

    ShopItem webnet;
    webnet.label = "Webnet (negates the foe's next attack)";
    webnet.costStl = kWebnetCostStl;
    webnet.alreadyOwned = false;
    webnet.buyable = isMage;
    items.push_back(std::move(webnet));

    ShopItem brooch;
    brooch.label = "Brooch of Imog (blocks all attacks for the rest of a fight, once/day)";
    brooch.costStl = kBroochOfImogCostStl;
    brooch.alreadyOwned = ownsBrooch(character);
    brooch.buyable = isMage && !brooch.alreadyOwned;
    items.push_back(std::move(brooch));

    return items;
}

PurchaseResult purchaseItem(Character& character, int index) {
    std::vector<ShopItem> items = availableShopItems(character);
    if (index < 0 || index >= static_cast<int>(items.size())) {
        return {false, "Nothing to buy there."};
    }
    const ShopItem& item = items[index];
    if (item.alreadyOwned) {
        return {false, "You already have that."};
    }
    if (!item.buyable) {
        return {false, "Wizards cannot wear armor or a shield."};
    }
    if (character.steelPieces < item.costStl) {
        return {false, "You don't have enough steel for that."};
    }

    character.steelPieces -= item.costStl;

    // Buying only adds to inventory now -- equipping is a separate,
    // deliberate action via GameLoop::handleInventory ('i'). See
    // docs/CHARACTER_NOTES.md.
    int armorCount = static_cast<int>(kBuyableArmor.size());
    const WeaponUpgrade* upgrade = weaponUpgradeFor(character.charClass);
    int weaponIndex = upgrade != nullptr ? armorCount + 1 : -1;
    // The magic weapon line always follows the (optional) mundane upgrade
    // line -- see availableShopItems.
    int magicWeaponIndex = upgrade != nullptr ? armorCount + 2 : armorCount + 1;
    // Potion, Webnet, and Brooch of Imog are always the final three lines,
    // in that fixed order -- see availableShopItems.
    int potionIndex = magicWeaponIndex + 1;
    int webnetIndex = potionIndex + 1;
    int broochIndex = potionIndex + 2;
    bool isPotion = false;
    if (index < armorCount) {
        character.inventory.push_back(InventoryItem{ItemKind::Armor, kBuyableArmor[index], "", 0, 0});
    } else if (index == armorCount) {
        character.inventory.push_back(InventoryItem{ItemKind::Shield, ArmorId::None, "", 0, 0});
    } else if (index == weaponIndex) {
        character.inventory.push_back(
            InventoryItem{ItemKind::Weapon, ArmorId::None, upgrade->name, upgrade->damageSides, upgrade->damageBonus});
    } else if (index == magicWeaponIndex) {
        const MagicWeapon& magic = magicWeaponFor(character.charClass);
        character.inventory.push_back(InventoryItem{ItemKind::Weapon, ArmorId::None, magic.name,
                                                      magic.damageSides, magic.damageBonus, magic.magicBonus});
    } else if (index == webnetIndex) {
        character.inventory.push_back(InventoryItem{ItemKind::Webnet, ArmorId::None, "", 0, 0});
    } else if (index == broochIndex) {
        character.inventory.push_back(InventoryItem{ItemKind::BroochOfImog, ArmorId::None, "", 0, 0});
    } else {
        // The one remaining catalog line -- the Potion of Healing.
        character.inventory.push_back(InventoryItem{ItemKind::Potion, ArmorId::None, "", 0, 0});
        isPotion = true;
    }

    std::string hint = ". Press 'i' to equip it.";
    if (isPotion) {
        hint = ". Press 'i' to drink it.";
    } else if (index == webnetIndex || index == broochIndex) {
        hint = ". Press 'i' in combat to use it.";
    }
    return {true, "Bought " + item.label + hint};
}

namespace {

// See SellItem/sellableItems in Equipment.h for why this is an invented,
// flagged convention rather than a sourced number.
int resaleValueStl(const Character& character, const InventoryItem& item, bool& sellable) {
    switch (item.kind) {
        case ItemKind::Armor:
            // SolamnicArmor was never sold (a quest reward -- see
            // docs/QUEST_NOTES.md's solamnic_armor), so it has no
            // established price, same reasoning as a starting weapon below.
            if (item.armorId == ArmorId::SolamnicArmor) {
                sellable = false;
                return 0;
            }
            sellable = true;
            return armorInfo(item.armorId).costStl / 2;
        case ItemKind::Shield:
            sellable = true;
            return kShieldCostStl / 2;
        case ItemKind::Weapon: {
            const WeaponUpgrade* upgrade = weaponUpgradeFor(character.charClass);
            if (upgrade != nullptr && item.weaponName == upgrade->name) {
                sellable = true;
                return upgrade->costStl / 2;
            }
            const MagicWeapon& magic = magicWeaponFor(character.charClass);
            if (item.weaponName == magic.name) {
                sellable = true;
                return magic.costStl / 2;
            }
            sellable = false;
            return 0;
        }
        case ItemKind::Potion:
            sellable = true;
            return kHealingPotionCostStl / 2;
        case ItemKind::Webnet:
            sellable = true;
            return kWebnetCostStl / 2;
        case ItemKind::BroochOfImog:
            sellable = true;
            return kBroochOfImogCostStl / 2;
        case ItemKind::QuestItem:
            // Never sold, never bought -- granted in the world for one
            // specific quest, same "no established price" treatment as
            // SolamnicArmor above.
            sellable = false;
            return 0;
    }
    sellable = false;
    return 0;
}

} // namespace

std::vector<SellItem> sellableItems(const Character& character) {
    std::vector<SellItem> items;
    for (const auto& invItem : character.inventory) {
        bool sellable = false;
        int value = resaleValueStl(character, invItem, sellable);
        items.push_back({inventoryItemLabel(invItem), value, sellable});
    }
    return items;
}

PurchaseResult sellItem(Character& character, int index) {
    if (index < 0 || index >= static_cast<int>(character.inventory.size())) {
        return {false, "Nothing to sell there."};
    }
    bool sellable = false;
    int value = resaleValueStl(character, character.inventory[index], sellable);
    if (!sellable) {
        return {false, "The shop won't buy that."};
    }

    std::string label = inventoryItemLabel(character.inventory[index]);
    character.inventory.erase(character.inventory.begin() + index);
    character.steelPieces += value;

    return {true, "Sold " + label + " for " + std::to_string(value) + " stl."};
}

int firstPotionIndex(const Character& character) {
    for (size_t i = 0; i < character.inventory.size(); ++i) {
        if (character.inventory[i].kind == ItemKind::Potion) return static_cast<int>(i);
    }
    return -1;
}

PurchaseResult drinkPotion(Character& character, int index) {
    if (index < 0 || index >= static_cast<int>(character.inventory.size()) ||
        character.inventory[index].kind != ItemKind::Potion) {
        return {false, "That's not a potion."};
    }
    character.inventory.erase(character.inventory.begin() + index);
    int healed = std::max(0, std::min(roll(kHealingPotionDiceCount, kHealingPotionDiceSides) + kHealingPotionFlatBonus,
                                       character.maxHp - character.currentHp));
    character.currentHp += healed;
    return {true, "You drink a Potion of Healing and recover " + std::to_string(healed) + " hit points."};
}

int firstWebnetIndex(const Character& character) {
    for (size_t i = 0; i < character.inventory.size(); ++i) {
        if (character.inventory[i].kind == ItemKind::Webnet) return static_cast<int>(i);
    }
    return -1;
}

int firstBroochIndex(const Character& character) {
    for (size_t i = 0; i < character.inventory.size(); ++i) {
        if (character.inventory[i].kind == ItemKind::BroochOfImog) return static_cast<int>(i);
    }
    return -1;
}

bool broochAvailableToday(const Character& character, long long today) {
    return firstBroochIndex(character) >= 0 && character.lastBroochUseDay != today;
}

PurchaseResult useWebnet(Character& character, int index) {
    if (index < 0 || index >= static_cast<int>(character.inventory.size()) ||
        character.inventory[index].kind != ItemKind::Webnet) {
        return {false, "That's not a webnet."};
    }
    character.inventory.erase(character.inventory.begin() + index);
    return {true, "You cast the Webnet -- it snares your foe in a net of entrapment!"};
}

PurchaseResult activateBrooch(Character& character, long long today) {
    if (!broochAvailableToday(character, today)) {
        return {false, "The Brooch of Imog's power is already spent for today."};
    }
    character.lastBroochUseDay = today;
    return {true, "You speak the Brooch of Imog's command word -- a minor globe of invulnerability surrounds you!"};
}

int findQuestItemIndex(const Character& character, const std::string& questItemId) {
    for (size_t i = 0; i < character.inventory.size(); ++i) {
        const InventoryItem& item = character.inventory[i];
        if (item.kind == ItemKind::QuestItem && item.questItemId == questItemId) return static_cast<int>(i);
    }
    return -1;
}

} // namespace character
