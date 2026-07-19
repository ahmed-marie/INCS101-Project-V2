#include "PenaltyCard.h"

PenaltyCard::PenaltyCard(bool faceUp) : Card(8, faceUp)
{
	cardType = CardType::Penalty;
}

std::string PenaltyCard::getRevealMessage() const
{
	return "Penalty Card is revealed";
}