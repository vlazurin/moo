#ifndef H_IP
#define H_IP

#include <stdint.h>
#include "network.h"

#define IP_VERSION 4
#define IP_DEFAULT_TTL 128

typedef struct ip4_header
{
    uint8_t header_size: 4; // in words, minimum 5
    uint8_t version: 4;
    uint8_t type_of_service;
    uint16_t total_size;
    uint16_t identification;
    uint16_t fragment_offset: 13;
    uint16_t flags: 3;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t header_checksum;
    ip4_addr_t source_ip;
    ip4_addr_t destination_ip;
} __attribute__((packed)) ip4_header_t;

typedef struct ip_packet
{
    eth_header_t eth;
    ip4_header_t ip;
} __attribute__((packed)) ip_packet_t;

void send_ip4_packet(network_device_t *net_dev, ip4_addr_t* destination, uint8_t protocol, void *packet, size_t size);

#endif
