#pragma once
#include "Card.h"

// Same reasoning as BonusCard: number 8 is fixed internally.
class PenaltyCard : public Card {
public:
    explicit PenaltyCard(bool faceUp);
    std::string getRevealMessage() const override;
};
