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
constexpr std::array<ArmorInfo, 4> kArmorTable = {{
    {ArmorId::None, "No Armor", 10, 0},
    {ArmorId::Leather, "Leather Armor", 8, 5},
    {ArmorId::ChainMail, "Chain Mail", 5, 75},
    {ArmorId::SplintMail, "Splint Mail", 4, 80},
}};

// Table 44 (Weapons, p.94), visually confirmed: Two-Handed Sword 1d10/50stl
// (no flat bonus), Footman's Flail 1d6+1/15stl, Long Sword 1d8/15stl (no
// flat bonus). Mage/Tinker deliberately have no entry -- see Equipment.h.
const WeaponUpgrade kFighterUpgrade{"Two-Handed Sword", 10, 0, 50};
const WeaponUpgrade kClericUpgrade{"Footman's Flail", 6, 1, 15};
const WeaponUpgrade kThiefUpgrade{"Long Sword", 8, 0, 15};

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
        case ItemKind::Weapon:
            return item.weaponName + " (1d" + std::to_string(item.weaponDamageSides) +
                   (item.weaponDamageBonus > 0 ? "+" + std::to_string(item.weaponDamageBonus) : "") + ")";
        case ItemKind::Potion:
            return "Potion of Healing (2d4+2 hp)";
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
                               character.weaponDamageSides, character.weaponDamageBonus};
            character.weaponName = item.weaponName;
            character.weaponDamageSides = item.weaponDamageSides;
            character.weaponDamageBonus = item.weaponDamageBonus;
            character.inventory.push_back(std::move(old));
            break;
        }
        case ItemKind::Potion:
            // Not equippable -- GameLoop::handleInventory branches on
            // ItemKind before ever calling here for a Potion; drinking is
            // a different action (see drinkPotion below), unreachable here
            // in practice.
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
    bool isPotion = false;
    if (index < armorCount) {
        character.inventory.push_back(InventoryItem{ItemKind::Armor, kBuyableArmor[index], "", 0, 0});
    } else if (index == armorCount) {
        character.inventory.push_back(InventoryItem{ItemKind::Shield, ArmorId::None, "", 0, 0});
    } else if (index == weaponIndex) {
        character.inventory.push_back(
            InventoryItem{ItemKind::Weapon, ArmorId::None, upgrade->name, upgrade->damageSides, upgrade->damageBonus});
    } else {
        // The one remaining catalog line -- the Potion of Healing, whether
        // or not this class had a weapon-upgrade entry ahead of it.
        character.inventory.push_back(InventoryItem{ItemKind::Potion, ArmorId::None, "", 0, 0});
        isPotion = true;
    }

    return {true, "Bought " + item.label +
                       (isPotion ? ". Press 'i' to drink it." : ". Press 'i' to equip it.")};
}

namespace {

// See SellItem/sellableItems in Equipment.h for why this is an invented,
// flagged convention rather than a sourced number.
int resaleValueStl(const Character& character, const InventoryItem& item, bool& sellable) {
    switch (item.kind) {
        case ItemKind::Armor:
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
            sellable = false;
            return 0;
        }
        case ItemKind::Potion:
            sellable = true;
            return kHealingPotionCostStl / 2;
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

} // namespace character
