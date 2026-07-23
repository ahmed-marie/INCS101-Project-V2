#include "StartScreen.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>

StartScreen::StartScreen(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(60, 60, 60, 60);
    layout->setSpacing(18);
    layout->setAlignment(Qt::AlignTop);

    auto* title = new QLabel("Memory Match");
    title->setStyleSheet("font-size: 34px; font-weight: 900; color: #6C63FF;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto* subtitle = new QLabel("Find the pairs. Watch out for Bonus and Penalty cards!");
    subtitle->setStyleSheet("font-size: 14px; color: #888888;");
    subtitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(subtitle);

    layout->addSpacing(20);

    p1Edit = new QLineEdit();
    p1Edit->setPlaceholderText("Player 1 name");
    p2Edit = new QLineEdit();
    p2Edit->setPlaceholderText("Player 2 name");

    const QString editStyle =
        "QLineEdit {"
        "  padding: 10px 14px; font-size: 14px;"
        "  border: 2px solid #DDDDDD; border-radius: 10px;"
        "}"
        "QLineEdit:focus { border-color: #6C63FF; }";
    p1Edit->setStyleSheet(editStyle);
    p2Edit->setStyleSheet(editStyle);
    layout->addWidget(p1Edit);
    layout->addWidget(p2Edit);

    layout->addSpacing(10);

    auto* whoStartsLabel = new QLabel("Who goes first?");
    whoStartsLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #555555;");
    layout->addWidget(whoStartsLabel);

    auto* radioRow = new QHBoxLayout();
    auto* p1Radio = new QRadioButton("Player 1");
    auto* p2Radio = new QRadioButton("Player 2");
    p1Radio->setChecked(true);
    startingPlayerGroup = new QButtonGroup(this);
    startingPlayerGroup->addButton(p1Radio, 0);
    startingPlayerGroup->addButton(p2Radio, 1);
    radioRow->addWidget(p1Radio);
    radioRow->addWidget(p2Radio);
    radioRow->addStretch();
    layout->addLayout(radioRow);

    layout->addSpacing(24);

    auto* startButton = new QPushButton("Start Game");
    startButton->setCursor(Qt::PointingHandCursor);
    startButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #6C63FF; color: white; font-size: 16px; font-weight: bold;"
        "  padding: 14px; border-radius: 12px; border: none;"
        "}"
        "QPushButton:hover { background-color: #7D75FF; }"
        "QPushButton:pressed { background-color: #5A52E0; }"
    );
    connect(startButton, &QPushButton::clicked, this, &StartScreen::onStartClicked);
    layout->addWidget(startButton);

    setStyleSheet("background-color: #FFFDF6;");
}

void StartScreen::onStartClicked()
{
    QString p1 = p1Edit->text().trimmed();
    QString p2 = p2Edit->text().trimmed();
    if (p1.isEmpty()) p1 = "Player 1";
    if (p2.isEmpty()) p2 = "Player 2";

    int startingPlayer = startingPlayerGroup->checkedId();
    emit startRequested(p1, p2, startingPlayer);
}
