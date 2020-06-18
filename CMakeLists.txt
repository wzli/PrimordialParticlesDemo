cmake_minimum_required(VERSION 3.0.2)

project(PrimoridalParticlesDemo)

# requires boost
find_package(Boost REQUIRED COMPONENTS iostreams)

# VSM setup
add_subdirectory(VirtualSpaceMeshnet)

# C++ setup
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow")

add_executable(sim_node
  src/sim_node.cpp
  src/display.cpp
  src/particles.cpp
  src/zmq_http_server.cpp
)
target_link_libraries(sim_node PUBLIC vsm ${Boost_LIBRARIES})
target_include_directories(sim_node PUBLIC include ${Boost_INCLUDE_DIRS})
