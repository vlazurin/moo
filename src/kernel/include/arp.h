#ifndef H_ARP
#define H_ARP

#include <stdint.h>
#include "network.h"
#include "list.h"

#define ARP_REQUEST 1
#define ARP_REPLY 2

#define ARP_HARDWARE_TYPE_ETHERNET 1

typedef struct arp_entry
{
    list_node_t list;
    ip4_addr_t ip_addr;
    eth_addr_t mac;
} arp_entry_t;

typedef struct arp_header
{
    uint16_t hardware_type;
    uint16_t protocol;
    uint8_t hardware_address_length;
    uint8_t protocol_length;
    uint16_t operation;
    eth_addr_t source;
    ip4_addr_t source_ip;
    eth_addr_t destination;
    ip4_addr_t destination_ip;
} __attribute__((packed)) arp_header_t;

typedef struct arp_packet
{
    eth_header_t eth;
    arp_header_t arp;
} __attribute__((packed)) arp_packet_t;

uint8_t get_mac_from_cache(const ip4_addr_t *ip, eth_addr_t *out);
void add_mac_to_arp_cache(ip4_addr_t *ip, eth_addr_t *mac);
uint8_t get_mac_by_ip(network_device_t *net_dev, ip4_addr_t *ip, eth_addr_t *out);
void process_arp_request(network_device_t *net_dev, void *packet);

#endif
