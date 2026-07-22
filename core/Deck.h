#pragma once
#include "Card.h"
#include "StandardCard.h"
#include "BonusCard.h"
#include "PenaltyCard.h"
#include <array>
#include <memory>

inline constexpr int GRID_SIZE = 4;
inline constexpr int NUM_STANDARD_PAIRS = 6;
inline constexpr int NUM_BONUS_PAIRS = 1;
inline constexpr int NUM_PENALTY_PAIRS = 1;
inline constexpr int DECK_SIZE =
    2 * (NUM_STANDARD_PAIRS + NUM_BONUS_PAIRS + NUM_PENALTY_PAIRS);

enum class CardEvent {NotFound, RevealedBefore, Found };

enum class RevealedCardsEvent {
    Invalid, TwoSameStandard, TwoDifferentStandard,
    TwoPenalty, TwoBonus,
    StandardAndBonus, StandardAndPenalty, BonusAndPenalty
};

enum class DeckStatus { Empty, OneCardLeft, TwoOrMoreLeft };

class Deck {
public:
    // Normal play: builds a fresh 16-card deck.
    Deck();

    // Test-only path: inject a known, non-shuffled layout so
    // GoogleTest can set up "two bonus cards at index 3 and 7"
    // deterministically instead of fighting with shuffle().
    // This single constructor is what makes evaluateFlippedCards()
    // unit-testable without any randomness or mocking framework.
    explicit Deck(std::array<std::unique_ptr<Card>, DECK_SIZE> presetCards);

    void shuffle();

    bool isCardExists(int index) const;
    DeckStatus getDeckStatus() const;

    CardEvent revealCard(int row, int col);
    CardEvent revealCard(int index);

    RevealedCardsEvent evaluateFlippedCards();
    CardType revealLastCard();

    // Read-only accessors so a GUI (or GameSnapshot builder) can
    // render the grid without Deck knowing GUI/console exists.
    // Returns nullptr if the slot is empty (card removed).
    const Card* getCardAt(int index) const;
    int getRemovedCards() const;

private:
    std::array<std::unique_ptr<Card>, DECK_SIZE> cardsArr;
    int removedCards;
    std::array<int, 2> revealedCardsIndex;
};
