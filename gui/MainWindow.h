#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <memory>
#include "Game.h"
#include "StartScreen.h"
#include "GameScreen.h"
#include "EndScreen.h"

// The one place in the whole GUI that constructs a Game. Everything
// else (StartScreen, GameScreen, EndScreen) only ever receives data
// from it or a pointer into it - none of them own game state.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onStartRequested(const QString& p1Name, const QString& p2Name, int startingPlayerIndex);
    void onGameOver();
    void onPlayAgainRequested();

private:
    QStackedWidget* stack;
    StartScreen* startScreen;
    GameScreen* gameScreen;
    EndScreen* endScreen;

    std::unique_ptr<Game> game;

    QString p1Name;
    QString p2Name;
};
