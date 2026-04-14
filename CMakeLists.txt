cmake_minimum_required(VERSION 4.0)
project(cpp_utils)

set(CMAKE_CXX_STANDARD 23)

include_directories(.)
add_subdirectory(./tests)

file(GLOB_RECURSE HEADERS "*.hpp")
add_library(cpp_utils_lib INTERFACE)
target_sources(cpp_utils_lib INTERFACE ${HEADERS})