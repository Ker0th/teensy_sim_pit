# CMakeList.txt : CMake project for teensy_sim_pit, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.10)

project(teensy_sim_pit LANGUAGES CXX)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add source to this project's executable.
add_executable(teensy_sim_pit
  src/teensy_sim_pit.cpp
  hpp/teensy_sim_pit.h
  main.cpp)

target_include_directories(teensy_sim_pit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hpp)
