# Architecture & Design Documentation

This document is the technical companion to the [main README](../README.md).
It covers *why* the project is structured the way it is, not just *what*
each class does - the reasoning behind these decisions is as much the
point of this project as the code itself.

## Table of contents

- [Why this project was rearchitected](#why-this-project-was-rearchitected)
- [High-level architecture](#high-level-architecture)
- [Class diagram](#class-diagram)
- [GUI implementation](#gui-implementation)
- [The `Game` state machine](#the-game-state-machine)
- [Turn-resolution logic](#turn-resolution-logic)
- [Sequence diagrams](#sequence-diagrams)
- [Testing strategy](#testing-strategy)
- [Game rules reference](#game-rules-reference)

## Why this project was rearchitected

The [original version](https://github.com/ahmed-marie/INCS101-Project)
of this project had two structural weaknesses that this rewrite
specifically targets:

1. **No GUI, and no easy path to add one.** Game logic and console
   I/O (`cin`/`cout`) were interleaved directly inside the same
   methods, and the game loop *blocked* waiting for console input.
   Neither a GUI event loop nor a test can work against code shaped
   like that.
2. **Manual, unautomated tests.** The original `test.cpp` was a
   print-and-eyeball harness - useful for learning ISTQB testing
   concepts, but not something that could run in CI or give a clear
   pass/fail signal.

Both problems share one root cause, and one fix: **separate game
logic from I/O entirely**, and make that logic *event-driven* (it
reacts to one discrete input at a time and returns immediately)
rather than loop-driven (it blocks and pulls input itself). That one
change is what unlocks both a GUI and a real automated test suite.

## High-level architecture

The project follows a Model/View split, structured as three
independent build targets:

```
core/    Pure game logic. Zero I/O, zero GUI framework code.
         Depends on nothing but the C++ standard library.
gui/     Qt widgets. Reads core/ state, translates user input into
         calls on core/'s public API. Depends on core/.
tests/   GoogleTest suite. Depends on core/ only - it never touches
         gui/, since core/ is fully driveable without a UI.
```

This maps onto MVC/MVVM-style thinking as follows:

- **Model** - `Game`, `Deck`, `Player`, and the `Card` hierarchy
  (`core/`). Owns all game state and rules; has no knowledge that a
  GUI or a test even exists.
- **View** - the Qt widgets (`gui/`), which render whatever
  `Game::getSnapshot()` returns.
- **Controller** - the click-handler code living alongside those
  same Qt widgets, translating a user action (e.g. a card click)
  into a call on `Game`'s public API. In most GUI frameworks,
  including Qt, View and Controller code end up living in the same
  class - that's normal, not a design flaw; the Model/View boundary
  is the one that matters and the one this architecture strictly
  enforces.
- **`GameSnapshot`** - not one of the three MVC roles, but a
  supporting piece: a plain, disposable data-transfer object built
  fresh on every call to `getSnapshot()`. It's the *only* channel
  through which `core/` state reaches a renderer or a test.

## Class diagram

```mermaid
classDiagram
    class Card {
        <<abstract>>
        #int number
        #bool faceUp
        #CardType cardType
        +getNumber() int
        +setNumber(int)
        +isFaceUp() bool
        +setFaceUp(bool)
        +getCardType() CardType
        +getRevealMessage()* string
    }
    class StandardCard {
        +StandardCard(int number, bool faceUp)
    }
    class BonusCard {
        +BonusCard(bool faceUp)
    }
    class PenaltyCard {
        +PenaltyCard(bool faceUp)
    }
    Card <|-- StandardCard
    Card <|-- BonusCard
    Card <|-- PenaltyCard

    class Deck {
        -int removedCards
        +revealCard(row, col) CardEvent
        +evaluateFlippedCards() RevealedCardsEvent
        +revealLastCard() CardType
        +getCardAt(index) Card
        +getDeckStatus() DeckStatus
    }
    Deck "1" o-- "16" Card : owns

    class Player {
        -string name
        -int score
        -int turnsNo
        +getScore() int
        +updateScore(int)
        +incrementTurn()
        +decrementTurn()
    }

    class Game {
        -GamePhase phase
        -TurnOutcome turnOutcome
        -int currentTurn
        -int nextTurn
        +onCardClicked(row, col) CardEvent
        +finalizeTurn() TurnOutcome
        +onBonusChoice(choice) TurnOutcome
        +onPenaltyChoice(choice) TurnOutcome
        +getSnapshot() GameSnapshot
        +getPhase() GamePhase
        +getWinnerIndex() int
        -resolveRevealedPair() TurnOutcome
        -applyTurnOutcome(TurnOutcome)
        -checkDeckStatusAndAdvance()
    }
    Game "1" *-- "2" Player : owns
    Game "1" *-- "1" Deck : owns
    Game ..> GameSnapshot : builds on demand

    class GameSnapshot {
        +grid CardView
        +players PlayerView
        +currentPlayerIndex int
        +statusMessage string
    }
```

## GUI implementation

`gui/` is Qt Widgets, structured as one screen per class, plus one
class that owns the `Game` for the whole session:

```mermaid
classDiagram
    class MainWindow {
        -QStackedWidget stack
        -unique_ptr~Game~ game
        +MainWindow()
        -onStartRequested(p1, p2, startingPlayerIndex)
        -onGameOver()
        -onPlayAgainRequested()
    }
    class StartScreen {
        +startRequested(p1Name, p2Name, startingPlayerIndex)
    }
    class GameScreen {
        -Game* game
        -bool inputLocked
        +bindGame(Game*)
        -onCardButtonClicked(row, col)
        -finalizeCurrentTurn()
        +gameOver()
    }
    class EndScreen {
        +setResult(p1, p1Score, p2, p2Score, winnerIndex)
        +playAgainRequested()
    }
    class CardButton {
        -int row
        -int col
        +updateView(CardView)
        +cardClicked(row, col)
    }

    MainWindow "1" *-- "1" StartScreen : owns
    MainWindow "1" *-- "1" GameScreen : owns
    MainWindow "1" *-- "1" EndScreen : owns
    MainWindow "1" o-- "1" Game : owns\n(only class that constructs one)
    GameScreen "1" o-- "16" CardButton : owns
    GameScreen ..> Game : calls public API,\nreads via getSnapshot()
```

`MainWindow` is the only class in the whole GUI that ever constructs
a `Game` - every other screen either receives plain data (`StartScreen`
emits a signal with the names/starting player and never touches
`Game` itself) or holds a non-owning pointer into the one `MainWindow`
owns (`GameScreen::bindGame(Game*)`). This keeps the "one owner, many
readers" rule from the core architecture intact across the UI layer
too.

**The 3-second reveal delay is a GUI-only concern, deliberately.**
When the second card of a turn is flipped, `Game` stops at a
`SecondCardRevealed` phase rather than evaluating immediately (see
[state machine](#the-game-state-machine) below) - it has no concept
of time or waiting, only phases. `GameScreen` is the one place that
turns that phase into an actual pause, via
`QTimer::singleShot(3000, this, &GameScreen::finalizeCurrentTurn)`,
so the player has a chance to see the second card before `Game`
evaluates the pair. This is a concrete case of the Model/View split
paying off: the timing requirement was purely a UI/UX need, and
implementing it never touched `core/` at all - only `Game`'s *phase
model* needed a new state, not any GUI-specific code.

While that timer is running, `GameScreen` also sets a private
`inputLocked` flag to ignore further clicks - `Game::onCardClicked()`
independently rejects any click outside `AwaitingFirstCard`/
`AwaitingSecondCard` too, so a click slipping past the GUI's lock
(or a future front-end that doesn't implement one) still can't
corrupt `Deck`'s in-progress pair-tracking.

## The `Game` state machine

`Game` tracks exactly what input it's waiting for next via
`GamePhase` - this is what lets a GUI know, at any moment, whether
to accept a card click, wait out a display delay, or show a
bonus/penalty choice dialog.

```mermaid
stateDiagram-v2
    [*] --> NotStarted
    NotStarted --> AwaitingFirstCard : startGame()

    AwaitingFirstCard --> AwaitingFirstCard : onCardClicked()\n[invalid click]
    AwaitingFirstCard --> AwaitingSecondCard : onCardClicked()\n[valid click]

    AwaitingSecondCard --> AwaitingSecondCard : onCardClicked()\n[invalid click]
    AwaitingSecondCard --> SecondCardRevealed : onCardClicked()\n[valid click]

    SecondCardRevealed --> SecondCardRevealed : onCardClicked()\n[ignored - pair already pending]
    SecondCardRevealed --> AwaitingFirstCard : finalizeTurn()\n[5 immediate outcomes]
    SecondCardRevealed --> AwaitingBonusChoice : finalizeTurn()\n[TwoBonus]
    SecondCardRevealed --> AwaitingPenaltyChoice : finalizeTurn()\n[TwoPenalty]

    AwaitingBonusChoice --> AwaitingFirstCard : onBonusChoice()
    AwaitingPenaltyChoice --> AwaitingFirstCard : onPenaltyChoice()

    SecondCardRevealed --> GameOver : finalizeTurn()\n[deck empty/last card resolved]
    AwaitingBonusChoice --> GameOver : onBonusChoice()\n[deck empty/last card resolved]
    AwaitingPenaltyChoice --> GameOver : onPenaltyChoice()\n[deck empty/last card resolved]

    GameOver --> [*]
```

An invalid click (an out-of-range coordinate, or a card that's
already face-up) **never changes `phase`** - only a valid flip
progresses the state machine. This is deliberate: it means the same
click handler can safely be called with bad input without any
special-case handling on the caller's side.

**`SecondCardRevealed` is a deliberate pause, not an accident.**
`onCardClicked()` stops here the moment the second card of a turn is
flipped, *before* evaluating it - `resolveRevealedPair()` (still
private) only ever runs from inside the new public `finalizeTurn()`,
which is only valid while `phase == SecondCardRevealed`. This exists
specifically so a caller can render both revealed cards before the
game decides what they mean; without it, a mismatched pair could be
evaluated and flipped back down before the player ever saw the
second card. Any click received while already in
`SecondCardRevealed` is rejected outright (`CardEvent::NotFound`,
`phase` unchanged) rather than accepted and queued - a pair is
already pending evaluation, and `Game` only ever tracks one at a time.

## Turn-resolution logic

Once `finalizeTurn()` is called (see [state machine](#the-game-state-machine)
above - only valid from `SecondCardRevealed`), `resolveRevealedPair()`
maps the resulting `RevealedCardsEvent` to a score change and a
`TurnOutcome`. Five of the seven outcomes resolve immediately; the two
same-type cases (`TwoBonus`, `TwoPenalty`) pause and wait for the
player's choice via `onBonusChoice()`/`onPenaltyChoice()`.

```mermaid
flowchart TD
    A[Two cards revealed] --> B{Card types?}
    B -->|Both Standard, same number| C["+1 point, BonusTurn"]
    B -->|Both Standard, different number| D["+0, EndTurn (flip back down)"]
    B -->|Standard + Bonus| E["+1 point, EndTurn"]
    B -->|Standard + Penalty| F["-1 point, EndTurn"]
    B -->|Bonus + Penalty| G["+0, EndTurn"]
    B -->|Both Bonus| H{Player's choice}
    H -->|Take 2 points| I["+2 points, EndTurn"]
    H -->|Take 1 + continue| J["+1 point, BonusTurn"]
    B -->|Both Penalty| K{Player's choice}
    K -->|Lose 2 points| L["-2 points, EndTurn"]
    K -->|Lose 1 + skip next| M["-1 point, SkipTurn"]
```

## Sequence diagrams

Each diagram traces one full scenario, from the first click through
the score update - including the 3-second display pause before the
second card is evaluated. Source files live in
[`docs/diagrams/`](diagrams/).

### Two identical Standard cards

```mermaid
sequenceDiagram
    actor U as User
    participant GUI as GUI/Controller
    participant G as Game
    participant D as Deck

    U->>GUI: click card (r1,c1)
    GUI->>G: onCardClicked(r1,c1)
    G->>D: revealCard(r1,c1)
    D-->>G: CardEvent::Found
    Note over G: phase = AwaitingSecondCard
    G-->>GUI: CardEvent::Found
    GUI-->>U: render flipped card

    U->>GUI: click card (r2,c2)
    GUI->>G: onCardClicked(r2,c2)
    G->>D: revealCard(r2,c2)
    D-->>G: CardEvent::Found
    Note over G: phase = SecondCardRevealed (not evaluated yet)
    G-->>GUI: CardEvent::Found
    GUI->>G: getSnapshot()
    G-->>GUI: snapshot (both cards face up)
    GUI-->>U: render both cards revealed
    Note over GUI: start 3s display timer

    Note over GUI: ...3 seconds pass...
    GUI->>G: finalizeTurn()
    G->>G: resolveRevealedPair()
    G->>D: evaluateFlippedCards()
    D->>D: compare card1.number == card2.number
    Note over D: same number -> TwoSameStandard
    D->>D: remove both cards, removedCards += 2
    D-->>G: RevealedCardsEvent::TwoSameStandard
    G-->>G: TurnOutcome::BonusTurn
    G->>G: applyTurnOutcome(BonusTurn)
    G->>G: currentPlayer.updateScore(+1)
    G->>G: currentPlayer.incrementTurn()
    Note over G: statusMessage = "Match! Bonus turn - go again."
    Note over G: phase = AwaitingFirstCard (same player)
    G->>D: getDeckStatus()
    D-->>G: TwoOrMoreLeft
    G-->>GUI: TurnOutcome::BonusTurn
    GUI->>G: getSnapshot()
    G-->>GUI: snapshot (score updated, message set)
    GUI-->>U: render updated score + "go again" message
```

### Two different Standard cards

```mermaid
sequenceDiagram
    actor U as User
    participant GUI as GUI/Controller
    participant G as Game
    participant D as Deck

    U->>GUI: click card (r1,c1)
    GUI->>G: onCardClicked(r1,c1)
    G->>D: revealCard(r1,c1)
    D-->>G: CardEvent::Found
    Note over G: phase = AwaitingSecondCard
    G-->>GUI: CardEvent::Found
    GUI-->>U: render flipped card

    U->>GUI: click card (r2,c2)
    GUI->>G: onCardClicked(r2,c2)
    G->>D: revealCard(r2,c2)
    D-->>G: CardEvent::Found
    Note over G: phase = SecondCardRevealed (not evaluated yet)
    G-->>GUI: CardEvent::Found
    GUI->>G: getSnapshot()
    G-->>GUI: snapshot (both cards face up)
    GUI-->>U: render both cards revealed
    Note over GUI: start 3s display timer

    Note over GUI: ...3 seconds pass...
    GUI->>G: finalizeTurn()
    G->>G: resolveRevealedPair()
    G->>D: evaluateFlippedCards()
    D->>D: compare card1.number != card2.number
    Note over D: different numbers -> TwoDifferentStandard
    D->>D: flip both cards back face-down (not removed)
    D-->>G: RevealedCardsEvent::TwoDifferentStandard
    G-->>G: TurnOutcome::EndTurn
    G->>G: applyTurnOutcome(EndTurn)
    G->>G: currentPlayer.updateScore(+0)
    Note over G: statusMessage = "No match. Turn passes."
    Note over G: currentTurn = nextTurn, phase = AwaitingFirstCard
    G->>D: getDeckStatus()
    D-->>G: TwoOrMoreLeft
    G-->>GUI: TurnOutcome::EndTurn
    GUI->>G: getSnapshot()
    G-->>GUI: snapshot (cards face-down again, next player's turn)
    GUI-->>U: render cards flipping back + turn indicator change
```

### Two Bonus cards

```mermaid
sequenceDiagram
    actor U as User
    participant GUI as GUI/Controller
    participant G as Game
    participant D as Deck

    U->>GUI: click card (r1,c1)
    GUI->>G: onCardClicked(r1,c1)
    G->>D: revealCard(r1,c1)
    D-->>G: CardEvent::Found
    Note over G: phase = AwaitingSecondCard
    G-->>GUI: CardEvent::Found
    GUI-->>U: render flipped card

    U->>GUI: click card (r2,c2)
    GUI->>G: onCardClicked(r2,c2)
    G->>D: revealCard(r2,c2)
    D-->>G: CardEvent::Found
    Note over G: phase = SecondCardRevealed (not evaluated yet)
    G-->>GUI: CardEvent::Found
    GUI->>G: getSnapshot()
    G-->>GUI: snapshot (both cards face up)
    GUI-->>U: render both cards revealed
    Note over GUI: start 3s display timer

    Note over GUI: ...3 seconds pass...
    GUI->>G: finalizeTurn()
    G->>G: resolveRevealedPair()
    G->>D: evaluateFlippedCards()
    D->>D: both cards are Bonus type
    D->>D: remove both cards, removedCards += 2
    D-->>G: RevealedCardsEvent::TwoBonus
    Note over G: outcome unknown yet - waiting on player
    G->>G: phase = AwaitingBonusChoice
    G-->>GUI: TurnOutcome::Pending
    GUI->>G: getSnapshot()
    G-->>GUI: snapshot (phase = AwaitingBonusChoice)
    GUI-->>U: show "2 points, or 1 point + extra turn?" dialog

    U->>GUI: selects option
    GUI->>G: onBonusChoice(choice)

    alt choice == 1 (take 2 points)
        G->>G: applyTurnOutcome(EndTurn)
        G->>G: currentPlayer.updateScore(+2)
        Note over G: statusMessage = "Took 2 points. Turn passes."
    else choice == 2 (take 1 point + extra turn)
        G->>G: applyTurnOutcome(BonusTurn)
        G->>G: currentPlayer.updateScore(+1)
        G->>G: currentPlayer.incrementTurn()
        Note over G: statusMessage = "Took 1 point. Bonus turn!"
    end

    G->>D: getDeckStatus()
    D-->>G: TwoOrMoreLeft
    G-->>GUI: TurnOutcome (EndTurn or BonusTurn)
    GUI->>G: getSnapshot()
    G-->>GUI: snapshot (score updated, message set)
    GUI-->>U: render updated score + message
```

### Two Penalty cards

```mermaid
sequenceDiagram
    actor U as User
    participant GUI as GUI/Controller
    participant G as Game
    participant D as Deck

    U->>GUI: click card (r1,c1)
    GUI->>G: onCardClicked(r1,c1)
    G->>D: revealCard(r1,c1)
    D-->>G: CardEvent::Found
    Note over G: phase = AwaitingSecondCard
    G-->>GUI: CardEvent::Found
    GUI-->>U: render flipped card

    U->>GUI: click card (r2,c2)
    GUI->>G: onCardClicked(r2,c2)
    G->>D: revealCard(r2,c2)
    D-->>G: CardEvent::Found
    Note over G: phase = SecondCardRevealed (not evaluated yet)
    G-->>GUI: CardEvent::Found
    GUI->>G: getSnapshot()
    G-->>GUI: snapshot (both cards face up)
    GUI-->>U: render both cards revealed
    Note over GUI: start 3s display timer

    Note over GUI: ...3 seconds pass...
    GUI->>G: finalizeTurn()
    G->>G: resolveRevealedPair()
    G->>D: evaluateFlippedCards()
    D->>D: both cards are Penalty type
    D->>D: remove both cards, removedCards += 2
    D-->>G: RevealedCardsEvent::TwoPenalty
    Note over G: outcome unknown yet - waiting on player
    G->>G: phase = AwaitingPenaltyChoice
    G-->>GUI: TurnOutcome::Pending
    GUI->>G: getSnapshot()
    G-->>GUI: snapshot (phase = AwaitingPenaltyChoice)
    GUI-->>U: show "lose 2 points, or 1 point + skip a turn?" dialog

    U->>GUI: selects option
    GUI->>G: onPenaltyChoice(choice)

    alt choice == 1 (lose 2 points)
        G->>G: applyTurnOutcome(EndTurn)
        G->>G: currentPlayer.updateScore(-2)
        Note over G: statusMessage = "Lost 2 points. Turn passes."
    else choice == 2 (lose 1 point + skip next turn)
        G->>G: applyTurnOutcome(SkipTurn)
        G->>G: currentPlayer.updateScore(-1)
        G->>G: currentPlayer.decrementTurn()
        Note over G: statusMessage = "Lost 1 point. Next turn skipped."
    end

    G->>D: getDeckStatus()
    D-->>G: TwoOrMoreLeft
    G-->>GUI: TurnOutcome (EndTurn or SkipTurn)
    GUI->>G: getSnapshot()
    G-->>GUI: snapshot (score updated, message set)
    GUI-->>U: render updated score + message
```

## Testing strategy

The test suite (`tests/`) mirrors `core/`'s module structure, one
file per class:

- `CardTests.cpp`, `PlayerTests.cpp`, `DeckTests.cpp` - unit tests,
  each class exercised in isolation.
- `GameTests.cpp` - integration tests: `Game` orchestrates `Deck`
  and `Player` together, so tests here exercise that orchestration
  through `Game`'s public API only (`onCardClicked`,
  `onBonusChoice`/`onPenaltyChoice`, `getSnapshot`) - private
  helper methods are exercised indirectly, never accessed directly.
- `Deck` and `Game` both expose a constructor that accepts a
  pre-built, non-shuffled state (`Deck(std::array<...>)`,
  `Game(Deck)`), specifically so tests can set up an exact scenario
  (e.g. "two Bonus cards at known positions") deterministically,
  without depending on `shuffle()`'s randomness.

*(This section will be expanded with actual coverage details once
the GoogleTest suite lands.)*

## Game rules reference

**Setup:** 16 cards in a 4x4 grid - 6 matching pairs of Standard
cards (numbered 1-6), one pair of Bonus cards, one pair of Penalty
cards. All cards start face-down.

**A turn:** the current player flips two cards. What happens next
depends on what's revealed - see the [turn-resolution flowchart](#turn-resolution-logic)
above for the complete rule set.

**End of game:** play continues until the grid is empty. Highest
score wins; equal scores end in a draw.