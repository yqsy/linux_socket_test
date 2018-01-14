add_library(static_1 static_1.cpp)
add_library(static_2 static_2.cpp)
target_link_libraries(static_2 static_1)

add_executable(app app.cpp)
target_link_libraries(app static_2)