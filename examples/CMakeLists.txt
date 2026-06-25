include_directories(..)

file(GLOB EXAMPLES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
foreach(DIR ${EXAMPLES})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${DIR})
        add_subdirectory(${DIR})
    endif()
endforeach()