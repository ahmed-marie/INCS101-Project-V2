#include "BonusCard.h"

BonusCard::BonusCard(bool faceUp) : Card(7, faceUp)
{
	cardType = CardType::Bonus;
}

std::string BonusCard::getRevealMessage() const
{
	return "Bonus Card is revealed";
}