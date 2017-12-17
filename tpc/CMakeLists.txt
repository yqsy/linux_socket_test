add_executable(sender sender.cpp)
target_link_libraries(sender boost_system)

add_executable(netcat netcat.cpp)
target_link_libraries(netcat pthread boost_system)
