cmake_minimum_required(VERSION 3.20)
project(INCS101ProjectV2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# run the following comma"core/main.cpp"test
# create a sandbox folder and add the main.cpp file to it
# add_executable(smoketest sandbox/main.cpp)

# or just add the main.cpp inside your test-target folder e.g. core folder
# add_executable(smoketest2 core/main.cpp)