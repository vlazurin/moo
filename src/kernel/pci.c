#include "pci.h"
#include "port.h"
#include "log.h"
#include "string.h"
#include "liballoc.h"
#include "e1000.h"
#include "network.h"
#include "list.h"
#include "mutex.h"
#include "ata.h"
#include "vesa_lfb.h"

driver_map_node_t driver_map[] = {
    {
        .vendor_id = 0x0,
        .device_id = 0x0,
        .class_id = 0x1,
        .subclass_id = 0x1,
        .interface_id = 0,
        .init = &init_ata
    },
    {
        .vendor_id = 0x0,
        .device_id = 0x0,
        .class_id = 0x3,
        .subclass_id = 0x0,
        .interface_id = 0x0,
        .init = &init_vesa_lfb_gpu
    },
    {
        .vendor_id = 0x8086,
        .device_id = 0x100e,
        .class_id = 0,
        .subclass_id = 0,
        .interface_id = 0,
        .init = &e1000_init
    },
    {
        .vendor_id = 0x8086,
        .device_id = 0x10f5,
        .class_id = 0,
        .subclass_id = 0,
        .interface_id = 0,
        .init = &e1000_init
    }
};

pci_device_t *pci_devices = 0;
mutex_t pci_devices_mutex = {0};

uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t addr = 0x80000000 | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDR_REGISTER, addr);
    return inl(PCI_CONFIG_DATA_REGISTER);
}

void pci_config_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value)
{
    uint32_t addr = 0x80000000 | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDR_REGISTER, addr);
    outl(PCI_CONFIG_DATA_REGISTER, value);
}

void detect_pci_devices()
{
    for(int bus = 0; bus < 256; bus++)
    {
        for(int slot = 0; slot < 32; slot++)
        {
            for(int function = 0; function < 8; function++)
            {
                uint32_t config = pci_config_read(bus, slot, function, 0);
                uint32_t vendor_id = config & 0x0000FFFF;
                uint32_t device_id = config >> 16;
                if(device_id == 0xFFFF) // vendor check
                {
                    continue;
                }

                config = pci_config_read(bus, slot, function, 0xC);
                if (((config >> 16) & 0x00FF) != 0) // we support only endpoint devices
                {
                    continue;
                }

                pci_device_t *device = kmalloc(sizeof(pci_device_t));
                memset(device, 0, sizeof(pci_device_t));
                device->vendor_id = vendor_id;
                device->device_id = device_id;
                config = pci_config_read(bus, slot, function, 0x08);
                device->class = (uint8_t)(config >> 24);
                device->subclass = (uint8_t)(config >> 16);
                device->interface = (uint8_t)(config >> 8);

                debug("[PCI] device at %i:%i:%i; vendor id: %x, device id: %x, class: %x, sublclass: %x, interface: %x\n",
                    bus, slot, function, vendor_id, device_id, device->class, device->subclass, device->interface);

                for(uint8_t bar = 0; bar < 6; bar++)
                {
                    config = pci_config_read(bus, slot, function, 0x10 + bar * 4);
                    pci_config_write(bus, slot, function, 0x10 + bar * 4, 0xffffffff);
                    uint32_t bar_size = pci_config_read(bus, slot, function, 0x10 + bar * 4);
                    bar_size &= 0xFFFFFFF0;
                    bar_size = ~bar_size;
                    bar_size++; // because we need size, not a end address :)
                    pci_config_write(bus, slot, function, 0x10 + bar * 4, config);

                    if ((config & 1) == 1) {
                        device->base_address[bar] = config & 0xFFFFFFFC;
                        debug("here  %i %x\n",bar, device->base_address[bar]);
                    } else {
                        device->base_address[bar] = config & 0xFFFFFFF0; // bits 4-31
                    }
                    device->base_address_length[bar] = bar_size;
                }

                mutex_lock(&pci_devices_mutex);
                add_to_list(pci_devices, device);
                mutex_release(&pci_devices_mutex);
            }
        }
    }
}

void init_pci_devices()
{
    detect_pci_devices();

    //mutex_lock(&pci_devices_mutex);
    pci_device_t *iterator = pci_devices;
    uint32_t count = sizeof(driver_map) / sizeof(driver_map_node_t);
    while(iterator != 0)
    {
        for(uint32_t i = 0; i < count; i++)
        {
            if (driver_map[i].vendor_id == iterator->vendor_id && driver_map[i].device_id == iterator->device_id)
            {
                if (iterator->class == PCI_CLASS_NETWORK_CONTROLLER)
                {
                    iterator->logical_driver = create_network_device(iterator);
                }
                driver_map[i].init(iterator);
                register_network_device(iterator->logical_driver);
                break;
            }
            else if (driver_map[i].class_id == iterator->class && driver_map[i].subclass_id == iterator->subclass)
            {
                driver_map[i].init(iterator);
                break;
            }
        }
        iterator = (pci_device_t*)iterator->list.next;
    }
    //mutex_release(&pci_devices_mutex);
}

pci_device_t* get_pci_device_by_class(uint8_t class, pci_device_t* curr)
{
    mutex_lock(&pci_devices_mutex);
    pci_device_t *iterator = pci_devices;
    if (curr != NULL) {
        iterator = (pci_device_t*)curr->list.next;
    }
    while(iterator != NULL) {
        if (iterator->class == class) {
            mutex_release(&pci_devices_mutex);
            return iterator;
        }
        iterator = (pci_device_t*)iterator->list.next;
    }
    mutex_release(&pci_devices_mutex);
    return NULL;
}
