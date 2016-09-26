#include "network.h"
#include "mutex.h"
#include "liballoc.h"
#include "debug.h"
#include "string.h"
#include "arp.h"
#include "ip.h"
#include "udp.h"
#include "stdlib.h"

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

void ip4_to_str(ip4_addr_t *addr, char *buff)
{
    itoa(addr->addr & 0xFF, buff, 10);
    buff += strlen(buff);
    *buff++ = '.';
    itoa((addr->addr >> 8) & 0xFF, buff, 10);
    buff += strlen(buff);
    *buff++ = '.';
    itoa((addr->addr >> 16) & 0xFF, buff, 10);
    buff += strlen(buff);
    *buff++ = '.';
    itoa((addr->addr >> 24) & 0xFF, buff, 10);
}

void receive_packet(struct network_device *net_dev, eth_packet_t *packet)
{
    uint16_t type = bswap16(packet->eth.type);

    if (type == PROTOCOL_ARP)
    {
        arp_packet_t *arp_packet = (arp_packet_t*)packet;
        if (bswap16(arp_packet->arp.operation) == ARP_REPLY)
        {
            if (COMPARE_ETH_ADDR(arp_packet->eth.destination, net_dev->mac))
            {
                add_mac_to_arp_cache(&arp_packet->arp.source_ip, &arp_packet->arp.source);
            }
        }
        else
        {
            process_arp_request(net_dev, packet);
        }
    }
    else if (type == PROTOCOL_IP)
    {
        ip_packet_t *ip_packet = (ip_packet_t*)packet;

        if (ip_packet->ip.protocol == PROTOCOL_UDP)
        {
            process_udp_packet(net_dev, ip_packet);
        }
    }

    __sync_add_and_fetch(&net_dev->stats.packets_received, 1);
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
    net_dev->pci_dev = pci_dev;
    net_dev->receive_packet = &receive_packet;

    return net_dev;
}
