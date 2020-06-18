cmake_minimum_required(VERSION 3.0.2)

project(PrimoridalParticlesDemo)

# Boost setup
set(BOOST_ROOT_DIR ${CMAKE_BINARY_DIR})
include(ExternalProject)
ExternalProject_Add(
    Boost
    URL https://dl.bintray.com/boostorg/release/1.73.0/source/boost_1_73_0.tar.gz
    CONFIGURE_COMMAND ./bootstrap.sh --prefix=${BOOST_ROOT_DIR} --with-libraries=iostreams
    BUILD_COMMAND ./b2 --prefix=${BOOST_ROOT_DIR} --with-iostreams install
    BUILD_IN_SOURCE true
    INSTALL_COMMAND ""
    )
ExternalProject_Get_Property(Boost install_dir)
set(Boost_INCLUDE_DIRS ${install_dir}/src/Boost)

# VSM setup
add_subdirectory(VirtualSpaceMeshnet)

# C++ setup
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

add_executable(sim_node
  src/sim_node.cpp
  src/particles.cpp
  src/zmq_http_server.cpp
)
target_link_libraries(sim_node PUBLIC vsm boost_iostreams)
target_include_directories(sim_node PUBLIC include ${Boost_INCLUDE_DIRS})
