#pragma once
#include "Card.h"

class StandardCard : public Card {
public:
    StandardCard(int number, bool faceUp);
    std::string getRevealMessage() const override;
};
