#include "MainWindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("Memory Match");
    resize(560, 720);

    stack = new QStackedWidget(this);
    startScreen = new StartScreen();
    gameScreen = new GameScreen();
    endScreen = new EndScreen();

    stack->addWidget(startScreen);
    stack->addWidget(gameScreen);
    stack->addWidget(endScreen);
    setCentralWidget(stack);

    connect(startScreen, &StartScreen::startRequested, this, &MainWindow::onStartRequested);
    connect(gameScreen, &GameScreen::gameOver, this, &MainWindow::onGameOver);
    connect(endScreen, &EndScreen::playAgainRequested, this, &MainWindow::onPlayAgainRequested);

    stack->setCurrentWidget(startScreen);
}

void MainWindow::onStartRequested(const QString& p1, const QString& p2, int startingPlayerIndex)
{
    p1Name = p1;
    p2Name = p2;

    // A fresh Game per session - the old one (if any) is destroyed
    // here, but GameScreen is never touched while startScreen is
    // showing, so there's no window where GameScreen could hold a
    // dangling pointer into it.
    game = std::make_unique<Game>();
    game->setPlayerNames(p1.toStdString(), p2.toStdString());
    game->startGame(startingPlayerIndex);

    gameScreen->bindGame(game.get());
    stack->setCurrentWidget(gameScreen);
}

void MainWindow::onGameOver()
{
    GameSnapshot snapshot = game->getSnapshot();
    int winnerIndex = game->getWinnerIndex();

    endScreen->setResult(p1Name, snapshot.players[0].score,
                          p2Name, snapshot.players[1].score,
                          winnerIndex);
    stack->setCurrentWidget(endScreen);
}

void MainWindow::onPlayAgainRequested()
{
    stack->setCurrentWidget(startScreen);
}
