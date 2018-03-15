cmake_minimum_required(VERSION 3.2)

project(desk C)
set(CMAKE_C_COMPILER "i386-pc-moo-gcc")
set(CMAKE_CXX_COMPILER "i386-pc-moo-gcc")

set(CMAKE_C_FLAGS "-std=c99 -static")
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-lcairo -lpixman-1 -lm -lfreetype -lpng15 -lz")

include_directories(../library/include)
include_directories(../../kernel/include)

set(SOURCE_FILES
        main.c
        ../library/gui.c
        ../library/text.c
        ../library/drawing.c
)
add_executable(desk ${SOURCE_FILES})

add_custom_command(TARGET desk POST_BUILD COMMAND cp desk ${CMAKE_SOURCE_DIR}/../hdd/bin/desk)
add_custom_command(TARGET desk POST_BUILD COMMAND bash ${CMAKE_SOURCE_DIR}/../make_image.sh)
