add_executable(memcached server.cpp)
target_link_libraries(memcached muduo_inspect muduo_http muduo_net muduo_base pthread rt)