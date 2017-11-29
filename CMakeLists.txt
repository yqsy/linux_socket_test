cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -Wextra -Wno-unused-parameter")

project(linux_socket_test)

find_package(Boost REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR})

link_directories(${Boost_LIBRARY_DIRS})

find_library(BOOSTPO_LIBRARY NAMES boost_program_options)
find_library(EV_LIBRARY NAMES ev)
find_library(EVENT_LIBRARY NAMES event)

message("${BOOSTPO_LIBRARY}")
message("${EV_LIBRARY}")
message("${EVENT_LIBRARY}")

add_subdirectory(ttcp)
