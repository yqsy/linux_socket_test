add_executable(ttcp_blocking ttcp_blocking.cpp common.cpp)
target_link_libraries(ttcp_blocking boost_program_options)

#add_executable(ttcp_libev_test ttcp_libev_test.cpp common.cpp)
#target_link_libraries(ttcp_libev_test boost_program_options ev)
