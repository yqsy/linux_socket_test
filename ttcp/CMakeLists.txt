add_executable(ttcp_blocking ttcp_blocking.cpp common.cpp)
target_link_libraries(ttcp_blocking boost_program_options)

add_executable(ttcp_libev ttcp_libev.cpp common.cpp 
    ${PROJECT_BINARY_DIR}/tbuf/tbuf.cpp)

target_link_libraries(ttcp_libev boost_program_options ev)
