cmake_minimum_required(VERSION 3.0.2)

project(PrimoridalParticlesDemo)

# ZeroMQ setup
add_subdirectory(VirtualSpaceMeshnet)

# C++ setup
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow")

add_executable(sim_node
  src/sim_node.cpp
  src/zmq_http_server.cpp
)
target_link_libraries(sim_node PUBLIC vsm)
target_include_directories(sim_node PUBLIC include)
