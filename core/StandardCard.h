#pragma once
#include "Card.h"

class StandardCard : public Card {
public:
    StandardCard(int number, bool faceUp = false);
    std::string getRevealMessage() const override;
};
