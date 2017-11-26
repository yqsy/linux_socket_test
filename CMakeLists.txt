cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

project(linux_socket_test)

find_package(Boost REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR})

link_directories(${Boost_LIBRARY_DIRS})

find_library(BOOSTPO_LIBRARY NAMES boost_program_options)
find_library(EV_LIBRARY NAMES ev)

message(${BOOSTPO_LIBRARY})
message(${EV_LIBRARY})

add_subdirectory(ttcp)
