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
- [Automatic endgame reveals](#automatic-endgame-reveals)
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
        +evaluateLastCard() CardType
        +revealLastPair()
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
        +revealFinalCard() CardType
        +finalizeLastCard() CardType
        +revealFinalPair()
        +finalizeLastPair() TurnOutcome
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
        -revealFinalCardAndFinish()
        -evaluateFinalCardAndFinish()
        -revealFinalPairAndFinish()
        -evaluateFinalPairAndFinish()
        -finishIfGameOver()
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

**The 1.5-second reveal delay is a GUI-only concern, deliberately.**
When the second card of a turn is flipped, `Game` stops at a
`SecondCardRevealed` phase rather than evaluating immediately (see
[state machine](#the-game-state-machine) below) - it has no concept
of time or waiting, only phases. `GameScreen` is the one place that
turns that phase into an actual pause, via `QTimer::singleShot()`,
so the player has a chance to see the second card before `Game`
evaluates the pair. This is a concrete case of the Model/View split
paying off: the timing requirement was purely a UI/UX need, and
implementing it never touched `core/` at all - only `Game`'s *phase
model* needed a new state, not any GUI-specific code. The same
pattern repeats twice more for the two endgame phases
(`AwaitingLastCardReveal`/`LastCardRevealed` and
`AwaitingLastPairReveal`/`LastPairRevealed`) - each is a
reveal-then-pause-then-evaluate chain of two `QTimer::singleShot()`
calls in `GameScreen`, never in `core/`.

**`finishIfGameOver()` exists to fix a real rendering race, not as
extra caution.** `GameScreen::gameOver()` is direct-connected to
`MainWindow::onGameOver()`, which synchronously swaps this screen
out for the End Screen. Calling `refresh()` and then immediately
`emit gameOver()` in the same function - which every endgame path
used to do - meant the screen could swap away *before* Qt's event
loop ever got a chance to actually paint what `refresh()` had just
scheduled (`setText()`/`setStyleSheet()` only schedule a repaint,
they don't force one). `finishIfGameOver()` delays the emission by a
short `QTimer::singleShot()`, guaranteeing a real repaint happens
first. Every path that can reach `GameOver` - a normal turn, a
choice, the final card, the final pair - funnels through this one
method rather than emitting `gameOver()` directly, so the fix only
needed to exist in one place.

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
    SecondCardRevealed --> AwaitingBonusChoice : finalizeTurn()\n[TwoBonus, 3+ cards remain after]
    SecondCardRevealed --> AwaitingPenaltyChoice : finalizeTurn()\n[TwoPenalty, 3+ cards remain after]
    SecondCardRevealed --> AwaitingFirstCard : finalizeTurn()\n[resolved, 3+ cards remain]
    SecondCardRevealed --> AwaitingLastCardReveal : finalizeTurn()\n[resolved, 1 card remains]
    SecondCardRevealed --> AwaitingLastPairReveal : finalizeTurn()\n[resolved, 2 cards remain]
    SecondCardRevealed --> GameOver : finalizeTurn()\n[deck now empty]

    AwaitingBonusChoice --> AwaitingFirstCard : onBonusChoice()\n[3+ cards remain]
    AwaitingBonusChoice --> AwaitingLastCardReveal : onBonusChoice()\n[1 card remains]
    AwaitingBonusChoice --> AwaitingLastPairReveal : onBonusChoice()\n[2 cards remain]

    AwaitingPenaltyChoice --> AwaitingFirstCard : onPenaltyChoice()\n[3+ cards remain]
    AwaitingPenaltyChoice --> AwaitingLastCardReveal : onPenaltyChoice()\n[1 card remains]
    AwaitingPenaltyChoice --> AwaitingLastPairReveal : onPenaltyChoice()\n[2 cards remain]

    AwaitingLastCardReveal --> LastCardRevealed : revealFinalCard()
    LastCardRevealed --> GameOver : finalizeLastCard()

    AwaitingLastPairReveal --> LastPairRevealed : revealFinalPair()
    LastPairRevealed --> GameOver : finalizeLastPair()\n[always ends the game]

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

**`AwaitingLastCardReveal`/`LastCardRevealed` and
`AwaitingLastPairReveal`/`LastPairRevealed` follow the exact same
pause-then-act pattern**, for the two endgame edge cases where the
deck runs down to a single orphaned Bonus/Penalty card, or a final
pair. In both cases the reveal and the scoring/removal are two
separate steps (`revealFinalCard()`/`finalizeLastCard()`,
`revealFinalPair()`/`finalizeLastPair()`) specifically so a caller
can render the reveal before the score changes - see
[Automatic endgame reveals](#automatic-endgame-reveals) below for
why this needed to be two steps rather than one.

Notice these two new phases are entered automatically by
`checkDeckStatusAndAdvance()` - the player is **never given a click**
to flip the final one or two cards themselves; there's nothing left
to click "against." This is different from every earlier phase in
the diagram, where the caller always supplies a value (a coordinate,
a choice) - `revealFinalCard()`/`revealFinalPair()` take no
arguments at all, they only need to be *called*, whenever the caller
is ready.

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
    B -->|Both Bonus| H{Does this pair\nempty the deck?}
    H -->|Yes - last 2 cards| H1["+2 points, EndTurn\n(no choice offered - see below)"]
    H -->|No - cards remain| H2{Player's choice}
    H2 -->|Take 2 points| I["+2 points, EndTurn"]
    H2 -->|Take 1 + continue| J["+1 point, BonusTurn"]
    B -->|Both Penalty| K{Does this pair\nempty the deck?}
    K -->|Yes - last 2 cards| K1["-2 points, EndTurn\n(no choice offered - see below)"]
    K -->|No - cards remain| K2{Player's choice}
    K2 -->|Lose 2 points| L["-2 points, EndTurn"]
    K2 -->|Lose 1 + skip next| M["-1 point, SkipTurn"]
```

**The "does this pair empty the deck?" branches exist for a concrete
reason, not just extra polish.** Choosing between "+2 points, end
turn" and "+1 point, bonus turn" only matters because of what
happens *next* - whether the same player keeps playing. If this pair
was the literal last two cards in the deck, there is no "next" -
both choices lead to the same place, `GameOver`, immediately. Since
the choice can't actually change anything, it's skipped entirely
rather than shown as a dialog whose answer doesn't matter; a fixed
score is applied directly. This check happens inside
`resolveRevealedPair()` itself, checking `deck.getDeckStatus() ==
DeckStatus::Empty` right after the pair has already been removed -
see [Automatic endgame reveals](#automatic-endgame-reveals) for how
this same check gets reused for the fully-automatic final-pair case.

## Sequence diagrams

Each diagram traces one full scenario, from the first click through
the score update - including the display pause before the second
card is evaluated. Source files live in
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
    D-->>G: ThreeOrMoreLeft
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
    D-->>G: ThreeOrMoreLeft
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
    D-->>G: ThreeOrMoreLeft
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
    D-->>G: ThreeOrMoreLeft
    G-->>GUI: TurnOutcome (EndTurn or SkipTurn)
    GUI->>G: getSnapshot()
    G-->>GUI: snapshot (score updated, message set)
    GUI-->>U: render updated score + message
```

## Automatic endgame reveals

These two scenarios are different in kind from every diagram above:
the player never clicks anything. `checkDeckStatusAndAdvance()`
detects "exactly one card left" or "exactly two cards left"
automatically, right after any turn resolves, and transitions
directly into a reveal phase - there's nothing left to click
"against."

### Final card (one Bonus/Penalty card remains)

```mermaid
sequenceDiagram
    actor U as User
    participant GUI as GUI/Controller
    participant G as Game
    participant D as Deck

    Note over G: phase = AwaitingLastCardReveal\n(reached automatically via checkDeckStatusAndAdvance()\nafter any turn - no click involved)

    GUI->>G: getPhase()
    G-->>GUI: AwaitingLastCardReveal
    Note over GUI: start display timer

    Note over GUI: ...timer elapses...
    GUI->>G: revealFinalCard()
    G->>D: revealLastCard()
    D->>D: flip the final card face-up\n(does NOT remove it yet)
    D-->>G: CardType (Bonus or Penalty)
    Note over G: phase = LastCardRevealed
    G-->>GUI: CardType
    GUI->>G: getSnapshot()
    G-->>GUI: snapshot (card face-up, not yet scored)
    GUI-->>U: render the revealed final card
    Note over GUI: start second display timer

    Note over GUI: ...timer elapses...
    GUI->>G: finalizeLastCard()
    G->>D: evaluateLastCard()
    D->>D: remove the card, removedCards += 1
    D-->>G: CardType
    G->>G: apply +1 (Bonus) or -1 (Penalty) score effect
    Note over G: phase = GameOver
    G-->>GUI: CardType
    GUI->>G: getSnapshot()
    G-->>GUI: snapshot (final scores, empty grid)
    GUI-->>U: render final card removed + updated score,\nthen (after finishIfGameOver()'s delay) switch to End Screen
```

### Final pair (two cards remain)

```mermaid
sequenceDiagram
    actor U as User
    participant GUI as GUI/Controller
    participant G as Game
    participant D as Deck

    Note over G: phase = AwaitingLastPairReveal\n(exactly two cards remain - no player click\ninvolved at all, unlike every other turn)

    GUI->>G: getPhase()
    G-->>GUI: AwaitingLastPairReveal
    Note over GUI: start display timer

    Note over GUI: ...timer elapses...
    GUI->>G: revealFinalPair()
    G->>D: revealLastPair()
    D->>D: flip BOTH remaining cards face-up\n(does NOT remove/score them yet)
    Note over G: phase = LastPairRevealed
    GUI->>G: getSnapshot()
    G-->>GUI: snapshot (both cards face-up, not yet scored)
    GUI-->>U: render both revealed cards
    Note over GUI: start second display timer

    Note over GUI: ...timer elapses...
    GUI->>G: finalizeLastPair()
    G->>G: resolveRevealedPair()\n(same scoring logic as a player-revealed pair -\nincluding the "pair empties the deck, skip any\nBonus/Penalty choice" rule)
    G->>D: evaluateFlippedCards()
    D->>D: score and remove both cards
    D-->>G: RevealedCardsEvent
    G->>G: applyTurnOutcome(...)
    G->>G: checkDeckStatusAndAdvance()
    Note over G: phase = GameOver (deck is now empty)
    G-->>GUI: TurnOutcome
    GUI->>G: getSnapshot()
    G-->>GUI: snapshot (final scores, empty grid)
    GUI-->>U: render final result, then switch to End Screen
```

`finalizeLastPair()` deliberately calls `resolveRevealedPair()`
rather than duplicating its scoring logic - the same "did this pair
just empty the deck?" check from
[turn-resolution logic](#turn-resolution-logic) above is what makes
the Bonus/Bonus and Penalty/Penalty cases apply their fixed score
correctly here too, with no separate code path needed.

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

**End of game:** play continues until the grid is empty. If exactly
one Bonus/Penalty card or one matching pair is left at any point,
it's revealed and scored automatically (see
[Automatic endgame reveals](#automatic-endgame-reveals)) - the player
never has to click these. If the final action would offer a
Bonus/Bonus or Penalty/Penalty choice with nothing left to
differentiate the outcomes, the choice is skipped and a fixed score
applied directly. Highest score wins; equal scores end in a draw.