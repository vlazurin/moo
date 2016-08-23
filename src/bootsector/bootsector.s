.code16gcc

# we have 3 bytes before FAT16 header
jmp start
nop

.long   0                   # OEM Name
.long   0                   # OEM Name
bytes_per_sector:
.word   0                   # Bytes per sector
sectors_per_cluster:
.byte   0                   # Sectors Per Cluster
reserved_sectors_count:
.word   0                   # Reserved Sectors
fat_tables_count:
.byte   0                   # Number of Copies of FAT
root_dir_max_entries:
.word   0                   # Maximum Root Directory Entries (N/A for FAT32)
.word   0                   # Number of Sectors in Partition Smaller than 32MB (N/A for FAT32)
.byte   0                   # Media Descriptor (F8h for Hard Disks)
fat_size:
.word   0                   # Sectors Per FAT in Older FAT Systems (N/A for FAT32)
.word   0                   # Sectors Per Track
.word   0                   # Number of Heads
.long   0                   # Number of Hidden Sectors in Partition
.long   0                   # Number of Sectors in Partition
.byte   0                   # Logical Drive Number of Partition
.byte   0                   # Unused
.byte   0                   # Extended Signature (29h)
.long   0                   # Serial Number of Partition
.long   0                   # Volume Name of Partition
.long   0                   # Volume Name of Partition
.word   0                   # Volume Name of Partition
.byte   0                   # Volume Name of Partition
.long   0                   # FAT Name
.long   0                   # FAT Name

# all variables must be after code because we are doing short jmp on "start" label

start:
    # reset registers & set stack to 0x7C00
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $0x7C00, %ax
    mov %ax, %sp

    # save boot drive number
    movb %dl, drive_number

    # clear the screen (via scroll window BIOS function)
    xor %al, %al
    xor %bh, %bh
    mov $0x06, %ah
    xor %cx, %cx
    mov $25, %dh
    mov $80, %dl
    int $0x10

    # load FAT table at 128kb, it can take up to 128kb (2 segments)
    movw reserved_sectors_count, %ax
    movw $0x0000, load_offset
    movw $0x2000, load_offset_seg
    mov %ax, lba_start_number
    movw fat_size, %ax
    movw %ax, sectors_count
    movw $disk_address_structure, %si
    mov $0x42, %ah
    movb drive_number, %dl
    int $0x13
    jc lba_error

    # load root directory at 0x0500, it's lowest available for use memory
    movw $0x0500, load_offset
    movw $0x0000, load_offset_seg
    movw root_dir_max_entries, %ax
    # 32 - root directory entry size
    mov $32, %cx
    mul %cx
    addw bytes_per_sector, %ax
    decw %ax
    cwd
    mov bytes_per_sector, %cx
    divw %cx
    movw %ax, sectors_count

    movb fat_tables_count, %al
    movw fat_size, %cx
    mul  %cx
    addw reserved_sectors_count, %ax
    addw sectors_count, %ax
    movw %ax, first_data_sector

    subw sectors_count, %ax
    movw %ax, lba_start_number

    movw $disk_address_structure, %si
    mov $0x42, %ah
    movb drive_number, %dl
    int $0x13
    jc lba_error

    # search for second stage file
    mov load_offset, %ax
check_file:
    mov %ax, %di
    # free entry
    cmpb $0xe5, (%di)
    je try_next_file
    # not found, end of directory
    cmpb $0x0, (%di)
    je file_not_found

    mov $file_name_length, %cx
    mov $file_name, %si
    repe cmpsb
    je init_load
try_next_file:
    add $32, %ax
    jmp check_file

file_not_found:
    mov $not_found_message, %bp
    mov $not_found_message_length, %cx
    call print_string
    cli
    hlt

init_load:
    # cluster number located at bytes 26-27. %di points on the attribute byte (11) now
    add $15, %di
    mov (%di), %ax
    movw $0, sectors_count
    movw $0x7e00, load_offset
    movw $0x0000, load_offset_seg
load_file:
    pushw %ax
    sub $2, %al
    mulb sectors_per_cluster
    add first_data_sector, %ax
    mov %ax, lba_start_number
    xor %ax,%ax
    movb sectors_per_cluster, %al
    movb %al, sectors_count
    mov $disk_address_structure, %si
    mov $0x42, %ah
    movb drive_number, %dl
    int $0x13
    jc lba_error

    # search for next cluster
    # next cluster number can be located in one of two FAT segments, some math required
    # FILE CANT BE BIGGER THAN ONE MEMORY SEGMENT...
    popw %ax
    movw $0x2000, %dx
    movw %ax, %cx
    add %cx, %cx
    jnc first_half
    addw $0x1000, %dx
first_half:
    push %es
    movw %dx, %es

    movw $2, %cx
    mulw %cx
    movw %ax, %di
    movw %es:(%di), %ax
    popw %es

    # if value in FAT is 0xFFF8 or bigger file has no more clusters
    cmp $0xFFF8, %ax
    je goto_next_stage
    ja goto_next_stage
    pushw %ax
    xor %cx, %cx
    movb sectors_per_cluster, %cl
    movw bytes_per_sector, %ax
    mulw %cx
    # loading next cluster of file
    movw load_offset, %cx
    addw %cx, %ax
    movw %ax, load_offset
    popw %ax
    jmp load_file

goto_next_stage:
    jmp 0x7e00

print_string:
    mov $0x13, %ah                      # function 13 - write string
    mov $0x01, %al                      # attribute in al: move cursor
    xor %bh,%bh                         # video page 0
    mov $7, %bl                         # attribute - gray color
    mov current_line, %dh               # row to put string
    mov $0, %dl                         # column to put string
    int $0x10
    inc %dh
    movb %dh, current_line
    ret

lba_error:
    mov $lba_error_message, %bp
    mov $lba_error_message_length, %cx
    call print_string
    cli
    hlt

disk_address_structure:
.byte 0x10
.byte 0
sectors_count:
.word 0
load_offset:
.word 0x0000
load_offset_seg:
.word 0x0000
lba_start_number:
.long 0
.long 0

first_data_sector:
.word   0

current_line:
.byte 0x0

file_name:
.ascii "LOADER     "
file_name_length = . -file_name

lba_error_message:
.ascii "[bootsector] Can't load file..."
lba_error_message_length = . -lba_error_message

drive_number:
.byte 0

not_found_message:
.ascii "Loader not found!"
not_found_message_length = . -not_found_message
