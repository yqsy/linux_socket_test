add_executable(chargen chargen.cpp main.cpp)
target_link_libraries(chargen muduo_base muduo_net pthread)
