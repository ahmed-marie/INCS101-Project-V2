#include "EndScreen.h"
#include <QVBoxLayout>
#include <QPushButton>

EndScreen::EndScreen(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(60, 60, 60, 60);
    layout->setSpacing(20);
    layout->setAlignment(Qt::AlignCenter);

    headline = new QLabel();
    headline->setAlignment(Qt::AlignCenter);
    headline->setStyleSheet("font-size: 28px; font-weight: 900; color: #6C63FF;");
    layout->addWidget(headline);

    scoresLabel = new QLabel();
    scoresLabel->setAlignment(Qt::AlignCenter);
    scoresLabel->setStyleSheet("font-size: 16px; color: #555555;");
    layout->addWidget(scoresLabel);

    auto* again = new QPushButton("Play Again");
    again->setCursor(Qt::PointingHandCursor);
    again->setStyleSheet(
        "QPushButton {"
        "  background-color: #6C63FF; color: white; font-size: 16px; font-weight: bold;"
        "  padding: 14px 28px; border-radius: 12px; border: none;"
        "}"
        "QPushButton:hover { background-color: #7D75FF; }"
        "QPushButton:pressed { background-color: #5A52E0; }"
    );
    connect(again, &QPushButton::clicked, this, &EndScreen::playAgainRequested);
    layout->addWidget(again, 0, Qt::AlignCenter);

    setStyleSheet("background-color: #FFFDF6;");
}

void EndScreen::setResult(const QString& p1Name, int p1Score,
                           const QString& p2Name, int p2Score,
                           int winnerIndex)
{
    if (winnerIndex == -1)
    {
        headline->setText("It's a draw!");
    }
    else
    {
        const QString winnerName = (winnerIndex == 0) ? p1Name : p2Name;
        headline->setText(QString("%1 wins!").arg(winnerName));
    }

    scoresLabel->setText(QString("%1: %2 pts     %3: %4 pts")
        .arg(p1Name).arg(p1Score).arg(p2Name).arg(p2Score));
}
