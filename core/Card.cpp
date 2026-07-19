#include "Card.h"

Card::Card(int number, bool faceUp)
{
	this->number = number;
	this->faceUp = faceUp;
	cardType = CardType::Invalid;
}


void Card::setNumber(int number)
{
	this->number = number;
}


int Card::getNumber() const
{
	return number;
}


void Card::setFaceUp(bool faceUp)
{
	this->faceUp = faceUp;
}


bool Card::isFaceUp() const
{
	return faceUp;
}

CardType Card::getCardType() const
{
	return cardType;
}
