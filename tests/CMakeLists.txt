set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})

file(GLOB TEST_FILES LIST_DIRECTORIES false ${CMAKE_CURRENT_LIST_DIR} "*.cpp")
foreach (TEST_FILE ${TEST_FILES})
    cmake_path(GET TEST_FILE STEM TEST_NAME)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} ${Boost_LIBRARIES})
endforeach ()

target_compile_options(integer PUBLIC -ftrapv)