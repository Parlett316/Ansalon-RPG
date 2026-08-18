#include "combat/Monster.h"
#include "character/Dice.h"

#include <stdexcept>

namespace combat {

void MonsterCatalog::addMonster(Monster monster) {
    monsters_.push_back(std::move(monster));
}

const Monster& MonsterCatalog::randomMonster() const {
    if (monsters_.empty()) {
        throw std::runtime_error("MonsterCatalog::randomMonster called on an empty catalog");
    }
    int index = character::roll(1, static_cast<int>(monsters_.size())) - 1;
    return monsters_[static_cast<size_t>(index)];
}

} // namespace combat
