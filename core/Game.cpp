#include "Game.h"

Game::Game()
{
    currentTurn = -1;
    nextTurn = -1;
    phase = GamePhase::NotStarted;
    statusMessage = std::string();
    turnOutcome = TurnOutcome::Pending;
    deck.shuffle();
}

Game::Game(Deck presetDeck)
{
    currentTurn = -1;
    nextTurn = -1;
    phase = GamePhase::NotStarted;
    statusMessage = std::string();
    turnOutcome = TurnOutcome::Pending;
    deck = std::move(presetDeck);
}

void Game::setPlayerNames(const std::string& p1Name, const std::string& p2Name)
{
    players[0].setName(p1Name);
    players[1].setName(p2Name);
}

void Game::startGame(int startingPlayerIndex) // 0 or 1
{
    currentTurn = startingPlayerIndex;
    nextTurn = (currentTurn + 1) % PLAYER_NUMBERS;
    players[currentTurn].incrementTurn();
    phase = GamePhase::AwaitingFirstCard;
}

// --- Actions driven by the caller (GUI click handler, or a test) ---

CardEvent Game::onCardClicked(int row, int col)
{
    if (!((1 <= row) && (row <= GRID_SIZE) && (1 <= col) && (col <= GRID_SIZE)))
    {
        return CardEvent::NotFound;
    }

    // Only accept a click while genuinely waiting for a card flip.
    // In particular, while phase == SecondCardRevealed a pair is
    // already flipped and pending finalizeTurn() - accepting a third
    // reveal here would corrupt Deck's two-slot revealedCardsIndex
    // bookkeeping. This guard exists regardless of what any particular
    // caller (GUI, test) does or doesn't enforce on its own side.
    if (phase != GamePhase::AwaitingFirstCard && phase != GamePhase::AwaitingSecondCard)
    {
        return CardEvent::NotFound;
    }

    CardEvent event = deck.revealCard(row, col);

    if (event != CardEvent::Found)
    {
        // invalid click (not found / already revealed) - phase must
        // stay exactly where it was, only the caller's feedback changes
        return event;
    }

    if (phase == GamePhase::AwaitingFirstCard)
    {
        // first card of the turn - just wait for the second
        phase = GamePhase::AwaitingSecondCard;
    }
    else if (phase == GamePhase::AwaitingSecondCard)
    {
        // second card of the turn - both cards are now face-up.
        // Deliberately stop here rather than evaluating immediately,
        // so the caller can render this state and let the player
        // actually see the second card before finalizeTurn() runs.
        phase = GamePhase::SecondCardRevealed;
    }

    return event;
}

TurnOutcome Game::finalizeTurn()
{
    if (phase != GamePhase::SecondCardRevealed)
    {
        return TurnOutcome::Pending;
    }

    turnOutcome = resolveRevealedPair();

    if (turnOutcome != TurnOutcome::Pending)
    {
        // one of the 5 immediate cases - apply turn-credit/shift
        // logic and check the deck right away
        applyTurnOutcome(turnOutcome);
        checkDeckStatusAndAdvance();
    }
    // else: TwoBonus/TwoPenalty - resolveRevealedPair() already set
    // phase to AwaitingBonusChoice/AwaitingPenaltyChoice; applyTurnOutcome()
    // runs later, from onBonusChoice()/onPenaltyChoice()

    return turnOutcome;
}

TurnOutcome Game::resolveRevealedPair()
{
    RevealedCardsEvent event = deck.evaluateFlippedCards();
    int score_delta = 0;

    //TurnOutcome turn = TurnOutcome::Pending;

    switch (event)
    {
    case RevealedCardsEvent::TwoSameStandard:
        score_delta = 1;
        turnOutcome = TurnOutcome::BonusTurn;
        statusMessage = std::string("Match! Earned 1 point. Bonus turn!");
        break;
    case RevealedCardsEvent::TwoDifferentStandard:
        score_delta = 0;
        turnOutcome = TurnOutcome::EndTurn;
        statusMessage = std::string("No match. Turn passes.");
        break;
    case RevealedCardsEvent::StandardAndPenalty:
        score_delta = -1;
        turnOutcome = TurnOutcome::EndTurn;
        statusMessage = std::string("Lost 1 point. Turn passes.");
        break;
    case RevealedCardsEvent::StandardAndBonus:
        score_delta = 1;
        turnOutcome = TurnOutcome::EndTurn;
        statusMessage = std::string("Earned 1 point. Turn passes.");
        break;
    case RevealedCardsEvent::BonusAndPenalty:
        score_delta = 0;
        turnOutcome = TurnOutcome::EndTurn;
        statusMessage = std::string("No points. Turn passes.");
        break;
    case RevealedCardsEvent::TwoBonus:
        // outcome depends on the player's choice - pause here and
        // let onBonusChoice() finish the job. Deck already fully
        // removed both cards inside evaluateFlippedCards() above,
        // regardless of what the player picks.
        phase = GamePhase::AwaitingBonusChoice;
        statusMessage = std::string(
            "Two Bonus Cards! 1: Take 2 points  2: Take 1 point + extra turn");
        turnOutcome = TurnOutcome::Pending;
        return turnOutcome;
    case RevealedCardsEvent::TwoPenalty:
        phase = GamePhase::AwaitingPenaltyChoice;
        statusMessage = std::string(
            "Two Penalty Cards! 1: Lose 2 points  2: Lose 1 point + skip turn");
        turnOutcome = TurnOutcome::Pending;
        return turnOutcome;
    default:
        break;
    }

    // one of the 5 immediate cases - apply the score now; phase and
    // turn-credit handling are the caller's (onCardClicked's) job
    players[currentTurn].updateScore(score_delta);
    return turnOutcome;
}

TurnOutcome Game::onBonusChoice(int choice)
{
    if (phase != GamePhase::AwaitingBonusChoice)
    {
        return TurnOutcome::Pending;
    }
    // reset, don't trust leftover state

    turnOutcome = TurnOutcome::Pending;

    if (choice == 1)
    {
        players[currentTurn].updateScore(2);
        turnOutcome = TurnOutcome::EndTurn;
        statusMessage = std::string("Took 2 points. Turn passes.");
    }
    else if (choice == 2)
    {
        players[currentTurn].updateScore(1);
        turnOutcome = TurnOutcome::BonusTurn;
        statusMessage = std::string("Took 1 point. Bonus turn!");
    }

    if (turnOutcome == TurnOutcome::EndTurn || turnOutcome == TurnOutcome::BonusTurn)
    {
        applyTurnOutcome(turnOutcome);
        checkDeckStatusAndAdvance();
    }

    return turnOutcome;
}

TurnOutcome Game::onPenaltyChoice(int choice)
{
    if (phase != GamePhase::AwaitingPenaltyChoice)
    {
        return TurnOutcome::Pending;
    }
    // reset, don't trust leftover state

    turnOutcome = TurnOutcome::Pending;

    if (choice == 1)
    {
        players[currentTurn].updateScore(-2);
        turnOutcome = TurnOutcome::EndTurn;
        statusMessage = std::string("Lost 2 points. Turn passes.");
    }
    else if (choice == 2)
    {
        players[currentTurn].updateScore(-1);
        turnOutcome = TurnOutcome::SkipTurn;
        statusMessage = std::string("Lost 1 point. Next turn skipped!");
    }

    if (turnOutcome == TurnOutcome::EndTurn || turnOutcome == TurnOutcome::SkipTurn)
    {
        applyTurnOutcome(turnOutcome);
        checkDeckStatusAndAdvance();
    }

    return turnOutcome;
}

// --- Read-only state for rendering and for tests ---

GameSnapshot Game::getSnapshot() const
{
    GameSnapshot snapshot;

    for (int row = 0; row < GRID_SIZE; ++row)
    {
        for (int col = 0; col < GRID_SIZE; ++col)
        {
            int index = row * GRID_SIZE + col;
            const Card* card = deck.getCardAt(index);

            CardView view; // defaults: exists=false, faceUp=false, number=0
            if (card != nullptr)
            {
                view.exists = true;
                view.faceUp = card->isFaceUp();
                view.number = card->isFaceUp() ? card->getNumber() : 0;
                view.type = card->getCardType();
            }
            snapshot.grid[row][col] = view;
        }
    }

    for (int i = 0; i < PLAYER_NUMBERS; ++i)
    {
        snapshot.players[i].name = players[i].getName();
        snapshot.players[i].score = players[i].getScore();
        snapshot.players[i].turnsRemaining = players[i].getTurnsNo();
    }

    snapshot.currentPlayerIndex = currentTurn;
    snapshot.statusMessage = statusMessage;

    return snapshot;
}

GamePhase Game::getPhase() const
{
    return phase;
}

int Game::getWinnerIndex() const
{
    if (players[0].getScore() > players[1].getScore())
    {
        return 0;
    }
    else if (players[0].getScore() < players[1].getScore())
    {
        return 1;
    }
    else
    {
        return -1; // draw
    }
}

void Game::applyTurnOutcome(TurnOutcome outcome)
{
    // spend one turn-credit for the turn that just completed
    players[currentTurn].decrementTurn();

    switch (outcome)
    {
    case TurnOutcome::EndTurn:
        // only actually pass control once the current player has
        // no turn-credits left (mirrors the old turnsNo > 0 loop
        // condition, just expressed without a loop)
        if (players[currentTurn].getTurnsNo() <= 0)
        {
            players[nextTurn].incrementTurn();
            currentTurn = nextTurn;
            nextTurn = (nextTurn + 1) % PLAYER_NUMBERS;
        }
        break;
    case TurnOutcome::BonusTurn:
        // same player continues - give them back the credit
        // decrementTurn() just spent
        players[currentTurn].incrementTurn();
        break;
    case TurnOutcome::SkipTurn:
        // hand the other player two turns in a row instead of
        // letting the skipped player's credit go negative
        players[nextTurn].incrementTurn(2);
        currentTurn = nextTurn;
        nextTurn = (nextTurn + 1) % PLAYER_NUMBERS;
        break;
    default:
        break;
    }
}

void Game::checkDeckStatusAndAdvance()
{
    DeckStatus status = deck.getDeckStatus();

    switch (status)
    {
    case DeckStatus::Empty:
        phase = GamePhase::GameOver;
        break;
    case DeckStatus::OneCardLeft:
    {
        CardType cardType = deck.revealLastCard();
        if (cardType == CardType::Bonus)
        {
            players[currentTurn].updateScore(1);
        }
        else if (cardType == CardType::Penalty)
        {
            players[currentTurn].updateScore(-1);
        }
        phase = GamePhase::GameOver;
        break;
    }
    case DeckStatus::TwoOrMoreLeft:
        phase = GamePhase::AwaitingFirstCard;
        turnOutcome = TurnOutcome::Pending;
        break;
    default:
        break;
    }
}