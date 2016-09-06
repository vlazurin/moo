#ifndef H_NETWORK
#define H_NETWORK

#include <stdint.h>
#include <stddef.h>
#include "pci.h"
#include "mutex.h"
#include "list.h"

#define MAX_ETHERNET_PAYLOAD_SIZE 1500
#define MIN_ETHERNET_PACKET_SIZE 14

#define PROTOCOL_TCP 6
#define PROTOCOL_UDP 17

#define PROTOCOL_IP 0x800
#define PROTOCOL_ARP 0x0806

uint16_t bswap16(uint16_t value);
uint32_t bswap32(uint32_t value);

typedef struct eth_addr
{
    uint8_t b[6];
}__attribute__((packed)) eth_addr_t;

typedef struct ip4_addr
{
    uint8_t b[4];
}__attribute__((packed)) ip4_addr_t;

typedef struct network_stats
{
    uint32_t packets_sent;
    uint32_t packets_received;
} network_stats_t;

typedef struct network_device network_device_t;

struct network_device
{
    list_node_t list;
    pci_device_t *pci_dev;
    ip4_addr_t ip4_addr;
    ip4_addr_t subnet_mask;
    ip4_addr_t router;
    eth_addr_t mac;
    void (*send_packet)(network_device_t*, void*, size_t);
    void (*enable)(network_device_t*);
    void (*receive_packet)(network_device_t*, void*);
    network_stats_t stats;
    uint8_t *port_map;
    mutex_t mutex;
};

network_device_t* create_network_device(pci_device_t *pci_dev);
void register_network_device(network_device_t *net_dev);

#endif
