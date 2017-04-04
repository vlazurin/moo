cmake_minimum_required(VERSION 3.2)

set(CMAKE_ASM_FLAGS "-m32 -march=i686 -ffreestanding -Wall -Werror")
set(CMAKE_C_FLAGS "-O0 -m32 -march=i686 -ffreestanding -Wall -fno-common -Werror -DRUN_TESTS -DDEBUG")
set(CMAKE_EXE_LINKER_FLAGS "-T${CMAKE_SOURCE_DIR}/kernel.ld -nostdlib -Wl,-Map=mapfile")

include_directories(../shared/include)
include_directories(./include)

project(kernel C ASM)

set(SOURCE_FILES
        head.s
        main.c
        mm.c
        liballoc.c
        mutex.c
        pit.c
        gdt.c
        gdt.S
        task.c
        log.c
        timer.c
        io.c
        shm.c
        irq.c
        irq.S
        ./support/list.c
        ./support/ring.c
        ./support/buffer.c
        pci.c
        task.S
        elf.c
        libc.c
        process.c
        syscalls.c
        rand.c
        ./fs/tty.c
        ./fs/vfs.c
        ./fs/tempfs.c
        ./fs/fat16fs.c
        ./fs/procfs.c
        ./network/network.c
        ./network/arp.c
        ./network/ip.c
        ./network/udp.c
        ./network/dhcp.c
        ./network/tcp.c
        ./drivers/char/urandom.c
        ./drivers/char/kb.c
        ./drivers/char/screen.c
        ./drivers/char/serial.c
        ./drivers/block/ata.c
        ./drivers/char/null.c
        ./drivers/video/vesa/vesa_lfb.c
        ./drivers/network/e1000/e1000.c
        ../shared/sources/system.c
        ../shared/sources/string.c
        ../shared/sources/port.c
        ../shared/sources/stdlib.c
        ./tests/tests.c)
add_executable(kernel ${SOURCE_FILES})

add_custom_command(TARGET kernel POST_BUILD COMMAND bash ${CMAKE_SOURCE_DIR}/../make_image.sh)
