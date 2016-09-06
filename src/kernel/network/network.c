#include "network.h"
#include "mutex.h"
#include "liballoc.h"
#include "string.h"

network_device_t *network_devices = 0;
mutex_t network_devices_mutex = 0;

uint16_t bswap16(uint16_t value)
{
    return ((value << 8) & 0xFF00) | ((value >> 8) & 0x00FF);
}

uint32_t bswap32(uint32_t value)
{
    return __builtin_bswap32(value);
}

void receive_packet(struct network_device *net_dev, void *packet)
{
    
}

void register_network_device(network_device_t *net_dev)
{
    mutex_lock(&network_devices_mutex);
    add_to_list(network_devices, net_dev);
    mutex_release(&network_devices_mutex);
}

network_device_t* create_network_device(pci_device_t *pci_dev)
{
    network_device_t *net_dev = kmalloc(sizeof(network_device_t));
    memset(net_dev, 0, sizeof(network_device_t));
    //net_dev->port_map = kmalloc(PORT_MAP_SIZE);
    //memset(&net_dev->port_map, 0, PORT_MAP_SIZE);

    net_dev->pci_dev = pci_dev;
    net_dev->receive_packet = &receive_packet;

    return net_dev;
}
