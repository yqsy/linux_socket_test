add_executable(ttcp_test ttcp_test.cpp common.cpp)
target_link_libraries(ttcp_test boost_program_options)

add_executable(ttcp_libev_test ttcp_libev_test.cpp common.cpp)
target_link_libraries(ttcp_libev_test boost_program_options ev)
