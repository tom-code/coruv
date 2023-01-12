
cmake_minimum_required(VERSION 3.20)
project(coruv)

include_directories(/usr/local/include)



set (CMAKE_CXX_FLAGS "-g -std=c++20 -Wall -pedantic -fsanitize=address")
set (CMAKE_EXE_LINKER_FLAGS "-L /usr/local/lib")


add_executable(coruv srv.cc c3.cc)
target_link_libraries(coruv uv)
