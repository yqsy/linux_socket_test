add_executable(memcached server.cpp memcached_server.cpp item.cpp session.cpp)
target_link_libraries(memcached muduo_inspect muduo_http muduo_net muduo_base boost_program_options pthread rt)


#add_executable(test_memcached test.cpp item.cpp)
#target_link_libraries(test_memcached muduo_net muduo_base pthread)