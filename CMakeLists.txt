cmake_minimum_required (VERSION 3.2)
project(elfdwt)

add_executable(elfdwt main.cpp)

target_compile_options(elfdwt PUBLIC -g)
target_compile_options(elfdwt PUBLIC -Og)