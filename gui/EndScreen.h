#pragma once
#include <QWidget>
#include <QLabel>

class EndScreen : public QWidget
{
    Q_OBJECT

public:
    explicit EndScreen(QWidget* parent = nullptr);

    // winnerIndex: 0 or 1, or -1 for a draw - matches Game::getWinnerIndex()'s contract.
    void setResult(const QString& p1Name, int p1Score,
                   const QString& p2Name, int p2Score,
                   int winnerIndex);

signals:
    void playAgainRequested();

private:
    QLabel* headline;
    QLabel* scoresLabel;
};
