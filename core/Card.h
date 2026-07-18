#pragma once
#include <string>

enum class CardType { Standard, Bonus, Penalty };

// Abstract base for all card kinds. Note what's NOT here anymore:
// no cout, no display() that prints. Card describes itself; the
// caller (GUI widget, console frontend, or a test) decides how
// to render that description. This is what makes Card testable
// without a screen and renderable without touching game logic.
class Card {
public:
    Card(int number, bool faceUp);
    virtual ~Card() = default;

    void setNumber(int number);
    int getNumber() const;

    void setFaceUp(bool faceUp);
    bool isFaceUp() const;

    CardType getCardType() const;

    // Type-specific message, e.g. "Bonus Card is revealed".
    // Still virtual/polymorphic (this is the OOP showcase),
    // but returns data instead of performing I/O.
    virtual std::string getRevealMessage() const = 0;

protected:
    int number;
    bool faceUp;
    CardType cardType;
};
