#include "elf.h"
#include "debug.h"
#include "system.h"
#include "mm.h"
#include "string.h"

#define ELF_MAGIC 0x7F
#define ELF_MAGIC_SIG_0 'E'
#define ELF_MAGIC_SIG_1 'L'
#define ELF_MAGIC_SIG_2 'F'

#define ELF_FILE_TYPE_EXEC 0x2

#define SHN_UNDEF 0
#define SHT_NOBITS 8

#include "../../usermode/brk/err.h"

uint8_t load_elf(char* filename, void **entry_point)
{
    uint8_t *file = (void*)&__src_dash;

    elf_header_t *header = (elf_header_t*)file;
    if (header->magic != ELF_MAGIC || header->signature[0] != ELF_MAGIC_SIG_0 || header->signature[1] != ELF_MAGIC_SIG_1
        || header->signature[2] != ELF_MAGIC_SIG_2)
    {
        debug("[elf] loaded file isn't in ELF format\n");
        return ELF_WRONG_FORMAT;
    }

    if (header->type != ELF_FILE_TYPE_EXEC)
    {
        debug("[elf] loaded file has invalid type\n");
        return ELF_WRONG_TYPE;
    }

    elf_section_t *section = (elf_section_t*)((uint32_t)header + header->section_table_position);
    for (uint16_t i = 0; i < header->section_header_entries_count; i++)
    {
        if (section->address != 0 && section->type != SHN_UNDEF)
        {
            //TODO: add section address validation, and restrict loading in kernel area
            //debug("[elf] loading section with type %i at %h\n", section->type, section->address);
            uint32_t start_page = section->address / 0x1000;
            uint32_t end_page = (section->address + section->size) / 0x1000;
            do
            {
                if (get_physical_address(start_page * 0x1000) == 0)
                {
                    uint32_t phys = alloc_physical_page();
                    map_virtual_to_physical(start_page * 0x1000, phys);
                }
                start_page++;
            }
            while(start_page <= end_page);

            if (section->type == SHT_NOBITS)
            {
                memset((void*)section->address, 0, section->size);
            }
            else
            {
                memcpy((void*)section->address, (void*)header + section->offset, section->size);
            }
        }
        section++;
    }

    *entry_point = (void*)(header->entry_point);

    return ELF_SUCCESS;
}
