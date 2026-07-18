#pragma once
#include "Card.h"
#include "Deck.h" // for GRID_SIZE
#include <array>
#include <string>

// This struct doesn't exist in your original design - it's new,
// and it's the single most important piece of this rearchitecture.
// It is the ONLY thing a GUI (or a test) needs to read to know
// the complete visible state of the game. Nothing in core/ ever
// prints; everything a caller needs to render or assert comes
// from here. Qt's paint/update code reads this struct and draws
// widgets; GoogleTest reads this same struct and calls EXPECT_EQ.
struct CardView {
    bool exists = false;   // false = removed from the grid entirely
    bool faceUp = false;
    int number = 0;         // meaningful only if faceUp
    CardType type = CardType::Standard;
};

struct PlayerView {
    std::string name;
    int score = 0;
    int turnsRemaining = 0;
};

struct GameSnapshot {
    std::array<std::array<CardView, GRID_SIZE>, GRID_SIZE> grid;
    std::array<PlayerView, 2> players;
    int currentPlayerIndex = 0;

    // Human-readable feedback for the last action, e.g.
    // "Bonus Card is revealed" or "That card is already revealed."
    // The GUI can show this as status text; a test can assert on
    // it too, but it's presentation-adjacent, not logic.
    std::string statusMessage;
};
