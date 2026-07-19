#include "StandardCard.h"


StandardCard::StandardCard(int number, bool faceUp) : Card(number, faceUp)
{
	cardType = CardType::Standard;
};


std::string StandardCard::getRevealMessage() const
{
	return "Standard Card " + std::to_string(number) + " is revealed";
}
