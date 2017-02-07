#include <stdint.h>
#include "udp.h"
#include "pit.h"
#include "string.h"
#include "log.h"
#include "liballoc.h"
#include "mutex.h"

// UDP ports data and queue must be stored in net_dev, but i have no plans to support machines with 2+ lan cards
udp_socket_t **udp_sockets = 0;
mutex_t udp_sockets_mutex;

void init_udp_protocol()
{
    udp_sockets = kmalloc(sizeof(udp_socket_t*) * 0xFFFF);
    memset(udp_sockets, 0, sizeof(udp_socket_t*) * 0xFFFF);
}

void process_udp_packet(network_device_t *net_dev, void *packet)
{
    udp_packet_t *udp_packet = packet;
    uint16_t port = bswap16(udp_packet->udp.destination_port);

    if (udp_sockets[port] != 0)
    {
        // TODO: add validation for socket state... it can be deleted
        void *buffer = kmalloc(udp_packet->ip.total_size + sizeof(eth_header_t));
        memcpy(buffer, packet, udp_packet->ip.total_size + sizeof(eth_header_t));
        udp_sockets[port]->receive_ring->push(udp_sockets[port]->receive_ring, buffer);
    }
}

void release_udp_socket(udp_socket_t *socket)
{
    mutex_lock(&udp_sockets_mutex);
    udp_sockets[socket->port] = 0;
    mutex_release(&udp_sockets_mutex);
    socket->receive_ring->free(socket->receive_ring);
    kfree(socket);
}

uint8_t create_udp_socket(network_device_t *net_dev, uint16_t port, udp_socket_t **out)
{
    mutex_lock(&udp_sockets_mutex);
    if (udp_sockets[port] == 0)
    {
        udp_sockets[port] = kmalloc(sizeof(udp_socket_t));
        memset(udp_sockets[port], 0, sizeof(udp_socket_t));
        udp_sockets[port]->net_dev = net_dev;
        udp_sockets[port]->port = port;
        udp_sockets[port]->receive_ring = create_ring(UDP_SOCKET_RING_SIZE);
        *out = udp_sockets[port];
        mutex_release(&udp_sockets_mutex);
        return UDP_SOCKET_SUCCESS;
    }
    mutex_release(&udp_sockets_mutex);
    return UDP_SOCKET_PORT_BUSY;
}

uint8_t receive_udp_packet(udp_socket_t *socket, uint32_t timeout, void **buffer)
{
    void *ptr = 0;
    uint32_t limit = get_pit_ticks() + timeout;
    while(limit > get_pit_ticks())
    {
        ptr = socket->receive_ring->pop(socket->receive_ring);
        if (ptr > 0)
        {
            *buffer = ptr;
            return UDP_SOCKET_SUCCESS;
        }
    }
    return UDP_SOCKET_TIMEOUT;
}

uint8_t send_udp_packet(udp_socket_t *socket, ip4_addr_t* ip, uint16_t port, void *payload, size_t size)
{
    if (size + sizeof(udp_packet_t) > MAX_ETHERNET_PACKET_SIZE)
    {
        return UDP_SOCKET_TOO_BIG_PACKET;
    }

    uint16_t total_size = size + sizeof(udp_packet_t);
    udp_packet_t *udp_packet = kmalloc(total_size);
    memset(udp_packet, 0, total_size);
    memcpy(udp_packet + 1, payload, size);
    udp_packet->udp.source_port = bswap16(socket->port);
    udp_packet->udp.destination_port = bswap16(port);
    udp_packet->udp.length = bswap16(size + sizeof(udp_header_t)); // only UDP header + payload

    send_ip4_packet(socket->net_dev, ip, PROTOCOL_UDP, udp_packet, total_size);
    kfree(udp_packet);
    return UDP_SOCKET_SUCCESS;
}
