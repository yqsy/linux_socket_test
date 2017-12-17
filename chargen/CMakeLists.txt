add_executable(chargen chargen.cpp main.cpp)
target_link_libraries(chargen muduo_base_cpp11 muduo_net_cpp11 pthread)
