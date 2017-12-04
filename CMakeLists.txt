cmake_minimum_required(VERSION 2.8)

project(linux_socket_test)


if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CXX_FLAGS
 -std=c++11
 -g
 -Wall
 -Wextra
 -Wno-unused-parameter
)

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -finline-limit=1000 -DNDEBUG")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)


find_package(Boost REQUIRED)
find_library(BOOSTPO_LIBRARY NAMES boost_program_options)
find_library(EV_LIBRARY NAMES ev)
find_library(EVENT_LIBRARY NAMES event)
find_library(PTHREAD_LIBRARY LIBRARY NAMES pthread)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR})

link_directories(${Boost_LIBRARY_DIRS})


message("${BOOSTPO_LIBRARY}")
message("${EV_LIBRARY}")
message("${EVENT_LIBRARY}")
message("${PTHREAD_LIBRARY}")

message ("${CMAKE_BUILD_TYPE}")
string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

add_subdirectory(ttcp)
add_subdirectory(ptrtest)
add_subdirectory(gprof_test)
