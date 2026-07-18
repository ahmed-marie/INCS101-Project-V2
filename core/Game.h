#pragma once
#include "Deck.h"
#include "Player.h"
#include "GameSnapshot.h"
#include <array>
#include <string>

inline constexpr int PLAYER_NUMBERS = 2;

// GamePhase replaces the old GAME_STATE_T, but it now also
// describes *what input the caller owes the game next* - this
// is what lets a GUI know "show a bonus-choice dialog now" instead
// of the old design where that choice was answered via a blocking
// cin >> n buried inside evaluateTurn().
enum class GamePhase {
    NotStarted,
    AwaitingFirstCard,
    AwaitingSecondCard,
    AwaitingBonusChoice,     // two bonus cards revealed, waiting on onBonusChoice()
    AwaitingPenaltyChoice,   // two penalty cards revealed, waiting on onPenaltyChoice()
    GameOver
};

enum class TurnOutcome { Pending, BonusTurn, SkipTurn, EndTurn };

// This is the core rearchitecture. Compare each public method here
// to the old Game class:
//   - intializeGame() -> setPlayerNames() + startGame(), no cin
//   - requestCardFlip() -> onCardClicked(row, col), no cin, returns
//     immediately instead of looping until valid input arrives
//   - evaluateTurn()'s cin >> n for bonus/penalty choice ->
//     onBonusChoice()/onPenaltyChoice(), called separately, only
//     when getPhase() says the game is actually waiting on it
//   - loopGame() -> gone entirely. There is no loop. The caller
//     (GUI event loop, or a test driving calls directly) is now
//     in control of pacing, not Game.
//   - finalizeGame()'s cout -> getWinnerIndex(), caller decides
//     how/whether to display it
//
// Nothing in this class includes <iostream>. That's the whole point.
class Game {
public:
    Game();

    void setPlayerNames(const std::string& p1Name, const std::string& p2Name);
    void startGame(int startingPlayerIndex); // 0 or 1

    // --- Actions driven by the caller (GUI click handler, or a test) ---

    // Attempt to flip the card at (row, col), 1-indexed, range [1, GRID_SIZE].
    // Returns what happened so the caller can react (e.g. show
    // "already revealed" feedback) without Game printing anything.
    CardEvent onCardClicked(int row, int col);

    // Only valid when getPhase() == AwaitingBonusChoice.
    // choice: 1 = take 2 points and end turn, 2 = take 1 point and continue.
    TurnOutcome onBonusChoice(int choice);

    // Only valid when getPhase() == AwaitingPenaltyChoice.
    // choice: 1 = lose 2 points and end turn, 2 = lose 1 point and skip next turn.
    TurnOutcome onPenaltyChoice(int choice);

    // --- Read-only state for rendering and for tests ---

    GameSnapshot getSnapshot() const;
    GamePhase getPhase() const;

    // Meaningful only once getPhase() == GameOver. -1 means a draw.
    int getWinnerIndex() const;

private:
    void resolveRevealedPair();          // calls deck.evaluateFlippedCards(), updates score/phase
    void applyTurnOutcome(TurnOutcome outcome);
    void checkDeckStatusAndAdvance();     // handles ONE_CARD_LEFT / EMPTY_DECK
    void shiftTurn();

    std::array<Player, PLAYER_NUMBERS> players;
    Deck deck;
    GamePhase phase;
    int currentTurn;
    int nextTurn;
    std::string statusMessage;
};
