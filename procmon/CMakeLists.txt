add_executable(procmon procmon.cpp plot.cc)
target_link_libraries(procmon httplight Jinja2CppLight muduo_base muduo_net pthread gd)

add_executable(jinja_test tests/jinja_test.cpp)
target_link_libraries(jinja_test Jinja2CppLight muduo_base muduo_net pthread)

add_executable(circular_buf_test tests/circular_buf_test.cpp)
target_link_libraries(circular_buf_test)

add_executable(plot_test tests/plot_test.cpp)
target_link_libraries(plot_test gd)

add_executable(plot_test2 tests/plot_test2.cpp)
target_link_libraries(plot_test2 gd)