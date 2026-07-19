#include "Player.h"

Player::Player(std::string name, int score)
{
	this->name = name;
	this->score = score;
	turnsNo = 0;
}


Player::~Player() {}


const std::string& Player::getName() const
{
	return name;
}


void Player::setName(std::string name)
{
	this->name = name;
}

void Player::setScore(int score)
{
	this->score = score;
}

int Player::getScore() const
{
	return score;
}

void Player::updateScore(int delta)
{
	this->score += delta;
}

void Player::setTurnsNo(int turns)
{
	this->turnsNo = turns;
}

int Player::getTurnsNo() const
{
	return turnsNo;
}

void Player::incrementTurn()
{
	++turnsNo;
}

void Player::incrementTurn(int turn)
{
	turnsNo+=turn;
}

void Player::decrementTurn()
{
	--turnsNo;
}
