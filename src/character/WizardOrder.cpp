#include "character/WizardOrder.h"

namespace character {

const char* robeColorName(RobeColor color) {
    switch (color) {
        case RobeColor::White: return "Wizard of the White Robes";
        case RobeColor::Red: return "Wizard of the Red Robes";
        case RobeColor::Black: return "Wizard of the Black Robes";
        case RobeColor::None: return "";
    }
    return "";
}

const char* robeMoonName(RobeColor color) {
    switch (color) {
        case RobeColor::White: return "Solinari";
        case RobeColor::Red: return "Lunitari";
        case RobeColor::Black: return "Nuitari";
        case RobeColor::None: return "";
    }
    return "";
}

RobeColor robeForAlignment(Alignment alignment) {
    switch (alignment) {
        case Alignment::LawfulGood:
        case Alignment::NeutralGood:
        case Alignment::ChaoticGood:
            return RobeColor::White;
        case Alignment::LawfulEvil:
        case Alignment::NeutralEvil:
        case Alignment::ChaoticEvil:
            return RobeColor::Black;
        default: // the three Neutral alignments
            return RobeColor::Red;
    }
}

} // namespace character
