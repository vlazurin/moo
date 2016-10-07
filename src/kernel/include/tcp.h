#ifndef H_TCP
#define H_TCP

#include <stdint.h>
#include "network.h"
#include "ring.h"
#include "ip.h"
#include "buffer.h"
#include "list.h"

#define TCP_FLAG_FIN (1 << 0)
#define TCP_FLAG_SYN (1 << 1)
#define TCP_FLAG_RST (1 << 2)
#define TCP_FLAG_ACK (1 << 4)

#define TCP_MAX_PAYLOAD_SIZE MAX_ETHERNET_PACKET_SIZE - sizeof(tcp_packet_t)
#define TCP_SOCKET_TIME_WAIT 15000

enum
{
    TCP_CONNECTION_CLOSED = 1,
    TCP_CONNECTION_SYN_SENT,
    TCP_CONNECTION_ESTABLISHED,
    TCP_CONNECTION_LAST_ACK,
    TCP_CONNECTION_CLOSE_WAIT,
    TCP_CONNECTION_FIN_1,
    TCP_CONNECTION_FIN_2,
    TCP_CONNECTION_TIME_WAIT
};

enum
{
    TCP_SOCKET_SUCCESS = 1,
    TCP_SOCKET_PORT_BUSY,
    TCP_SOCKET_TIMEOUT,
    TCP_SOCKET_BUFFER_IS_FULL,
    TCP_SOCKET_TRANSMIT_NOT_FINISHED,
    TCP_SOCKET_WRONG_STATE
};

typedef struct tcp_header
{
    uint16_t source_port;
    uint16_t destination_port;
    uint32_t seq_number;
    uint32_t ack_number;
    uint8_t ns:1;
    uint8_t reserved:3;
    uint8_t data_offset:4;
    uint8_t flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_pointer;
} __attribute__((packed)) tcp_header_t;

typedef struct tcp_pseudo_header
{
    ip4_addr_t source;
    ip4_addr_t destination;
    uint8_t reserved;
    uint8_t protocol;
    uint16_t tcp_length;
} __attribute__((packed)) tcp_pseudo_header_t;

typedef struct tcp_packet
{
    eth_header_t eth;
    ip4_header_t ip;
    tcp_header_t tcp;
} __attribute__((packed)) tcp_packet_t;

typedef struct tcp_socket tcp_socket_t;

typedef struct tcp_socket_binder
{
    mutex_t mutex;
    uint16_t port;
    tcp_socket_t *connected;
    tcp_socket_t *accept_wait;
    tcp_socket_t *handshake;
    tcp_socket_t *time_wait;
    uint8_t is_listening;
} tcp_socket_binder_t;

struct tcp_socket
{
    list_node_t list;
    uint8_t state;
    uint16_t port;
    network_device_t *net_dev;
    ip4_addr_t remote_host;
    uint16_t remote_port;
    uint32_t seq_number;
    uint32_t ack_number;
    uint32_t remote_ack_number;
    uint32_t remote_window_size;
    buffer_t *transmit_buffer;
    buffer_t *receive_buffer;
    ring_t *ring;
    uint32_t ttl;
};

void init_tcp_protocol();
uint8_t tcp_bind(network_device_t* net_dev, uint16_t port, tcp_socket_binder_t **out);
uint8_t tcp_connect(network_device_t *net_dev, ip4_addr_t *ip, uint16_t port, tcp_socket_binder_t *binder, tcp_socket_t **out);
void process_tcp_packet(network_device_t *net_dev, void* packet);
uint32_t read_from_tcp_socket(tcp_socket_t *socket, void *buffer, uint32_t size, uint32_t timeout);
uint8_t close_tcp_connection(tcp_socket_t *socket);
uint8_t write_to_tcp_socket(tcp_socket_t *socket, void *buffer, uint32_t size);
uint8_t tcp_listen(tcp_socket_binder_t *binder);
uint8_t accept_tcp_connection(tcp_socket_binder_t *binder, tcp_socket_t **out, uint32_t timeout);

#endif
