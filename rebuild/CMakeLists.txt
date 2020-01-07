cmake_minimum_required(VERSION 3.15)
project(rebuild)

set(CMAKE_CXX_STANDARD 14)

add_executable(rebuild main.cpp IO_operation.cpp IO_operation.h table.h float_coding.cpp float_coding.h loser_tree.cpp loser_tree.h table.cpp)