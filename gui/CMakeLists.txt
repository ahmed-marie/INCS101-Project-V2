# gui/CMakeLists.txt
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Qt's moc (meta-object compiler) needs to process every Q_OBJECT
# class - these three settings tell CMake to run it automatically
# for every file in this target, rather than needing a manual step.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(gui
    main.cpp
    MainWindow.h
    MainWindow.cpp
    StartScreen.h
    StartScreen.cpp
    GameScreen.h
    GameScreen.cpp
    EndScreen.h
    EndScreen.cpp
    CardButton.h
    CardButton.cpp
)

target_link_libraries(gui PRIVATE core Qt6::Widgets)
