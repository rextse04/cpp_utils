cmake_minimum_required(VERSION 4.0)
project(cpp_utils)

set(CMAKE_CXX_STANDARD 26)

add_subdirectory(./tests)
add_subdirectory(./examples)

file(GLOB_RECURSE HEADERS "*.hpp")
add_library(cpp_utils_lib INTERFACE ${HEADERS})
target_include_directories(cpp_utils_lib INTERFACE .)