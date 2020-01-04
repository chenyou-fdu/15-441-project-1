cmake_minimum_required(VERSION 3.15)
project(http_server)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(parser_lib)
add_executable(http_server main.cpp)
target_link_libraries(http_server parser_lib)

# Ref:
# https://stackoverflow.com/questions/2491421/cmake-command-syntax-question
# https://github.com/yedf/handy/blob/master/raw-examples/epoll.cc