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

    // Runs after a second display-delay timer, once the player has
    // had a chance to see the just-completed turn's result, before
    // Game flips the final remaining card face-up.
    void revealFinalCardAndFinish();

    // Runs after a third display-delay timer, once the player has had
    // a chance to actually see the revealed final card, before Game
    // scores it, removes it, and ends the game.
    void evaluateFinalCardAndFinish();

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

    static constexpr int SECOND_CARD_DISPLAY_MS = 1500;
    static constexpr int FINAL_CARD_DISPLAY_MS = 1500;
    static constexpr int LAST_CARD_EVALUATE_DELAY_MS = 1500;
    static constexpr int GAME_OVER_TRANSITION_MS = 1000;

    void refresh();
    void promptBonusChoice();
    void promptPenaltyChoice();

    // Checks getPhase() == GameOver and, if so, emits gameOver() after
    // a short delay rather than immediately. This exists because
    // gameOver() is direct-connected to MainWindow::onGameOver(),
    // which synchronously swaps this screen out - emitting it right
    // after refresh() would swap the board away before Qt's event
    // loop ever gets a chance to actually paint refresh()'s changes
    // (setText/setStyleSheet only *schedule* a repaint, they don't
    // force one). The delay guarantees a repaint happens first, so
    // the player actually sees the final board state before the
    // screen changes.
    void finishIfGameOver();
};