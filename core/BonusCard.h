#pragma once
#include "Card.h"

// Note the constructor only takes faceUp: a BonusCard is always
// number 7 by definition, so there's no reason to let a caller
// pass in a different number and create an invalid bonus card.
// This is a small but deliberate change from the original design
// (which took `number` for every card type) - it's an example of
// making illegal states unrepresentable.
class BonusCard : public Card {
public:
    explicit BonusCard(bool faceUp);
    std::string getRevealMessage() const override;
};
