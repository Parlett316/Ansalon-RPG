#include "character/Companion.h"
#include "character/Ability.h"
#include "character/CharClass.h"
#include "character/Equipment.h"
#include "character/Race.h"

#include <algorithm>

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

} // namespace character
