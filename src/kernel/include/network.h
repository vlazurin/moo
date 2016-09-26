#ifndef H_NETWORK
#define H_NETWORK

#include <stdint.h>
#include <stddef.h>
#include "pci.h"
#include "mutex.h"
#include "list.h"

#define MAX_ETHERNET_PAYLOAD_SIZE 1500
#define MIN_ETHERNET_PACKET_SIZE 14
#define MAX_ETHERNET_PACKET_SIZE 1514

#define PROTOCOL_TCP 6
#define PROTOCOL_UDP 17

#define ETH_ADDR_LENGTH 6
#define IP4_ADDR_LENGTH 4

#define PROTOCOL_IP 0x0800
#define PROTOCOL_ARP 0x0806

uint16_t bswap16(uint16_t value);
uint32_t bswap32(uint32_t value);

typedef struct eth_addr
{
    uint8_t b[6];
}__attribute__((packed)) eth_addr_t;

#define COMPARE_ETH_ADDR(x, y) ((x).b[0] == (y).b[0] && (x).b[1] == (y).b[1] && (x).b[2] == (y).b[2] && (x).b[3] == (y).b[3] && (x).b[4] == (y).b[4] && (x).b[5] == (y).b[5])
#define COMPARE_IP4_ADDR(x, y) ((x).addr == (y).addr)

#define BUILD_IP4_ADDR(x1, x2, x3, x4) {((((x4) & 0xFF) << 24) | (((x3) & 0xFF) << 16) | (((x2) & 0xFF) << 8) | (((x1) & 0xFF)))}
#define IP4_ADDR_EMPTY BUILD_IP4_ADDR(0x0, 0x0, 0x0, 0x0)
#define IP4_ADDR_BROADCAST BUILD_IP4_ADDR(0xFF, 0xFF, 0xFF, 0xFF)
#define COMPARE_SUBNETS(x, y, z) (((x).addr & (z).addr) == ((y).addr & (z).addr))

#define ETH_ADDR_EMPTY {{0, 0, 0, 0, 0, 0}}
#define ETH_ADDR_BROADCAST {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}

typedef struct ip4_addr
{
    uint32_t addr;
}__attribute__((packed)) ip4_addr_t;

typedef struct eth_header
{
    eth_addr_t destination;
    eth_addr_t source;
    uint16_t type;
}__attribute__((packed)) eth_header_t;

typedef struct eth_packet
{
    eth_header_t eth;
}__attribute__((packed)) eth_packet_t;

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
    void (*receive_packet)(network_device_t*, eth_packet_t*);
    network_stats_t stats;
    mutex_t mutex;
};

network_device_t* create_network_device(pci_device_t *pci_dev);
void register_network_device(network_device_t *net_dev);
void ip4_to_str(ip4_addr_t *addr, char *buff);

#endif
