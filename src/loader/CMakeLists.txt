cmake_minimum_required(VERSION 3.2)

set(CMAKE_ASM_FLAGS "-m16 -march=i686 -ffreestanding -Wall")
set(CMAKE_C_FLAGS "-O0 -m16 -march=i686 -ffreestanding -Wall -Werror")
set(CMAKE_EXE_LINKER_FLAGS "-T${CMAKE_SOURCE_DIR}/loader.ld -nostdlib")

include_directories(../shared/include)
include_directories(./include)

project(loader C ASM)

set(SOURCE_FILES
        head.s
        main.c
        bios.c
        ../shared/sources/string.c
        ../shared/sources/system.c
        ../shared/sources/debug.c
        ../shared/sources/port.c
        ../shared/sources/stdlib.c
        lba.c
        fat16.c

)
add_executable(loader ${SOURCE_FILES})

add_custom_command(TARGET loader POST_BUILD COMMAND bash ${CMAKE_SOURCE_DIR}/../make_image.sh)
