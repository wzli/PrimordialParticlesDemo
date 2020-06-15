cmake_minimum_required(VERSION 3.0.2)

project(PrimoridalParticlesDemo)

# ZeroMQ setup
add_subdirectory(VirtualSpaceMeshnet)

# C++ setup
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow")

# add executable
add_library(main
  src/main.cpp
)
target_link_libraries(main PUBLIC vsm)
target_include_directories(main PUBLIC include)
