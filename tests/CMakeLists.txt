set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(.)

file(GLOB_RECURSE TEST_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_LIST_DIR} "*.cpp")
foreach (TEST_FILE ${TEST_FILES})
    cmake_path(REMOVE_EXTENSION TEST_FILE OUTPUT_VARIABLE TEST_NAME)
    string(REPLACE "/" "_" TEST_NAME ${TEST_NAME}_test)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME}
            PRIVATE cpp_utils_lib
            PRIVATE ${Boost_LIBRARIES})
endforeach ()

target_compile_options(integer_test PUBLIC -ftrapv)