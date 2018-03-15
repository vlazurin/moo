cmake_minimum_required(VERSION 3.2)

project(test_gui C)
set(CMAKE_C_COMPILER "i386-pc-moo-gcc")
set(CMAKE_CXX_COMPILER "i386-pc-moo-gcc")

set(CMAKE_C_FLAGS "-std=c99 -static")
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

set(SOURCE_FILES
        test_gui.c
)
add_executable(test_gui ${SOURCE_FILES})

add_custom_command(TARGET test_gui POST_BUILD COMMAND cp test_gui ${CMAKE_SOURCE_DIR}/../hdd/bin/test_gui)
add_custom_command(TARGET test_gui POST_BUILD COMMAND bash ${CMAKE_SOURCE_DIR}/../make_image.sh)
