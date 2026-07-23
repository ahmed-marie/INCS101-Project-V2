#pragma once
#include <QWidget>
#include <QLabel>
#include <array>
#include "Game.h"
#include "CardButton.h"

// The main board. Holds a non-owning pointer to the Game that
// MainWindow owns for the current session - GameScreen never
// constructs or destroys a Game itself. Every click is translated
// into exactly one call on Game's public API, followed by a full
// re-render from a fresh getSnapshot() - GameScreen never guesses
// at state, it always re-reads it.
class GameScreen : public QWidget
{
    Q_OBJECT

public:
    explicit GameScreen(QWidget* parent = nullptr);

    // Points this screen at a (freshly started) Game and redraws
    // the board to match its current state from scratch.
    void bindGame(Game* game);

signals:
    void gameOver();

private slots:
    void onCardButtonClicked(int row, int col);

    // Runs after the display-delay timer fires, once the player has
    // had a chance to actually see the second card.
    void finalizeCurrentTurn();

private:
    Game* game = nullptr;
    std::array<std::array<CardButton*, GRID_SIZE>, GRID_SIZE> cardButtons;

    QLabel* player1Label;
    QLabel* player2Label;
    QLabel* statusLabel;

    // True while the second card is being shown and the pause timer
    // is running - blocks further clicks until finalizeCurrentTurn()
    // completes. This is a GUI-only concern; Game itself now guards
    // the same window independently (see Game::onCardClicked()), so
    // this is belt-and-suspenders, not the only line of defense.
    bool inputLocked = false;

    static constexpr int SECOND_CARD_DISPLAY_MS = 3000;

    void refresh();
    void promptBonusChoice();
    void promptPenaltyChoice();
};