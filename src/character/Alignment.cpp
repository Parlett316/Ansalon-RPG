#include "character/Alignment.h"

namespace character {

const char* alignmentName(Alignment a) {
    switch (a) {
        case Alignment::LawfulGood: return "Lawful Good";
        case Alignment::NeutralGood: return "Neutral Good";
        case Alignment::ChaoticGood: return "Chaotic Good";
        case Alignment::LawfulNeutral: return "Lawful Neutral";
        case Alignment::TrueNeutral: return "True Neutral";
        case Alignment::ChaoticNeutral: return "Chaotic Neutral";
        case Alignment::LawfulEvil: return "Lawful Evil";
        case Alignment::NeutralEvil: return "Neutral Evil";
        case Alignment::ChaoticEvil: return "Chaotic Evil";
    }
    return "";
}

} // namespace character
