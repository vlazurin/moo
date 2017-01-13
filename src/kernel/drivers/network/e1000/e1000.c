#include <stdint.h>
#include <stdbool.h>
#include "pci.h"
#include "task.h"
#include "liballoc.h"
#include "mmio.h"
#include "mm.h"
#include "timer.h"
#include "network.h"
#include "string.h"
#include "system.h"
#include "debug.h"

// must be multiple of 8
// 256 because it will take exactly one page (descriptor size is 16 bytes)
#define RX_DESCRIPTORS_COUNT 256
#define TX_DESCRIPTORS_COUNT 256

#define DESCRIPTOR_BUFFER_SIZE 2048

#define E1000_REG_RDH   0x2810
#define E1000_REG_RDT   0x2818
#define E1000_REG_TCTL  0x0400
#define E1000_REG_TDBAL 0x3800
#define E1000_REG_TDBAH 0x3804
#define E1000_REG_TDLEN 0x3808
#define E1000_REG_TDH   0x3810
#define E1000_REG_TDT   0x3818
#define E1000_REG_CTRL  0x0000
#define E1000_REG_IMC   0x00d8
#define E1000_REG_MTA   0x5200
#define E1000_REG_RAL   0x5400
#define E1000_REG_RAH   0x5404
#define E1000_REG_CTRL_EXT 0x18
#define E1000_REG_RCTL  0x0100
#define E1000_REG_RDBAL 0x2800
#define E1000_REG_RDBAH 0x2804
#define E1000_REG_RDLEN 0x2808
#define E1000_REG_EERD  0x14

#define E1000_RCTL_EN   (1<<1)
#define E1000_RCTL_SBP  (1<<2)
#define E1000_RCTL_UPE  (1<<3)  /* Unicast promiscuous */
#define E1000_RCTL_MPE  (1<<4)  /* Multicast promiscuous */
#define E1000_RCTL_LPE  (1<<5)  /* Long packet reception */
#define E1000_RCTL_BAM  (1<<15) /* Broadcast accept mode */
#define E1000_RCTL_SECRC (1<<26) /* Strip ethernet CRC from incoming packet */
#define E1000_RCTL_BSIZE_2048 (0<<16) // default 2048b buffer
#define E1000_CTRL_RST  (1<<26)
#define E1000_CTRL_SLU  (1<<6)
#define E1000_CTRL_EXT_LINK_MODE_MASK (uint32_t)(3ULL<<22)

#define E1000_TCTL_EN   (1<<1)
#define E1000_TCTL_PSP  (1<<3) // pad short packets
#define E1000_TCTL_MULR (1<<28) // (Multiple Request Support) WTF? 2 days of debugging, WTF?

typedef struct e1000_rx_descriptor {
    uint64_t address;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed)) e1000_rx_descriptor_t;

typedef struct e1000_tx_descriptor
{
    uint64_t address;
    uint16_t length;
    uint8_t checksum_offset;
    uint8_t command;
    uint8_t status;
    uint8_t checksum_start;
    uint16_t special;
} __attribute__((packed)) e1000_tx_descriptor_t;

typedef struct e1000_device
{
    uint32_t mmio;
    e1000_rx_descriptor_t *rx_base;
    e1000_tx_descriptor_t *tx_base;
    uint32_t rx_tail;
    uint32_t tx_tail;
    void *rx_data_buffer;
    void *tx_data_buffer;
    mutex_t mutex;
    pci_device_t *pci_dev;
} e1000_device_t;

void rx_thread(e1000_device_t *dev)
{
    network_device_t *net_dev = (network_device_t*)dev->pci_dev->logical_driver;

    while(true)
    {
        if (!(dev->rx_base[dev->rx_tail].status & 1))
        {
            force_task_switch();
            continue;
        }

        // we got packet
        uint8_t drop = 0;
        if ((dev->rx_base[dev->rx_tail].status & (1 << 1)) == 0)
        {
            debug("[e1000] no End Of Packet flag. Status: %h, packet ignored.\n", dev->rx_base[dev->rx_tail].status);
            drop = 1;
        }
        else if (dev->rx_base[dev->rx_tail].length < 60)
        {
            debug("[e1000] packet length is %i, packet ignored.\n", dev->rx_base[dev->rx_tail].length);
            drop = 1;
        }
        else if (dev->rx_base[dev->rx_tail].errors)
        {
            debug("[e1000] packet error code is %i, packet ignored.\n", dev->rx_base[dev->rx_tail].errors);
            drop = 1;
        }

        if (drop == 0)
        {
            net_dev->receive_packet(net_dev, dev->rx_data_buffer + dev->rx_tail * DESCRIPTOR_BUFFER_SIZE);
        }

        dev->rx_base[dev->rx_tail].status = 0;
        dev->rx_tail = (dev->rx_tail + 1) % RX_DESCRIPTORS_COUNT;
        mmio_write32(dev->mmio, E1000_REG_RDT, dev->rx_tail);
    }
}

void e1000_setup_tx(e1000_device_t *dev)
{
    uint32_t desc_pages = PAGE_ALIGN(sizeof(e1000_tx_descriptor_t) * TX_DESCRIPTORS_COUNT) / 0x1000;
    // tx_buffer must be aligned on 16 byte boundary, also we need continuous chunk of memory
    // alloc_physical_range returns address aligned on page boundary
    uint32_t physical_addr = alloc_physical_range(desc_pages);
    uint32_t virtual_addr = alloc_hardware_space_chunk(desc_pages);
    map_virtual_to_physical_range(virtual_addr, physical_addr, desc_pages);

    dev->tx_base = (void*)virtual_addr;
    memset(dev->tx_base, 0, sizeof(e1000_tx_descriptor_t) * TX_DESCRIPTORS_COUNT);

    uint16_t pages_count = PAGE_ALIGN(DESCRIPTOR_BUFFER_SIZE * RX_DESCRIPTORS_COUNT) / 0x1000;
    dev->tx_data_buffer = (void*)alloc_hardware_space_chunk(pages_count);
    uint32_t phys_addr = alloc_physical_range(pages_count);
    map_virtual_to_physical_range((uint32_t)dev->tx_data_buffer, phys_addr, pages_count);
    memset(dev->tx_data_buffer, 0, DESCRIPTOR_BUFFER_SIZE * RX_DESCRIPTORS_COUNT);

    for (uint16_t i = 0; i < TX_DESCRIPTORS_COUNT; i++)
    {
        e1000_tx_descriptor_t *current = &(dev->tx_base[i]);
        current->address = get_physical_address((uint32_t)dev->tx_data_buffer + DESCRIPTOR_BUFFER_SIZE * i);
    }

    // must be (uint32_t)dev->tx_base >> 32, but we don't use 64bit mode
    mmio_write32(dev->mmio, E1000_REG_TDBAH, 0);
    // must contains "& 0xFFFFFFFF" but we don't use 64bit mode
    mmio_write32(dev->mmio, E1000_REG_TDBAL, get_physical_address((uint32_t)dev->tx_base));

    mmio_write32(dev->mmio, E1000_REG_TDLEN, (uint32_t)(TX_DESCRIPTORS_COUNT * sizeof(e1000_tx_descriptor_t)));
    mmio_write32(dev->mmio, E1000_REG_TDH, 0);
    mmio_write32(dev->mmio, E1000_REG_TDT, 0);
    dev->tx_tail = 0;

    mmio_write32(dev->mmio, E1000_REG_TCTL, E1000_TCTL_PSP | E1000_TCTL_MULR);
}

void e1000_enable(network_device_t *net_dev)
{
    e1000_device_t *dev = (e1000_device_t*)net_dev->pci_dev->hardware_driver;
    // enable tx first, so system is able to reply on incoming packets
    mmio_write32(dev->mmio, E1000_REG_TCTL, mmio_read32(dev->mmio, E1000_REG_TCTL) | E1000_TCTL_EN);
    mmio_write32(dev->mmio, E1000_REG_RCTL, mmio_read32(dev->mmio, E1000_REG_RCTL) | E1000_RCTL_EN);
}

uint16_t e1000_eeprom_read_8254x(uint32_t base, uint8_t eeprom_addr)
{
    // bit 0 - start read
    *(uint32_t*)(base + E1000_REG_EERD) = ((uint32_t)eeprom_addr << 8) | 1;

    uint32_t data = mmio_read32(base, E1000_REG_EERD);
    // 0 - read in progress
    while (!(data & (1<<4))) {
        force_task_switch();
        data = mmio_read32(base, E1000_REG_EERD);
    }

    // data is in last 16 bits
    return data >> 16;
}

void e1000_setup_rx(e1000_device_t *dev)
{
     uint32_t desc_pages = PAGE_ALIGN(sizeof(e1000_rx_descriptor_t) * RX_DESCRIPTORS_COUNT) / 0x1000;
    // rx_buffer must be aligned on 16 byte boundary, also we need continuous chunk of memory
    // alloc_physical_range returns address aligned on page boundary
    uint32_t physical_addr = alloc_physical_range(desc_pages);
    uint32_t virtual_addr = alloc_hardware_space_chunk(desc_pages);
    map_virtual_to_physical_range(virtual_addr, physical_addr, desc_pages);

    dev->rx_base = (void*)virtual_addr;
    memset(dev->rx_base, 0, sizeof(e1000_rx_descriptor_t) * RX_DESCRIPTORS_COUNT);

    uint16_t pages_count = PAGE_ALIGN(DESCRIPTOR_BUFFER_SIZE * RX_DESCRIPTORS_COUNT) / 0x1000;
    dev->rx_data_buffer = (void*)alloc_hardware_space_chunk(pages_count);
    uint32_t phys_addr = alloc_physical_range(pages_count);
    map_virtual_to_physical_range((uint32_t)dev->rx_data_buffer, phys_addr, pages_count);
    memset(dev->rx_data_buffer, 0, DESCRIPTOR_BUFFER_SIZE * RX_DESCRIPTORS_COUNT);

    for (uint16_t i = 0; i < RX_DESCRIPTORS_COUNT; i++)
    {
        e1000_rx_descriptor_t *current = &(dev->rx_base[i]);
        current->address = get_physical_address((uint32_t)dev->rx_data_buffer + DESCRIPTOR_BUFFER_SIZE * i);
    }

    // must be (uint32_t)dev->rx_base >> 32, but we don't use 64bit mode
    mmio_write32(dev->mmio, E1000_REG_RDBAH, 0);
    // must contains "& 0xFFFFFFFF" but we don't use 64bit mode
    mmio_write32(dev->mmio, E1000_REG_RDBAL, get_physical_address((uint32_t)dev->rx_base));
    mmio_write32(dev->mmio, E1000_REG_RDLEN, RX_DESCRIPTORS_COUNT * sizeof(e1000_rx_descriptor_t));
    mmio_write32(dev->mmio, E1000_REG_RDH, 0);
    mmio_write32(dev->mmio, E1000_REG_RDT, RX_DESCRIPTORS_COUNT - 1);
    mmio_write32(dev->mmio, E1000_REG_RCTL, mmio_read32(dev->mmio, E1000_REG_RCTL) | E1000_RCTL_SBP | E1000_RCTL_UPE
                 | E1000_RCTL_MPE | E1000_RCTL_BAM
                 | E1000_RCTL_BSIZE_2048 | E1000_RCTL_SECRC);
}

void send_packet(network_device_t *net_dev, void *packet, size_t size)
{
    e1000_device_t *dev = (e1000_device_t*)net_dev->pci_dev->hardware_driver;

    if (packet == 0)
    {
        debug("[e1000] can't send packet because of zero pointer\n");
        return;
    }

    if (size == 0)
    {
        debug("[e1000] can't send empty packet\n");
        return;
    }

    if (size > MAX_ETHERNET_PACKET_SIZE)
    {
        debug("[e1000] trying to send eth packet with size > %i\n", MAX_ETHERNET_PAYLOAD_SIZE);
        return;
    }

    if (size < MIN_ETHERNET_PACKET_SIZE)
    {
        debug("[e1000] trying to send eth packet with size < %i\n", MIN_ETHERNET_PACKET_SIZE);
        return;
    }

    mutex_lock(&dev->mutex);
    memcpy(dev->tx_data_buffer + DESCRIPTOR_BUFFER_SIZE * dev->tx_tail, packet, size);
    dev->tx_base[dev->tx_tail].length = size;
    dev->tx_base[dev->tx_tail].command = (1 << 3) | 3;
    dev->tx_base[dev->tx_tail].status = 0;

    uint32_t old_tail = dev->tx_tail;
    dev->tx_tail = (dev->tx_tail + 1) % TX_DESCRIPTORS_COUNT;
    mmio_write32(dev->mmio, E1000_REG_TDT, dev->tx_tail);
    mutex_release(&dev->mutex);

    while(!(dev->tx_base[old_tail].status & 1))
    {
        asm("nop");
    }
}

void e1000_init(pci_device_t *pci_dev)
{
    e1000_device_t *dev = kmalloc(sizeof(e1000_device_t));
    memset(dev, 0, sizeof(e1000_device_t));

    pci_dev->hardware_driver = dev;
    dev->pci_dev = pci_dev;

    network_device_t *net_dev = (network_device_t*)pci_dev->logical_driver;
    net_dev->send_packet = &send_packet;
    net_dev->enable = &e1000_enable;

    uint32_t mmio_region_pages = PAGE_ALIGN(pci_dev->base_address_length[0]) / 0x1000;
    dev->mmio = alloc_hardware_space_chunk(mmio_region_pages);

    for(uint32_t i = 0; i < mmio_region_pages; i++)
    {
        map_virtual_to_physical(dev->mmio + i * 0x1000, pci_dev->base_address[0] + i * 0x1000);
    }

    mmio_write32(dev->mmio, E1000_REG_IMC, 0xffffffff);
    mmio_write32(dev->mmio, E1000_REG_CTRL, mmio_read32(dev->mmio, E1000_REG_CTRL) | E1000_CTRL_RST);
    sleep(100);
    mmio_write32(dev->mmio, E1000_REG_CTRL, mmio_read32(dev->mmio, E1000_REG_CTRL) | E1000_CTRL_SLU);
    //mmio_write32(dev->mmio, E1000_REG_CTRL_EXT, mmio_read32(dev->mmio, E1000_REG_CTRL_EXT) & ~E1000_CTRL_EXT_LINK_MODE_MASK);

    if (pci_dev->device_id == 0x100e)
    {
        uint16_t mac_part = e1000_eeprom_read_8254x(dev->mmio, 0);
        net_dev->mac.b[0] = mac_part & 0xff;
        net_dev->mac.b[1] = (mac_part >> 8) & 0xff;
        mac_part = e1000_eeprom_read_8254x(dev->mmio, 1);

        net_dev->mac.b[2] = mac_part & 0xff;
        net_dev->mac.b[3] = (mac_part >> 8) & 0xff;
        mac_part = e1000_eeprom_read_8254x(dev->mmio, 2);

        net_dev->mac.b[4] = mac_part & 0xff;
        net_dev->mac.b[5] = (mac_part >> 8) & 0xff;
    }
    else if (pci_dev->device_id == 0x10f5)
    {
        uint32_t mac_part = mmio_read32(dev->mmio, E1000_REG_RAL);
        net_dev->mac.b[0] = mac_part & 0xff;
        net_dev->mac.b[1] = (mac_part >> 8) & 0xff;
        net_dev->mac.b[2] = (mac_part >> 16) & 0xff;
        net_dev->mac.b[3] = (mac_part >> 24) & 0xff;
        mac_part = mmio_read32(dev->mmio, E1000_REG_RAH);
        net_dev->mac.b[4] = mac_part & 0xff;
        net_dev->mac.b[5] = (mac_part >> 8) & 0xff;
    }

    // disable all interrupts (bits 17-31 are reserved).
    // pci-pci-x-family-gbe-controllers-software-dev-manual page 299:
    // Software should write a 1b to the reserved bits to ensure future compatibility
    mmio_write32(dev->mmio, E1000_REG_IMC, 0xffffffff);
    // link up
    mmio_write32(dev->mmio, E1000_REG_CTRL, mmio_read32(dev->mmio, E1000_REG_CTRL) | E1000_CTRL_SLU | (1 << 30));

    // Multicast array table... Something related to MAC Filtering madness
    for (uint8_t i = 0; i < 128; i++)
    {
        mmio_write32(dev->mmio, E1000_REG_MTA + i * 4, 0);
    }

    e1000_setup_rx(dev);
    e1000_setup_tx(dev);
    start_thread(rx_thread, (uint32_t)dev);
    e1000_enable(net_dev);
    debug("[e1000] initialized\n");
}
