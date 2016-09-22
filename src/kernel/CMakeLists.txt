cmake_minimum_required(VERSION 3.2)

set(CMAKE_ASM_FLAGS "-m32 -march=i686 -ffreestanding -Wall -Werror")
set(CMAKE_C_FLAGS "-O0 -m32 -march=i686 -ffreestanding -Wall -fno-common -Werror -DDEBUG")
set(CMAKE_EXE_LINKER_FLAGS "-T${CMAKE_SOURCE_DIR}/kernel.ld -nostdlib -Wl,-Map=mapfile")

include_directories(../shared/include)
include_directories(./include)

project(kernel C ASM)

set(SOURCE_FILES
        head.s
        main.c
        interrupts.c
        mm.c
        liballoc.c
        mutex.c
        stb_truetype.c
        pit.c
        tasking.c
        process.s
        timer.c
        ./support/list.c
        pci.c
        ./network/network.c
        ./drivers/video/vesa/vesa_lfb.c
        ./drivers/network/e1000/e1000.c
        ../shared/sources/system.c
        ../shared/sources/debug.c
        ../shared/sources/string.c
        ../shared/sources/port.c
        ../shared/sources/stdlib.c
        ./tests/tests.c)
add_executable(kernel ${SOURCE_FILES})

add_custom_command(TARGET kernel POST_BUILD COMMAND bash ${CMAKE_SOURCE_DIR}/../make_image.sh)
