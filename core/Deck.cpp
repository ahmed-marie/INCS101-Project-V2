#include "Deck.h"


// Normal play: builds a fresh 16-card deck.
Deck::Deck()
{
	// Initialize the cardsArr with the different types of cards
	// 1. intialize the first 12 cards with standard cards
	int size = NUM_STANDARD_PAIRS;

	// arrayIndex represent the index in the array, counterIndex represents the card number
	int arrayIndex = 0;
	for (int counterIndex = 1; counterIndex <= size; counterIndex++)
	{
		// 1. initialize the first card in the pair
		//cardsArr[arrayIndex++] = new StandardCard(counterIndex);
		cardsArr[arrayIndex++] = std::make_unique<StandardCard>(counterIndex);
		// 2. initialize the second card in the pair
		//cardsArr[arrayIndex++] = new StandardCard(counterIndex);
		cardsArr[arrayIndex++] = std::make_unique<StandardCard>(counterIndex);
	}

	// 2. intialize the next two cards with bonus cards
	for (int i = 0; i < NUM_BONUS_PAIRS; i++)
	{
		// 1. initialize the first card in the pair
		//cardsArr[arrayIndex++] = new BonusCard(counterIndex);
		cardsArr[arrayIndex++] = std::make_unique<BonusCard>();
		// 2. initialize the second card in the pair
		//cardsArr[arrayIndex++] = new BonusCard(counterIndex, false);
		cardsArr[arrayIndex++] = std::make_unique<BonusCard>(false);
	}
	// 3. intialize the next two cards with penalty cards
	for (int i = 0; i < NUM_PENALTY_PAIRS; i++)
	{
		// 1. initialize the first card in the pair
		//cardsArr[arrayIndex++] = new PenaltyCard(counterIndex);
	    cardsArr[arrayIndex++] = std::make_unique<PenaltyCard>();
		// 2. initialize the second card in the pair
		//cardsArr[arrayIndex++] = new PenaltyCard(counterIndex, false);
		cardsArr[arrayIndex++] = std::make_unique<PenaltyCard>(false);
	}
	// initialized the number of removed cards to 0
	removedCards = 0;

	// initialize the revealedCardsIndex Array elements to -1
	revealedCardsIndex[0] = -1;
	revealedCardsIndex[1] = -1;
}

// Test-only path: inject a known, non-shuffled layout for testability.

Deck::Deck(std::array<std::unique_ptr<Card>, DECK_SIZE> presetCards)
{
	this->cardsArr = std::move(presetCards);

	// initialized the number of removed cards to 0
	this->removedCards = 0;

	// initialize the revealedCardsIndex Array elements to -1
	revealedCardsIndex[0] = -1;
	revealedCardsIndex[1] = -1;
}

void Deck::shuffle()
{
	srand(static_cast<unsigned>(time(0)));
	for (int i = 0; i < DECK_SIZE; ++i) {
		int j = rand() % DECK_SIZE; // Generate a random index
		swap(cardsArr[i], cardsArr[j]); // Swap the cards at indices i and j
	}
}

bool Deck::isCardExists(int index) const
{
	return cardsArr[index] != nullptr;
}

DeckStatus Deck::getDeckStatus() const
{
	if (removedCards == DECK_SIZE)
	{
		return DeckStatus::Empty;
	}
	else if (removedCards == (DECK_SIZE - 1))
	{
		return DeckStatus::OneCardLeft;
	}
	else
	{
		return DeckStatus::TwoOrMoreLeft; 
	}
}

CardEvent Deck::revealCard(int row, int col)
{
	// converts row and col to index. 
	// row and col have values between 1 and GRID_SIZE inclusive
	int index = (row - 1) * GRID_SIZE + col - 1; // i * j - 1
	return revealCard(index);
}

CardEvent Deck::revealCard(int index)
{
	if (isCardExists(index) == false)
	{
		return CardEvent::NotFound;
	}
	else if (cardsArr[index]->isFaceUp() == true)
	{
		return CardEvent::RevealedBefore;
	}
	else
	{
		// Flip the card
		cardsArr[index]->setFaceUp(true);
		// add its index in the reavealedCardIndex
		// 0 or 1 whether 0 is free
		if (revealedCardsIndex[0] == -1)
		{
			revealedCardsIndex[0] = index;
		}
		else
		{
			revealedCardsIndex[1] = index;
		}
		
		// display the card's message (this should be replaced with the 
		//cardsArr[index]->displayCardMessage();

		return CardEvent::Found; 
	}
}

RevealedCardsEvent Deck::evaluateFlippedCards()
{
	RevealedCardsEvent status = RevealedCardsEvent::Invalid;
	// get the card pointers
	std::unique_ptr<Card>& card1_ptr = cardsArr[revealedCardsIndex[0]];
	std::unique_ptr<Card>& card2_ptr = cardsArr[revealedCardsIndex[1]];
	// get their corresponding type
	CardType card1_type = card1_ptr->getCardType();
	CardType card2_type = card2_ptr->getCardType();

	// Determine the card's status 
	// (numbered according to cases in Notion)

	// two standard cards: 1. same 7. different
	if ((card1_type == CardType::Standard) && (card2_type == CardType::Standard))
	{
		if (card1_ptr->getNumber() == card2_ptr->getNumber())
		{
			status = RevealedCardsEvent::TwoSameStandard;
		}
		else
		{
			status = RevealedCardsEvent::TwoDifferentStandard;
		}
	}
	// 2. Two penalty cards
	else if ((card1_type == CardType::Penalty) && (card2_type == CardType::Penalty))
	{
		status = RevealedCardsEvent::TwoPenalty;
	}
	// 3. Two bonus cards
	else if ((card1_type == CardType::Bonus) && (card2_type == CardType::Bonus))
	{
		status = RevealedCardsEvent::TwoBonus;
	}
	// 4. A Bonus card and a Penalty Card
	else if (((card1_type == CardType::Bonus) && (card2_type == CardType::Penalty)) ||
		((card1_type == CardType::Penalty) && (card2_type == CardType::Bonus)))
	{
		status = RevealedCardsEvent::BonusAndPenalty;
	}
	// 5. A Standard card and a Bonus Card
	else if (((card1_type == CardType::Standard) && (card2_type == CardType::Bonus)) ||
		((card1_type == CardType::Bonus) && (card2_type == CardType::Standard)))
	{
		status = RevealedCardsEvent::StandardAndBonus;
	}
	// 6. A Standard card and a Penalty Card
	else if (((card1_type == CardType::Standard) && (card2_type == CardType::Penalty)) ||
		((card1_type == CardType::Penalty) && (card2_type == CardType::Standard)))
	{
		status = RevealedCardsEvent::StandardAndPenalty;
	}
	else
	{
		// non existing case
	}

	// manipulating deck and cards based on their type
	switch (status)
	{
		// cases 1. to 4.
	case RevealedCardsEvent::TwoSameStandard:
	case RevealedCardsEvent::TwoPenalty:
	case RevealedCardsEvent::TwoBonus:
	case RevealedCardsEvent::BonusAndPenalty:
		// delete both cards
		card1_ptr.reset(nullptr);
		card2_ptr.reset(nullptr);

		// increment removedCards by 2
		removedCards += 2;
		break;

	// case 5. and 6.
	case RevealedCardsEvent::StandardAndBonus:
	case RevealedCardsEvent::StandardAndPenalty:
		if (card1_type == CardType::Standard)
		{
			// flip the standard card
			cardsArr[revealedCardsIndex[0]]->setFaceUp(false);
			// delete the bonus/penalty card
			card2_ptr.reset(nullptr);
		}
		else
		{
			// flip the standard card
			cardsArr[revealedCardsIndex[1]]->setFaceUp(false);
			// delete the bonus/penalty card
			card1_ptr.reset(nullptr);
			cardsArr[revealedCardsIndex[0]] = nullptr;
		}
		// increment removedCards by 1
		removedCards++;
		break;

	case RevealedCardsEvent::TwoDifferentStandard:
		// flip the standard cards
		cardsArr[revealedCardsIndex[0]]->setFaceUp(false);
		cardsArr[revealedCardsIndex[1]]->setFaceUp(false);
		break;

	default: break;
	}

	// set the card indices in revealedCardsIndex to -1
	revealedCardsIndex[0] = -1;
	revealedCardsIndex[1] = -1;

	// return the status
	return status;
}

CardType Deck::revealLastCard()
{
	// searching for the last unflipped card in the deck
	for (int i = 0; i < DECK_SIZE; i++)
	{
		if (cardsArr[i] != nullptr)
		{
			cardsArr[i]->setFaceUp(true);
			//cardsArr[i]->displayCardMessage();
			return cardsArr[i]->getCardType();
		}
	}
	return CardType::Invalid;
}

// Read-only accessors so a GUI (or GameSnapshot builder) can
// render the grid without Deck knowing GUI/console exists.
// Returns nullptr if the slot is empty (card removed).
const Card* Deck::getCardAt(int index) const
{
	return cardsArr[index].get();
}

int Deck::getRemovedCards() const
{
	return removedCards;
}