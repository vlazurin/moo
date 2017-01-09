#include "ata.h"
#include "system.h"
#include "vfs.h"
#include "port.h"
#include "liballoc.h"
#include "string.h"
#include "debug.h"

static void ide_delay(struct ide_channel *channel)
{
    inb(channel->base + ATA_REG_ALTSTATUS);
    inb(channel->base + ATA_REG_ALTSTATUS);
    inb(channel->base + ATA_REG_ALTSTATUS);
    inb(channel->base + ATA_REG_ALTSTATUS);
}

static void reset_ata(struct ide_channel *channel)
{
    outb(channel->ctrl, 0x04);
    ide_delay(channel);
    outb(channel->ctrl, 0x0);
}

static uint8_t get_ata_status(struct ide_channel *channel)
{
    uint8_t status = 0;
    uint32_t tries = 10000;
    do {
        status = inb(channel->base + ATA_REG_STATUS);
        tries--;
    } while((status & ATA_SR_BSY) && tries > 0);
    return status;
}

// slow and simple lba28 / PIO reading
static int read_ata_device(struct ata_device *dev, void *buf, uint16_t lba, uint8_t sectors)
{
    if (lba + sectors >= dev->identify.sectors_28) {
        debug("LBA >= disk sectors count");
        return -1;
    }

    mutex_lock(&dev->channel->mutex);
    outb(dev->channel->base + ATA_REG_FEATURES, 0x00);
    outb(dev->channel->base + ATA_REG_SECCOUNT0, sectors);
    outb(dev->channel->base + ATA_REG_LBA0, (lba & 0x00000FF) >> 0);
    outb(dev->channel->base + ATA_REG_LBA1, (lba & 0x000FF00) >> 8);
    outb(dev->channel->base + ATA_REG_LBA2, (lba & 0x0FF0000) >> 16);

    outb(dev->channel->base + ATA_REG_HDDEVSEL, 0xE0 | (dev->slave << 4) | ((lba & 0xF000000) >> 24));
    outb(dev->channel->base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    uint16_t *buf_w = buf;
    while(sectors > 0) {
        while (!(get_ata_status(dev->channel) & ATA_SR_DRQ));
        for (uint16_t i = 0; i < SECTOR_SIZE / 2; i++) {
            *buf_w++ = inw(dev->channel->base);
        }
        sectors--;
    }
    mutex_release(&dev->channel->mutex);
    return 0;
}

// slow and simple lba28 / PIO  writing
static int write_ata_device(struct ata_device *dev, void *buf, uint16_t lba, uint8_t sectors)
{
    if (lba + sectors >= dev->identify.sectors_28) {
        debug("LBA >= disk sectors count");
        return -1;
    }

    mutex_lock(&dev->channel->mutex);
    outb(dev->channel->base + ATA_REG_FEATURES, 0x00);
    outb(dev->channel->base + ATA_REG_SECCOUNT0, sectors);
    outb(dev->channel->base + ATA_REG_LBA0, (lba & 0x00000FF) >> 0);
    outb(dev->channel->base + ATA_REG_LBA1, (lba & 0x000FF00) >> 8);
    outb(dev->channel->base + ATA_REG_LBA2, (lba & 0x0FF0000) >> 16);

    outb(dev->channel->base + ATA_REG_HDDEVSEL, 0xE0 | (dev->slave << 4) | ((lba & 0xF000000) >> 24));
    outb(dev->channel->base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    ide_delay(dev->channel);

    uint16_t *buf_w = buf;
    for (uint16_t i = 0; i < 256 * sectors; i++) {
        outw(dev->channel->base, *buf_w++);
    }
    outb(dev->channel->base + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    ide_delay(dev->channel);
    mutex_release(&dev->channel->mutex);
    return 0;
}

static int read(vfs_file_t *file, void *buf, uint32_t size, uint32_t *offset)
{
    uint32_t total = ALIGN(size, SECTOR_SIZE);
    if (total / SECTOR_SIZE > 256) {
        debug("can't read more than 256 sectors\n");
        return 0;
    }

    uint8_t *tmp = kmalloc(total);
    int err = read_ata_device(file->node->obj, tmp, ALIGN(*offset, SECTOR_SIZE) / SECTOR_SIZE, total);
    if (err) {
        kfree(tmp);
        return 0;
    }
    memcpy(buf, tmp, size);
    kfree(tmp);
    return size;
}

static int write(vfs_file_t *file, void *buf, uint32_t size, uint32_t *offset)
{
    uint32_t total = ALIGN(size, SECTOR_SIZE);
    if (total / SECTOR_SIZE > 256) {
        debug("can't write more than 256 sectors\n");
        return 0;
    }

    uint8_t *tmp = kmalloc(total);
    memcpy(tmp, buf, size);
    int err = write_ata_device(file->node->obj, tmp, ALIGN(*offset, SECTOR_SIZE) / SECTOR_SIZE, total);
    kfree(tmp);
    if (err) {
        return 0;
    }
    return size;
}

static vfs_file_operations_t ata_file_ops = {
    .open = NULL,
    .close = NULL,
    .write = &write,
    .read = &read
};

static void init_pata_drive(struct ide_channel *channel, uint8_t slave)
{
    static uint8_t next_hda = 0;

    outb(channel->base + ATA_REG_HDDEVSEL, ATA_MASTER_ID | (slave << 4));
    ide_delay(channel);
    outb(channel->base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ide_delay(channel);

    struct ata_device *dev = kmalloc(sizeof(struct ata_device));
    memset(dev, 0, sizeof(struct ata_device));
    dev->slave = slave;
    dev->channel = channel;
    dev->read = &read_ata_device;
    dev->write = &write_ata_device;

    channel->devices[slave] = dev;

    uint16_t *buf = (uint16_t*)&dev->identify;
    for(uint32_t i = 0; i < 256; i++) {
        *buf++ = inw(channel->base);
    }

    char name[20];
    sprintf(name, "/dev/hda%i", __sync_fetch_and_add(&next_hda, 1));
    create_vfs_node(name, S_IFBLK, &ata_file_ops, dev, 0);
}

void init_ata(pci_device_t *pci_device)
{
    struct ide_controller *ide = kmalloc(sizeof(struct ide_controller));
    memset(ide, 0, sizeof(struct ide_controller));
    ide->channels[0].base = (pci_device->base_address[0] & 0xFFFFFFFC) + 0x1F0 * (!pci_device->base_address[0]);
    ide->channels[0].ctrl = (pci_device->base_address[1] & 0xFFFFFFFC) + 0x3F6 * (!pci_device->base_address[1]);
    ide->channels[1].base = (pci_device->base_address[2] & 0xFFFFFFFC) + 0x170 * (!pci_device->base_address[2]);
    ide->channels[1].ctrl = (pci_device->base_address[3] & 0xFFFFFFFC) + 0x376 * (!pci_device->base_address[3]);

    // disable interrupts
    outb(ide->channels[0].ctrl + ATA_REG_CONTROL, 2);
    outb(ide->channels[1].ctrl + ATA_REG_CONTROL, 2);

    for(uint8_t c = 0; c < 2; c++) {
        for(uint8_t d = 0; d < 2; d++) {
            reset_ata(&ide->channels[c]);
            outb(ide->channels[c].base + ATA_REG_HDDEVSEL, ATA_MASTER_ID | (d << 4));
            ide_delay(&ide->channels[c]);

            outb(ide->channels[c].base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            ide_delay(&ide->channels[c]);

            // wait if device is busy
            uint8_t status = get_ata_status(&ide->channels[c]);
            if (status == 0) {
                continue;
            }

            uint8_t cl = inb(ide->channels[c].base + ATA_REG_LBA1);
            uint8_t ch = inb(ide->channels[c].base + ATA_REG_LBA2);
            if (cl == 0xFF && ch == 0xFF) {
                continue;
            } else if ((cl == 0x00 && ch == 0x00) || (cl == 0x3C && ch == 0xC3)) {
                // Parallel ATA / emulated SATA
                init_pata_drive(&ide->channels[c], d);
            }
        }
    }

    pci_device->hardware_driver = ide;
}
