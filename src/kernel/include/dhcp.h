#ifndef H_DHCP
#define H_DHCP

#include <stdint.h>
#include "network.h"
#include "udp.h"

#define DHCP_REQUEST 1
#define DHCP_RESPONSE 2

#define DHCP_TYPE_DISCOVER 1
#define DHCP_TYPE_OFFER 2
#define DHCP_TYPE_REQUEST 3
#define DHCP_TYPE_DECLINE 4
#define DHCP_TYPE_ACK 5
#define DHCP_TYPE_NOT_ACK 6
#define DHCP_TYPE_RELEASE 7
#define DHCP_TYPE_INFORMATION 8

enum
{
    DHCP_OK = 0,
    DHCP_NO_REPLY,
    DHCP_WRONG_TRANSACTION,
    DHCP_WRONG_MESSAGE_TYPE,
    DHCP_SOCKET_ERROR,
    DHCP_INVALID_IP
};

typedef struct dhcp_header
{
    uint8_t code;
    uint8_t hardware_type;
    uint8_t length;
    uint8_t hops;
    uint32_t transaction_id;
    uint16_t seconds;
    uint16_t flags;
    ip4_addr_t client_ip;
    ip4_addr_t your_ip;
    ip4_addr_t server_ip;
    ip4_addr_t router_ip;
    eth_addr_t client_mac;
    uint8_t reserved[10]; // for hardware address we have 16 bytes... we need only 6 -> 10 is reserved
    uint8_t server_name[64];
    uint8_t boot_file_name[128];
} __attribute__((packed)) dhcp_header_t;

typedef struct dhcp_packet
{
    eth_header_t eth;
    ip4_header_t ip;
    udp_header_t udp;
    dhcp_header_t dhcp;
} __attribute__((packed)) dhcp_packet_t;

uint8_t configure_dhcp(network_device_t *net_dev);

#endif
