
#ifndef H_UDP
#define H_UDP

#include "network.h"
#include "ip.h"
#include "ring.h"

#define UDP_SOCKET_SUCCESS 0
#define UDP_SOCKET_PORT_BUSY 1
#define UDP_SOCKET_TIMEOUT 2
#define UDP_SOCKET_TOO_BIG_PACKET 3

#define UDP_SOCKET_RING_SIZE 128

typedef struct udp_header
{
    uint16_t source_port;
    uint16_t destination_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_header_t;

typedef struct udp_packet
{
    eth_header_t eth;
    ip4_header_t ip;
    udp_header_t udp;
} __attribute__((packed)) udp_packet_t;

typedef struct udp_socket
{
    ring_t *receive_ring;
    uint16_t port;
    network_device_t *net_dev;
    udp_packet_t* (*receive)(struct udp_socket*, uint32_t);
    void (*free)(struct udp_socket*);
} udp_socket_t;

uint8_t send_udp_packet(udp_socket_t *socket, ip4_addr_t* ip, uint16_t port, void *payload, size_t size);
uint8_t create_udp_socket(network_device_t *net_dev, uint16_t port, udp_socket_t **out);
void init_udp_protocol();
void release_udp_socket(udp_socket_t *socket);
void process_udp_packet(network_device_t *net_dev, void *packet);
uint8_t receive_udp_packet(udp_socket_t *socket, uint32_t timeout, void **buffer);

#endif
