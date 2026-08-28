#include "character/Companion.h"
#include "character/Ability.h"
#include "character/CharClass.h"
#include "character/Equipment.h"
#include "character/Race.h"

#include <algorithm>
#include <stdexcept>

namespace character {

Character buildCompanion() {
    Character character;
    character.name = "Bren Alder";
    character.race = RaceId::Human;
    character.subrace = SubraceId::None;
    character.charClass = ClassId::Fighter;
    character.alignment = Alignment::NeutralGood;

    // Fixed, hand-picked ability scores -- deliberately NOT character::roll,
    // so buildCompanion() is pure and game::SaveGame can reconstruct an
    // identical companion on load. See Companion.h.
    character.scores = AbilityScores{15, 13, 14, 10, 10, 12};

    const ClassInfo& fighter = classInfo(ClassId::Fighter);
    character.level = 1;
    character.experience = 0;
    character.maxHp = std::max(1, fighter.hitDieSides +
                                       hpAdjustmentForConstitution(character.scores.constitution, /*isWarrior=*/true));
    character.currentHp = character.maxHp;
    character.weaponName = fighter.weaponName;
    character.weaponDamageSides = fighter.weaponDamageSides;
    character.weaponDamageBonus = 0;
    recomputeArmorClass(character); // equippedArmor is still None -- just the Dex adjustment
    character.thac0 = 20; // every class starts at THAC0 20, same convention as CharacterCreator

    character.saves = fighter.level1Saves;
    applyRacialSavingThrowBonus(character.race, character.scores.constitution, character.saves);

    // Fixed, not rolled -- see the "not character::roll" note above.
    character.steelPieces = 50;

    return character;
}

Character buildDessaCorrin() {
    Character character;
    character.name = "Dessa Corrin";
    character.race = RaceId::Human;
    character.subrace = SubraceId::None;
    character.charClass = ClassId::Thief;
    character.alignment = Alignment::ChaoticGood;

    // Fixed, hand-picked ability scores -- same "not character::roll"
    // reasoning as buildCompanion() above. DEX 16 comfortably clears the
    // Thief's PHB p.54 prime requisite minimum (9).
    character.scores = AbilityScores{10, 16, 12, 12, 10, 13};

    const ClassInfo& thief = classInfo(ClassId::Thief);
    character.level = 1;
    character.experience = 0;
    character.maxHp = std::max(1, thief.hitDieSides +
                                       hpAdjustmentForConstitution(character.scores.constitution, /*isWarrior=*/false));
    character.currentHp = character.maxHp;
    character.weaponName = thief.weaponName;
    character.weaponDamageSides = thief.weaponDamageSides;
    character.weaponDamageBonus = 0;
    recomputeArmorClass(character); // equippedArmor is still None -- just the Dex adjustment
    character.thac0 = 20; // every class starts at THAC0 20, same convention as CharacterCreator

    character.saves = thief.level1Saves;
    applyRacialSavingThrowBonus(character.race, character.scores.constitution, character.saves);

    // Fixed, not rolled -- see the "not character::roll" note above.
    character.steelPieces = 35;

    return character;
}

Character buildCompanionById(const std::string& id) {
    if (id == "bren_alder") return buildCompanion();
    if (id == "dessa_corrin") return buildDessaCorrin();
    throw std::runtime_error("character::buildCompanionById: unknown companion id '" + id + "'");
}

bool isKnownCompanionId(const std::string& id) {
    return id == "bren_alder" || id == "dessa_corrin";
}

} // namespace character
