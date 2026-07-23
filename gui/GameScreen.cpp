#include "GameScreen.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

GameScreen::GameScreen(QWidget* parent) : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    // --- score header ---
    auto* scoreRow = new QHBoxLayout();
    player1Label = new QLabel();
    player2Label = new QLabel();
    scoreRow->addWidget(player1Label);
    scoreRow->addStretch();
    scoreRow->addWidget(player2Label);
    mainLayout->addLayout(scoreRow);

    // --- status message ---
    statusLabel = new QLabel();
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("font-size: 15px; color: #555555; padding: 4px;");
    statusLabel->setWordWrap(true);
    mainLayout->addWidget(statusLabel);

    // --- card grid ---
    auto* gridWidget = new QWidget();
    auto* grid = new QGridLayout(gridWidget);
    grid->setSpacing(10);

    for (int row = 0; row < GRID_SIZE; ++row)
    {
        for (int col = 0; col < GRID_SIZE; ++col)
        {
            // Game's public API is 1-indexed
            auto* btn = new CardButton(row + 1, col + 1);
            connect(btn, &CardButton::cardClicked, this, &GameScreen::onCardButtonClicked);
            grid->addWidget(btn, row, col);
            cardButtons[row][col] = btn;
        }
    }

    mainLayout->addWidget(gridWidget, 0, Qt::AlignHCenter);
    setStyleSheet("background-color: #FFFDF6;");
}

void GameScreen::bindGame(Game* g)
{
    game = g;
    refresh();
}

void GameScreen::onCardButtonClicked(int row, int col)
{
    if (game == nullptr || inputLocked)
    {
        return;
    }

    game->onCardClicked(row, col);
    refresh();

    if (game->getPhase() == GamePhase::SecondCardRevealed)
    {
        // Both cards are visible now - give the player a moment to
        // actually see the second card before Game evaluates the
        // pair. The delay is entirely a GUI concern: Game has no
        // notion of wall-clock time, only the SecondCardRevealed
        // phase, so this is the one place that timing decision belongs.
        inputLocked = true;
        QTimer::singleShot(SECOND_CARD_DISPLAY_MS, this, &GameScreen::finalizeCurrentTurn);
        return;
    }

    if (game->getPhase() == GamePhase::GameOver)
    {
        emit gameOver();
    }
}

void GameScreen::finalizeCurrentTurn()
{
    if (game == nullptr)
    {
        return;
    }

    game->finalizeTurn();
    refresh();

    if (game->getPhase() == GamePhase::AwaitingBonusChoice)
    {
        promptBonusChoice();
        refresh();
    }
    else if (game->getPhase() == GamePhase::AwaitingPenaltyChoice)
    {
        promptPenaltyChoice();
        refresh();
    }

    inputLocked = false;

    if (game->getPhase() == GamePhase::GameOver)
    {
        emit gameOver();
    }
}

void GameScreen::promptBonusChoice()
{
    QMessageBox box(this);
    box.setWindowTitle("Bonus Cards!");
    box.setText("You revealed two Bonus cards! Choose your reward:");
    QPushButton* takeTwo = box.addButton("Take 2 points, end turn", QMessageBox::AcceptRole);
    QPushButton* takeOne = box.addButton("Take 1 point, go again", QMessageBox::ActionRole);
    box.exec();

    int choice = (box.clickedButton() == takeOne) ? 2 : 1;
    (void)takeTwo; // only referenced for symmetry/readability
    game->onBonusChoice(choice);
}

void GameScreen::promptPenaltyChoice()
{
    QMessageBox box(this);
    box.setWindowTitle("Penalty Cards!");
    box.setText("You revealed two Penalty cards! Choose your fate:");
    QPushButton* loseTwo = box.addButton("Lose 2 points, end turn", QMessageBox::AcceptRole);
    QPushButton* loseOne = box.addButton("Lose 1 point, skip next turn", QMessageBox::ActionRole);
    box.exec();

    int choice = (box.clickedButton() == loseOne) ? 2 : 1;
    (void)loseTwo; // only referenced for symmetry/readability
    game->onPenaltyChoice(choice);
}

void GameScreen::refresh()
{
    if (game == nullptr)
    {
        return;
    }

    GameSnapshot snapshot = game->getSnapshot();

    for (int row = 0; row < GRID_SIZE; ++row)
    {
        for (int col = 0; col < GRID_SIZE; ++col)
        {
            cardButtons[row][col]->updateView(snapshot.grid[row][col]);
        }
    }

    const bool p1Turn = (snapshot.currentPlayerIndex == 0);
    const QString labelStyleTemplate =
        "font-size: 16px; font-weight: bold; padding: 10px 18px;"
        "border-radius: 10px; background-color: %1;";

    player1Label->setText(QString("%1: %2 pts")
        .arg(QString::fromStdString(snapshot.players[0].name))
        .arg(snapshot.players[0].score));
    player1Label->setStyleSheet(labelStyleTemplate.arg(p1Turn ? "#FFE066" : "#F0F0FF"));

    player2Label->setText(QString("%1: %2 pts")
        .arg(QString::fromStdString(snapshot.players[1].name))
        .arg(snapshot.players[1].score));
    player2Label->setStyleSheet(labelStyleTemplate.arg(p1Turn ? "#F0F0FF" : "#FFE066"));

    statusLabel->setText(QString::fromStdString(snapshot.statusMessage));
}