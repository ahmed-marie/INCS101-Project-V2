# core/CMakeLists.txt
add_library(core STATIC
    Card.cpp
    StandardCard.cpp
    BonusCard.cpp
    PenaltyCard.cpp
    Deck.cpp
    Player.cpp
    Game.cpp)

# PUBLIC so any target linking against core (gui, and later tests)
# automatically gets this directory on its include path - without
# this, gui's #include "Game.h" wouldn't resolve.
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


