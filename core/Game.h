#pragma once
#include "Deck.h"
#include "Player.h"
#include "GameSnapshot.h"
#include <array>
#include <string>

inline constexpr int PLAYER_NUMBERS = 2;

enum class GamePhase {
    NotStarted,
    AwaitingFirstCard,
    AwaitingSecondCard,
    SecondCardRevealed,      // both cards face-up, not yet evaluated -
    // waiting on finalizeTurn(). Exists so the
    // caller gets a chance to render the second
    // card before Game evaluates the pair; Game
    // itself has no concept of "wait" or timing,
    // only this phase.
    AwaitingBonusChoice,     // two bonus cards revealed, waiting on onBonusChoice()
    AwaitingPenaltyChoice,   // two penalty cards revealed, waiting on onPenaltyChoice()
    GameOver
};

// Pending is not a real outcome - it exists purely as a safe default
// value to assign local TurnOutcome variables to before the real
// value is computed (the same defensive pattern used to fix the
// uninitialized RevealedCardsEvent bug in Deck::evaluateFlippedCards).
// It should never be the *final* value passed to applyTurnOutcome().
enum class TurnOutcome { Pending, BonusTurn, SkipTurn, EndTurn };

class Game {
public:
    Game();
    Game(Deck presetDeck);

    void setPlayerNames(const std::string& p1Name, const std::string& p2Name);
    void startGame(int startingPlayerIndex); // 0 or 1

    // --- Actions driven by the caller (GUI click handler, or a test) ---

    // Attempt to flip the card at (row, col), 1-indexed, range [1, GRID_SIZE].
    // Returns what happened to THIS card only (found / already revealed /
    // out of range) - it does not evaluate the pair, even when this is
    // the second card of the turn. When it completes a pair, check
    // getPhase(): SecondCardRevealed means both cards are now face-up
    // and the caller owes Game a call to finalizeTurn() next (typically
    // after a short delay, so the player actually sees the second card).
    // Only valid while getPhase() is AwaitingFirstCard or
    // AwaitingSecondCard - any other phase returns NotFound without
    // touching the deck, since a pair is already pending evaluation.
    CardEvent onCardClicked(int row, int col);

    // Only valid when getPhase() == SecondCardRevealed. Evaluates the
    // revealed pair and, for the 5 immediate outcomes, fully resolves
    // the turn (score, turn-credits, deck-empty check). For TwoBonus/
    // TwoPenalty, instead leaves phase at AwaitingBonusChoice/
    // AwaitingPenaltyChoice and returns TurnOutcome::Pending - the
    // caller still owes onBonusChoice()/onPenaltyChoice() next.
    TurnOutcome finalizeTurn();

    // Only valid when getPhase() == AwaitingBonusChoice.
    // choice: 1 = take 2 points and end turn, 2 = take 1 point and continue.
    // Returns the resulting TurnOutcome so a caller (or a test) can
    // assert on it directly, without needing a second call.
    TurnOutcome onBonusChoice(int choice);

    // Only valid when getPhase() == AwaitingPenaltyChoice.
    // choice: 1 = lose 2 points and end turn, 2 = lose 1 point and skip next turn.
    TurnOutcome onPenaltyChoice(int choice);

    // --- Read-only state for rendering and for tests ---

    // Builds a fresh, disposable snapshot of current state, including
    // whatever statusMessage was last set by an internal method (e.g.
    // "Bonus turn! Alice goes again."). Game's members are the live
    // source of truth; the returned struct is just a point-in-time copy.
    GameSnapshot getSnapshot() const;
    GamePhase getPhase() const;

    // Meaningful only once getPhase() == GameOver. -1 means a draw.
    int getWinnerIndex() const;

private:
    // Called once the second card of a turn is flipped (from
    // onCardClicked()). Calls deck.evaluateFlippedCards(), and for the
    // 5 outcomes that don't require a choice, applies the score delta
    // directly (score depends on which RevealedCardsEvent occurred,
    // which applyTurnOutcome() below has no visibility into) and
    // returns the resulting TurnOutcome. For TwoBonus/TwoPenalty,
    // instead sets phase to AwaitingBonusChoice/AwaitingPenaltyChoice
    // and returns TurnOutcome::Pending - the real outcome isn't known
    // yet, it's waiting on onBonusChoice()/onPenaltyChoice().
    TurnOutcome resolveRevealedPair();

    // Applies turnsNo adjustments and turn-passing logic (including
    // the shift-to-next-player mechanics that used to live in a
    // separate shiftTurn() method) for an already-resolved outcome.
    // Score has already been applied by the caller before this runs -
    // this method only ever touches turn credits/ownership, never score.
    // Called by onCardClicked() (5 immediate cases) and by
    // onBonusChoice()/onPenaltyChoice() (2 choice-driven cases) -
    // never called with TurnOutcome::Pending.
    void applyTurnOutcome(TurnOutcome outcome);

    // Checks deck.getDeckStatus() after a turn resolves; auto-reveals
    // the last remaining card and awards its points if exactly one is
    // left, and sets phase = GameOver once the deck is empty. This can
    // override whatever applyTurnOutcome() just decided about the next
    // phase, so it always runs after applyTurnOutcome(), never before.
    void checkDeckStatusAndAdvance();

    std::array<Player, PLAYER_NUMBERS> players;
    Deck deck;
    GamePhase phase;
    int currentTurn;
    int nextTurn;
    std::string statusMessage;
    TurnOutcome turnOutcome;
};