cmake_minimum_required(VERSION 3.2)

set(CMAKE_ASM_FLAGS "-march=i686 -ffreestanding -Wall -m16")
set(CMAKE_EXE_LINKER_FLAGS "-T${CMAKE_SOURCE_DIR}/bootsector.ld -nostdlib")
project(bootsector ASM)

set(SOURCE_FILES bootsector.s)
add_executable(bootsector ${SOURCE_FILES})

add_custom_command(TARGET bootsector POST_BUILD COMMAND bash ${CMAKE_SOURCE_DIR}/../make_image.sh)
