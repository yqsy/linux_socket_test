cmake_minimum_required(VERSION 2.8)

project(linux_socket_test)


if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT MUDUO_PATH)
  set(MUDUO_PATH "/root/reference/package/build/release-install")
endif()


set(CXX_FLAGS
 -std=c++11
 -g
 -Wall
 -Wextra
 -Wno-unused-parameter
)

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

find_path(Muduo_INCLUDE_DIR muduo "${MUDUO_PATH}/include")
find_path(Muduo_LIBRARY_DIR libmuduo_net.a "${MUDUO_PATH}/lib")
link_directories(${Muduo_LIBRARY_DIR})

message(STATUS ${Muduo_INCLUDE_DIR})
message(STATUS ${Muduo_LIBRARY_DIR})

include_directories(${Muduo_INCLUDE_DIR})
find_library(muduo_base muduo_base)
find_library(muduo_net muduo_net)
message(STATUS ${muduo_base})
message(STATUS ${muduo_net})

find_package(Boost REQUIRED)
find_library(BOOSTPO_LIBRARY NAMES boost_program_options)
find_library(EV_LIBRARY NAMES ev)
find_library(EVENT_LIBRARY NAMES event)
find_library(PTHREAD_LIBRARY LIBRARY NAMES pthread)
find_library(BOOSTTEST_LIBRARY NAMES boost_unit_test_framework)
find_library(BOOST_SYSTEN NAMES boost_system)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR})

link_directories(${Boost_LIBRARY_DIRS})

message("${BOOSTPO_LIBRARY}")
message("${BOOSTTEST_LIBRARY}")
message("${EV_LIBRARY}")
message("${EVENT_LIBRARY}")
message("${PTHREAD_LIBRARY}")
message("${BOOST_SYSTEN}")

message ("${CMAKE_BUILD_TYPE}")
string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

add_subdirectory(http)
#add_subdirectory(tpc)
#add_subdirectory(chargen)
#add_subdirectory(ttcp)
#add_subdirectory(tbuf)
#add_subdirectory(circular_buffer)

#add_subdirectory(ptrtest)
#add_subdirectory(gprof_test)
