#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QButtonGroup>

// First screen shown. Purely a form - it knows nothing about Game;
// it just collects two names and a starting-player choice and hands
// them to MainWindow via a signal, which is the one place a Game
// actually gets constructed.
class StartScreen : public QWidget
{
    Q_OBJECT

public:
    explicit StartScreen(QWidget* parent = nullptr);

signals:
    // startingPlayerIndex is 0 or 1, matching Game::startGame()'s contract.
    void startRequested(const QString& p1Name, const QString& p2Name, int startingPlayerIndex);

private slots:
    void onStartClicked();

private:
    QLineEdit* p1Edit;
    QLineEdit* p2Edit;
    QButtonGroup* startingPlayerGroup;
};
