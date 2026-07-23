# Memory Match — C++ Card Game

A two-player memory-matching card game, built as a hands-on exercise in
professional C++ engineering practice: clean object-oriented design, a
UI-agnostic game engine, a Qt desktop front-end, and an automated
GoogleTest suite.

## About This Project

This started as a course project (Programming III / INCS101) implemented
as a single-file console application. It has since been **rearchitected
from the ground up** as a self-directed exercise in modern C++ and
software engineering practice — decoupling game logic from I/O, adding
dependency-injectable classes for deterministic testing, and rebuilding
the interface as a proper desktop GUI.

The original console version is preserved for comparison here:
**[INCS101-Project (v1)](https://github.com/ahmed-marie/INCS101-Project)**.

## Project Status

| Component | Status |
|---|---|
| Core game engine (`core/`) | ✅ Implemented |
| Qt desktop GUI (`gui/`) | ✅ Implemented |
| Automated unit & integration tests (GoogleTest) | 🔄 In progress |
| Continuous Integration (GitHub Actions) | 🔜 Planned |

The game is fully playable end to end: start screen, board, bonus/
penalty choice dialogs, and an end screen with a "Play Again" option.
See [Building and Running](#building-and-running) below.

## Highlights for Technical Reviewers

- **UI-agnostic core** — the entire game engine (`core/`) has zero
  dependency on any I/O framework (no `<iostream>`, no GUI includes).
  It communicates state through a single read-only `GameSnapshot`
  struct, making it consumable by a console, a Qt GUI, or a test suite
  identically.
- **Event-driven state machine** — the game logic reacts to discrete
  actions (`onCardClicked`, `onBonusChoice`, ...) rather than blocking
  on input, so it can drive a responsive GUI instead of a linear
  console loop.
- **Designed for deterministic testing** — both `Deck` and `Game` have
  constructors that accept a known, non-shuffled card layout, so test
  scenarios (e.g. "two bonus cards revealed") are fully reproducible
  rather than depending on `rand()`.
- **Modern C++** — `std::unique_ptr`-managed ownership, `enum class`
  throughout, CMake-based cross-platform build.
- **Core stays UI-agnostic even under real GUI constraints** — when a
  card needs to visibly stay revealed for a few seconds before the
  game evaluates it, that delay lives entirely in `gui/` via
  `QTimer::singleShot()`. `Game` itself has no concept of time or
  waiting, only an explicit `SecondCardRevealed` phase — proof the
  Model/View split holds up under a real interaction requirement, not
  just in theory.

See **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** for the full
technical write-up: class diagram, game-flow state machine, and
sequence diagrams for each turn-resolution scenario.

## Building and Running

Requires Qt6 (Widgets) installed and discoverable by CMake — see
`CMakeUserPresets.json` if you need to point CMake at a non-standard
Qt install location (this file is gitignored; it's machine-specific).

```bash
git clone https://github.com/ahmed-marie/INCS101-Project-V2.git
cd INCS101-Project-V2
cmake -S . -B build
cmake --build build
```

This builds two targets: `core` (the game engine, as a static
library) and `gui` (the playable Qt application). Run the game with:

```bash
./build/gui/gui        # Linux/macOS
build\gui\gui.exe       # Windows
```

## Tech Stack

C++20 · CMake · GoogleTest · Qt6

## Documentation

- **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** — architecture,
  class diagram, game rules, state machine, and sequence diagrams