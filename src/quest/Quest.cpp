#include "quest/Quest.h"

namespace quest {

void QuestCatalog::addQuest(Quest quest) {
    quests_.push_back(std::move(quest));
}

const Quest* QuestCatalog::find(const std::string& id) const {
    for (const auto& q : quests_) {
        if (q.id == id) return &q;
    }
    return nullptr;
}

} // namespace quest
