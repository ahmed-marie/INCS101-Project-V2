#pragma once
#include <QPushButton>
#include "GameSnapshot.h" // for CardView, CardType

// One clickable cell in the 4x4 grid. Owns no game logic at all - it
// only knows how to render itself given a CardView (read-only data
// from Game::getSnapshot()) and how to announce that it was clicked,
// identified by its (row, col). GameScreen is the Controller that
// decides what a click means.
class CardButton : public QPushButton
{
    Q_OBJECT

public:
    CardButton(int row, int col, QWidget* parent = nullptr);

    int getRow() const { return row; }
    int getCol() const { return col; }

    // Updates this button's appearance to match the given card state.
    // Safe to call every refresh - it fully re-derives text/style/
    // enabled-state from view each time, no incremental state kept.
    void updateView(const CardView& view);

signals:
    // row/col are 1-indexed, matching Game::onCardClicked()'s contract.
    void cardClicked(int row, int col);

private:
    int row;
    int col;

    void applyFaceDownStyle();
    void applyFaceUpStyle(const CardView& view);
    void applyEmptySlotStyle();
};
