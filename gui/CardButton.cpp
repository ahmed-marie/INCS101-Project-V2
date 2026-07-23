#include "CardButton.h"
#include <QFont>

CardButton::CardButton(int row, int col, QWidget* parent)
    : QPushButton(parent), row(row), col(col)
{
    setFixedSize(90, 90);
    setCursor(Qt::PointingHandCursor);

    QFont f = font();
    f.setPointSize(20);
    f.setBold(true);
    setFont(f);

    connect(this, &QPushButton::clicked, this, [this]() {
        emit cardClicked(this->row, this->col);
    });

    applyFaceDownStyle();
}

void CardButton::updateView(const CardView& view)
{
    if (!view.exists)
    {
        applyEmptySlotStyle();
        return;
    }

    if (!view.faceUp)
    {
        applyFaceDownStyle();
        return;
    }

    applyFaceUpStyle(view);
}

void CardButton::applyFaceDownStyle()
{
    setEnabled(true);
    setText("?");
    setStyleSheet(
        "QPushButton {"
        "  background-color: #6C63FF;"
        "  color: white;"
        "  border: 3px solid #4B44CC;"
        "  border-radius: 14px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #7D75FF;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #5A52E0;"
        "}"
    );
}

void CardButton::applyFaceUpStyle(const CardView& view)
{
    // already revealed this turn - not clickable again until it
    // either gets removed (match) or flips back down (no match)
    setEnabled(false);

    QString text;
    QString bg;
    QString border;

    switch (view.type)
    {
    case CardType::Bonus:
        text = "*";
        bg = "#FFC93C";
        border = "#E0A800";
        break;
    case CardType::Penalty:
        text = "!";
        bg = "#FF6B6B";
        border = "#D64545";
        break;
    case CardType::Standard:
    default:
        text = QString::number(view.number);
        bg = "#4ECDC4";
        border = "#2FA89F";
        break;
    }

    setText(text);
    setStyleSheet(QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: white;"
        "  border: 3px solid %2;"
        "  border-radius: 14px;"
        "}"
    ).arg(bg, border));
}

void CardButton::applyEmptySlotStyle()
{
    setEnabled(false);
    setText("");
    setStyleSheet(
        "QPushButton {"
        "  background-color: transparent;"
        "  border: 3px dashed #DDDDDD;"
        "  border-radius: 14px;"
        "}"
    );
}
