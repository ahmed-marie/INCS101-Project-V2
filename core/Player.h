#pragma once
#include <string>

// Almost unchanged from your original - this class was already
// well-decoupled (it never printed anything except displayScore()).
// displayScore() is gone: the GUI reads getName()/getScore() and
// renders them itself; tests call the same getters to assert.
class Player {
public:
 
    explicit Player(std::string name = std::string(), int score = 0);

    ~Player() = default;

    void setName(const std::string& name);
    const std::string& getName() const;

    void setScore(int score);
    int getScore() const;
    void updateScore(int delta);

    void setTurnsNo(int turns);
    int getTurnsNo() const;
    void incrementTurn();
    void incrementTurn(int turn);
    void decrementTurn();

private:
    std::string name;
    int score;
    int turnsNo;
};
