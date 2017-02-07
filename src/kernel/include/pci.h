#ifndef H_PCI
#define H_PCI

#include <stdint.h>
#include "list.h"

#define PCI_ENABLE_BIT 0x80000000
#define PCI_CONFIG_ADDR_REGISTER 0xCF8
#define PCI_CONFIG_DATA_REGISTER 0xCFC

#define PCI_CLASS_MASS_STORAGE 0x01
#define PCI_CLASS_NETWORK_CONTROLLER 0x02
#define PCI_CLASS_VIDEO_ADAPTER 0x03

#define PCI_SUB_CLASS_IDE 0x01

typedef struct pci_device
{
    list_node_t list;
    uint32_t vendor_id;
    uint32_t device_id;
    uint8_t class;
    uint8_t subclass;
    uint8_t interface;
    uint32_t base_address[6];
    uint32_t base_address_length[6];
    void* hardware_driver;
    void* logical_driver;
} pci_device_t;

typedef struct driver_map_node
{
    uint32_t vendor_id;
    uint32_t device_id;
    uint8_t class_id;
    uint8_t subclass_id;
    uint8_t interface_id;
    void (*init)(pci_device_t*);
} driver_map_node_t;

void detect_pci_devices();
void init_pci_devices();
pci_device_t* get_pci_device_by_class(uint8_t class, pci_device_t* curr);

#endif
