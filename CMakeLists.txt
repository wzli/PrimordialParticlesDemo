cmake_minimum_required(VERSION 3.0.2)

project(PrimoridalParticlesDemo)

# Boost setup
include(ExternalProject)
ExternalProject_Add(
    Boost
    URL https://dl.bintray.com/boostorg/release/1.73.0/source/boost_1_73_0.tar.gz
    BUILD_COMMAND ""
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
    )
ExternalProject_Get_Property(Boost install_dir)
set(Boost_INCLUDE_DIRS ${install_dir}/src/Boost)

# VSM setup
add_subdirectory(VirtualSpaceMeshnet)

# C++ setup
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

add_executable(sim_node
  src/sim_node.cpp
  src/zmq_http_server.cpp
)
target_link_libraries(sim_node PUBLIC vsm)
target_include_directories(sim_node PUBLIC include ${Boost_INCLUDE_DIRS})
