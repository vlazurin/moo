#ifndef H_ATA
#define H_ATA

#include "pci.h"

#define ATA_PRIMARY 0
#define ATA_SECONDARY 1

#define ATA_PRIMARY_IO 0x1F0
#define ATA_SECONDARY_IO 0x170

#define ATA_PORT_DATA 0
#define ATA_PORT_ERROR 1
#define ATA_PORT_SECTORS 2
#define ATA_PORT_LBA_0 3
#define ATA_PORT_LBA_1 4
#define ATA_PORT_LBA_2 5
#define ATA_PORT_DRIVE 6
#define ATA_PORT_LBA_3 6
#define ATA_PORT_COMMAND 7

#define ATA_MASTER 0
#define ATA_SLAVE 1

#define ATA_MASTER_ID 0xA0
#define ATA_SLAVE_ID 0xB0

struct ide_channel {
   uint16_t base;
   uint16_t ctrl;
};

void init_ata(pci_device_t *pci_device);

#endif
