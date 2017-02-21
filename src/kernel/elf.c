#include "elf.h"
#include "liballoc.h"
#include "vfs.h"
#include "log.h"
#include "system.h"
#include "mm.h"
#include "string.h"
#include "errno.h"

#define ELF_MAGIC 0x7F
#define ELF_MAGIC_SIG "ELF"

#define ELF_FILE_TYPE_EXEC 0x2

#define SHN_UNDEF 0
#define SHT_NOBITS 8

int check_elf(char *filename)
{
    log(KERN_DEBUG, "check_elf called (\"%s\")\n", filename);
    struct stat st;
    int err = stat_fs(filename, &st);
    if (err) {
        log(KERN_DEBUG, "check_elf error (%i)\n", err);
        return err;
    }
    if ((st.st_mode & S_IFREG) != S_IFREG) {
        log(KERN_DEBUG, "check_elf error (%i)\n", -ENOEXEC);
        return -ENOEXEC;
    }

    char sig[4];
    file_descriptor_t fd = sys_open(filename, 0);
    if (fd < 0) {
        return fd;
    }
    err = sys_read(fd, sig, 4);
    sys_close(fd);
    if (err < 0) {
        log(KERN_DEBUG, "check_elf error (%i)\n", err);
        return err;
    }

    if (sig[0] != ELF_MAGIC || strncmp(&sig[1], ELF_MAGIC_SIG, 3) != 0) {
        log(KERN_DEBUG, "check_elf failed (ELF file signature is wrong)\n");
        return -ENOEXEC;
    }

    return 0;
}

int load_elf(char* filename, void **entry_point)
{
    struct stat st;
    int err = stat_fs(filename, &st);
    if (err) {
        return err;
    }
    uint8_t *file = kmalloc(st.st_size);
    file_descriptor_t fd = sys_open(filename, 0);
    if (fd < 0) {
        return err;
    }
    err = sys_read(fd, file, st.st_size);
    if (err < 0) {
        return err;
    }
    sys_close(fd);

    elf_header_t *header = (elf_header_t*)file;
    if (header->magic != ELF_MAGIC || strncmp((void*)&header->signature, ELF_MAGIC_SIG, 3) != 0) {
        log(KERN_INFO, "file isn't in ELF format\n");
        kfree(file);
        return -ENOEXEC;
    }

    if (header->type != ELF_FILE_TYPE_EXEC) {
        log(KERN_INFO, "ELF file has wrong format\n");
        kfree(file);
        return -ENOEXEC;
    }

    elf_section_t *section = (elf_section_t*)((uint32_t)header + header->section_table_position);
    for (uint16_t i = 0; i < header->section_header_entries_count; i++) {
        if (section->address != 0 && section->type != SHN_UNDEF) {
            //TODO: add section address validation, and restrict loading in kernel area
            log(KERN_DEBUG, "loading ELF file section (%i %x %x)\n", section->type, section->address, section->size);
            uint32_t start_page = section->address / 0x1000;
            uint32_t end_page = (section->address + section->size) / 0x1000;
            do {
                if (get_physical_address(start_page * 0x1000) == 0) {
                    uint32_t phys = alloc_physical_page();
                    map_virtual_to_physical(start_page * 0x1000, phys);
                }
                start_page++;
            }
            while(start_page <= end_page);

            if (section->type == SHT_NOBITS) {
                memset((void*)section->address, 0, section->size);
            } else {
                memcpy((void*)section->address, (void*)header + section->offset, section->size);
            }
        }
        section++;
    }

    *entry_point = (void*)(header->entry_point);
    kfree(file);
    return 0;
}
