#include "tcp.h"
#include "pit.h"
#include "mutex.h"
#include "task.h"
#include "liballoc.h"
#include "string.h"
#include "log.h"

#define TCP_SOCKET_BUFFER_SIZE 65535
#define TCP_SOCKET_RING_SIZE 128
#define TCP_CONNECT_TIMEOUT 5000

// TCP sockets data must be stored in net_dev, but i have no plans to support machines with 2+ lan cards
tcp_socket_binder_t **tcp_binders = 0;
mutex_t tcp_binders_mutex = {0};

uint8_t send_tcp_packet(tcp_socket_t *socket, uint8_t flags, void* payload, uint16_t size);

tcp_socket_t *get_tcp_socket_by_remote(tcp_socket_t *list, ip4_addr_t *ip, uint16_t port)
{
    tcp_socket_t *current = list;
    while(current != 0)
    {
        if (current->remote_port == port && COMPARE_IP4_ADDR(current->remote_host, *ip))
        {
            return current;
        }
        current = (tcp_socket_t*)current->list.next;
    }

    return 0;
}

tcp_socket_t *create_tcp_socket(network_device_t *net_dev, uint16_t port, ip4_addr_t *remote_ip, uint16_t remote_port)
{
    tcp_socket_t *socket = kmalloc(sizeof(tcp_socket_t));
    memset(socket, 0, sizeof(tcp_socket_t));
    socket->state = TCP_CONNECTION_CLOSED;
    socket->remote_port = remote_port;
    socket->remote_host = *remote_ip;
    socket->port = port;
    socket->net_dev = net_dev;
    socket->ring = create_ring(TCP_SOCKET_RING_SIZE);
    socket->receive_buffer = create_buffer(TCP_SOCKET_BUFFER_SIZE);
    socket->transmit_buffer = create_buffer(TCP_SOCKET_BUFFER_SIZE);

    return socket;
}

void process_tcp_packet(network_device_t *net_dev, void* packet)
{
    tcp_packet_t *tcp_packet = packet;
    create_tcp_socket(net_dev, bswap16(tcp_packet->tcp.destination_port), &tcp_packet->ip.source_ip, bswap16(tcp_packet->tcp.source_port));
    uint16_t port = bswap16(tcp_packet->tcp.destination_port);

    if (tcp_binders[port] != 0)
    {
        mutex_lock(&tcp_binders[port]->mutex);
        // TODO: add validation for socket state... it can be deleted
        uint16_t buffer_size = bswap16(tcp_packet->ip.total_size) + sizeof(eth_header_t);
        uint16_t remote_port = bswap16(tcp_packet->tcp.source_port);
        tcp_socket_t *socket = 0;

        // try to find socket
        if (socket == 0)
        {
            socket = get_tcp_socket_by_remote(tcp_binders[port]->connected, &tcp_packet->ip.source_ip, remote_port);
        }
        if (socket == 0)
        {
            socket = get_tcp_socket_by_remote(tcp_binders[port]->handshake, &tcp_packet->ip.source_ip, remote_port);
        }
        if (socket == 0)
        {
            socket = get_tcp_socket_by_remote(tcp_binders[port]->accept_wait, &tcp_packet->ip.source_ip, remote_port);
        }

        // if no our side has no socket and packet is connection request and system listens local port -> create new socket
        // this block of code must be after get_tcp_socket_by_remote() because system can receive request for socket in TIME_WAIT state
        if (socket == 0 && tcp_binders[port]->is_listening == 1 && tcp_packet->tcp.flags == TCP_FLAG_SYN)
        {
            socket = create_tcp_socket(net_dev, port, &tcp_packet->ip.source_ip, bswap16(tcp_packet->tcp.source_port));
            socket->state = TCP_CONNECTION_CLOSED;
            add_to_list(tcp_binders[port]->handshake, socket);
            mutex_release(&tcp_binders[port]->mutex);

            char ip_string[16];
            memset(&ip_string, 0, 16);
            ip4_to_str(&tcp_packet->ip.source_ip, ip_string);
            //debug("[tcp] created new socket for income connection from %s:%i\n", ip_string, remote_port);
        }

        if (socket == 0)
        {
            char ip_string[16];
            memset(&ip_string, 0, 16);
            ip4_to_str(&tcp_packet->ip.source_ip, ip_string);
            //debug("[tcp] received packet for unknown socket from %s:%i\n", ip_string, remote_port);
            mutex_release(&tcp_binders[port]->mutex);
            return;
        }

        void *buffer = kmalloc(buffer_size);
        memcpy(buffer, packet, buffer_size);
        uint8_t result = socket->ring->push(socket->ring, buffer);
        if (result != RING_BUFFER_OK)
        {
            //debug("[tcp] socket ring is full\n");
            hlt();
        }
    }

    mutex_release(&tcp_binders[port]->mutex);
}

void handle_syn_state(tcp_socket_t *socket, uint8_t *reply_flags, tcp_packet_t *packet)
{
    uint16_t port = socket->port;

    if (packet->tcp.flags == (TCP_FLAG_ACK | TCP_FLAG_SYN))
    {
        uint32_t seq_number = bswap32(packet->tcp.seq_number);
        socket->ack_number = seq_number + 1;
        *reply_flags |= TCP_FLAG_ACK;
        socket->state = TCP_CONNECTION_ESTABLISHED;
        delete_from_list((void*)&tcp_binders[port]->handshake, socket);
        add_to_list(tcp_binders[port]->connected, socket);

        char ip_string[16];
        memset(&ip_string, 0, 16);
        ip4_to_str(&socket->remote_host, ip_string);
        //debug("[tcp] connected to remote %s:%i\n", ip_string, socket->remote_port);
    }
    else
    {
        *reply_flags |= TCP_FLAG_RST;
    }
}

void handle_received_payload(tcp_socket_t *socket, uint8_t *reply_flags, tcp_packet_t *packet)
{
    uint32_t payload_size = bswap16(packet->ip.total_size) - sizeof(ip4_header_t) - packet->tcp.data_offset * 4;
    if (payload_size > 0)
    {
        if (payload_size <= socket->receive_buffer->get_free_space(socket->receive_buffer))
        {
            void* payload = (void*)packet + sizeof(ip_packet_t) + packet->tcp.data_offset * 4;
            socket->receive_buffer->add(socket->receive_buffer, payload, payload_size);
            socket->ack_number += payload_size;

            char ip_string[16];
            memset(&ip_string, 0, 16);
            ip4_to_str(&socket->remote_host, ip_string);
            //debug("[tcp] received %i bytes from remote %s:%i\n", payload_size, ip_string, socket->remote_port);
        }
        else
        {
            // Simply print debug message, remote host will receive our window size in reply.
            // Probably socket must wait some time for a free space in buffer and only then ask about packet resend.
            char ip_string[16];
            memset(&ip_string, 0, 16);
            ip4_to_str(&socket->remote_host, ip_string);
            //debug("[tcp] not enough free space in receive_buffer, remote: %s:%i\n", ip_string, socket->remote_port);
        }

        *reply_flags |= TCP_FLAG_ACK;
    }
}

void handle_established_state(tcp_socket_t *socket, uint8_t *reply_flags, tcp_packet_t *packet)
{
    handle_received_payload(socket, reply_flags, packet);

    if (packet->tcp.flags & TCP_FLAG_FIN)
    {
        socket->state = TCP_CONNECTION_CLOSE_WAIT;
        *reply_flags |= TCP_FLAG_ACK;
        socket->ack_number++;

        char ip_string[16];
        memset(&ip_string, 0, 16);
        ip4_to_str(&socket->remote_host, ip_string);
        //debug("[tcp] received FIN from remote: %s:%i, socket state is CLOSE_WAIT\n", ip_string, socket->remote_port);
    }
}

void handle_last_ask_state(tcp_socket_t *socket, uint8_t *reply_flags, tcp_packet_t *packet)
{
    if (packet->tcp.flags & TCP_FLAG_ACK)
    {
        socket->state = TCP_CONNECTION_TIME_WAIT;
        socket->ttl = TCP_SOCKET_TIME_WAIT + get_pit_ticks();
        delete_from_list((void*)&tcp_binders[socket->port]->connected, socket);
        // TODO: socket must know about own queue, so we don't need to try delete from 2 queues
        delete_from_list((void*)&tcp_binders[socket->port]->accept_wait, socket);
        add_to_list(tcp_binders[socket->port]->time_wait, socket);

        char ip_string[16];
        memset(&ip_string, 0, 16);
        ip4_to_str(&socket->remote_host, ip_string);
        //debug("[tcp] received LAST_ASK from remote: %s:%i, socket is moved to time_wait queue\n", ip_string, socket->remote_port);
    }
}

void handle_fin_1_state(tcp_socket_t *socket, uint8_t *reply_flags, tcp_packet_t *packet)
{
    handle_received_payload(socket, reply_flags, packet);

    if (packet->tcp.flags & TCP_FLAG_FIN)
    {
        socket->state = TCP_CONNECTION_TIME_WAIT;
        *reply_flags |= TCP_FLAG_ACK;
        socket->ack_number++;
        socket->ttl = TCP_SOCKET_TIME_WAIT + get_pit_ticks();
        delete_from_list((void*)&tcp_binders[socket->port]->connected, socket);
        add_to_list(tcp_binders[socket->port]->time_wait, socket);

        char ip_string[16];
        memset(&ip_string, 0, 16);
        ip4_to_str(&socket->remote_host, ip_string);
        //debug("[tcp] received LAST_ASK AND FIN from remote: %s:%i, socket is moved to time_wait queue\n", ip_string, socket->remote_port);
    }
}

void handle_closed_state(tcp_socket_t *socket, uint8_t *reply_flags, tcp_packet_t *packet)
{
    uint16_t port = socket->port;
    if (packet->tcp.flags == TCP_FLAG_SYN && tcp_binders[port]->is_listening == 1)
    {
        uint32_t seq_number = bswap32(packet->tcp.seq_number);
        socket->ack_number = seq_number + 1;
        *reply_flags |= TCP_FLAG_SYN;
        socket->state = TCP_CONNECTION_ESTABLISHED;
        delete_from_list((void*)&tcp_binders[port]->handshake, socket);
        add_to_list(tcp_binders[port]->accept_wait, socket);
        char ip_string[16];
        memset(&ip_string, 0, 16);
        ip4_to_str(&packet->ip.source_ip, ip_string);
        //debug("[tcp] established connection on local port %i with remote %s:%i\n", port, ip_string, bswap16(packet->tcp.source_port));
    }
    else
    {
        // Probably this code is never executed, because we create new socket only if packet flags are ok and binder is listening.
        // Just a failsafe.
        socket->state = TCP_CONNECTION_TIME_WAIT;
        socket->ttl = TCP_SOCKET_TIME_WAIT + get_pit_ticks();
        delete_from_list((void*)&tcp_binders[port]->handshake, socket);
        add_to_list(tcp_binders[port]->time_wait, socket);

        char ip_string[16];
        memset(&ip_string, 0, 16);
        ip4_to_str(&packet->ip.source_ip, ip_string);
        //debug("[tcp] can't establish connection with remote %s:%i, local socket is in closed state, packet flags is %i, binder is_listening %i\n",
        //    ip_string, bswap16(packet->tcp.source_port), packet->tcp.flags, tcp_binders[port]->is_listening);
    }
}

void handle_fin_2_state(tcp_socket_t *socket, uint8_t *reply_flags, tcp_packet_t *packet)
{
    if (packet->tcp.flags & TCP_FLAG_FIN)
    {
        *reply_flags |= TCP_FLAG_ACK;
        socket->ack_number++;
        socket->state = TCP_CONNECTION_TIME_WAIT;
        socket->ttl = TCP_SOCKET_TIME_WAIT + get_pit_ticks();
        delete_from_list((void*)&tcp_binders[socket->port]->connected, socket);
        add_to_list(tcp_binders[socket->port]->time_wait, socket);

        char ip_string[16];
        memset(&ip_string, 0, 16);
        ip4_to_str(&socket->remote_host, ip_string);
        //debug("[tcp] closing connection with remote %s:%i, local socket moved to TIME_WAIT state\n", ip_string, socket->remote_port);
    }
}

void process_socket_queue(tcp_socket_t **list, void* transmit_payload)
{
    tcp_packet_t *received_packet = 0;
    tcp_socket_t *socket = *list;
    tcp_socket_t *next = 0;

    while(socket != 0)
    {
        // socket can be moved to other queue, so we need our next item backup
        next = (tcp_socket_t*)socket->list.next;

        uint8_t reply_flags = 0;
        uint16_t port = socket->port;
        received_packet = socket->ring->pop(socket->ring);
        if (received_packet != 0)
        {
            if (received_packet->tcp.flags & TCP_FLAG_RST)
            {
                socket->state = TCP_CONNECTION_TIME_WAIT;
                socket->ttl = TCP_SOCKET_TIME_WAIT + get_pit_ticks();
                delete_from_list((void*)list, socket);
                add_to_list(tcp_binders[port]->time_wait, socket);

                socket = (tcp_socket_t*)socket->list.next;

                char ip_string[16];
                memset(&ip_string, 0, 16);
                ip4_to_str(&received_packet->ip.source_ip, ip_string);
                //debug("[tcp] received RST packet from %s:%i\n", ip_string, bswap16(received_packet->tcp.source_port));

                kfree(received_packet);

                socket = next;
                continue;
            }

            socket->remote_window_size = bswap16(received_packet->tcp.window_size);

            if (socket->state == TCP_CONNECTION_CLOSED)
            {
                handle_closed_state(socket, &reply_flags, received_packet);
            }
            else if (bswap32(received_packet->tcp.seq_number) < socket->ack_number)
            {
                // simply ignore packet with seq_number < our ack_number
                reply_flags |= TCP_FLAG_ACK;
            }
            else if (bswap32(received_packet->tcp.seq_number) > socket->ack_number)
            {
                // seems we got packet from future, but we have no reorder support
                reply_flags |= TCP_FLAG_ACK;
            }
            else if (socket->state == TCP_CONNECTION_ESTABLISHED)
            {
                handle_established_state(socket, &reply_flags, received_packet);
            }
            else if (socket->state == TCP_CONNECTION_CLOSE_WAIT)
            {
                // socket ignores all income data packets, we need to send all data in our transmit_buffer and close connection
            }
            else if (socket->state == TCP_CONNECTION_LAST_ACK)
            {
                handle_last_ask_state(socket, &reply_flags, received_packet);
            }
            else if (socket->state == TCP_CONNECTION_FIN_1)
            {
                handle_fin_1_state(socket, &reply_flags, received_packet);
            }
            else if (socket->state == TCP_CONNECTION_FIN_2)
            {
                handle_fin_2_state(socket, &reply_flags, received_packet);
            }
            if (socket->state == TCP_CONNECTION_SYN_SENT)
            {
                handle_syn_state(socket, &reply_flags, received_packet);
            }

            kfree(received_packet);
        }

        uint32_t copied = 0;
        if (socket->state == TCP_CONNECTION_ESTABLISHED || socket->state == TCP_CONNECTION_CLOSE_WAIT || socket->state == TCP_CONNECTION_FIN_1)
        {
            uint32_t max_payload_size = (socket->remote_window_size < TCP_MAX_PAYLOAD_SIZE ? socket->remote_window_size : TCP_MAX_PAYLOAD_SIZE);
            copied = socket->transmit_buffer->get(socket->transmit_buffer, transmit_payload, max_payload_size);
        }

        if (socket->state == TCP_CONNECTION_CLOSE_WAIT && socket->transmit_buffer->get_free_space(socket->transmit_buffer) == TCP_SOCKET_BUFFER_SIZE)
        {
            socket->state = TCP_CONNECTION_LAST_ACK;
            reply_flags |= TCP_FLAG_FIN;

            char ip_string[16];
            memset(&ip_string, 0, 16);
            ip4_to_str(&socket->remote_host, ip_string);
            //debug("[tcp] sending FIN confirm to remote: %s:%i, socket is in LAST_ASK state\n", ip_string, socket->remote_port);
        }

        if (socket->state == TCP_CONNECTION_FIN_1)
        {
            if (socket->transmit_buffer->get_free_space(socket->transmit_buffer) == TCP_SOCKET_BUFFER_SIZE)
            {
                socket->state = TCP_CONNECTION_FIN_2;
                reply_flags |= TCP_FLAG_FIN;

                char ip_string[16];
                memset(&ip_string, 0, 16);
                ip4_to_str(&socket->remote_host, ip_string);
                //debug("[tcp] sending FIN confirm to remote: %s:%i, socket is in FIN_2 state\n", ip_string, socket->remote_port);
            }
        }

        if (copied > 0 || reply_flags != 0)
        {
            reply_flags |= TCP_FLAG_ACK;
            send_tcp_packet(socket, reply_flags, transmit_payload, copied);
        }

        socket = next;
    }
}

void process_tcp_data()
{
    uint8_t *transmit_payload = kmalloc(TCP_MAX_PAYLOAD_SIZE);

    while(1)
    {
        for(uint16_t port = 0; port < 0xFFFF; port++)
        {
            if(tcp_binders[port] == 0)
            {
                continue;
            }

            assert(0 && "sleep queue isn't used becauf of direct flag usage");
            if (__sync_lock_test_and_set(&tcp_binders[port]->mutex.flag, 1))
            {
                // binder will be handled next time
                continue;
            }

            process_socket_queue(&tcp_binders[port]->handshake, transmit_payload);
            process_socket_queue(&tcp_binders[port]->connected, transmit_payload);
            process_socket_queue(&tcp_binders[port]->accept_wait, transmit_payload);

            tcp_socket_t *socket = tcp_binders[port]->time_wait;
            while(socket != 0)
            {
                tcp_socket_t *next = (tcp_socket_t*)socket->list.next;
                if (socket->ttl <= get_pit_ticks())
                {
                    delete_from_list((void*)&tcp_binders[port]->time_wait, socket);

                    char ip_string[16];
                    memset(&ip_string, 0, 16);
                    ip4_to_str(&socket->remote_host, ip_string);
                    //debug("[tcp] socket for remote %s:%i removed from TIME_WAIT queue\n", ip_string, socket->remote_port);

                    socket->ring->free(socket->ring);
                    socket->transmit_buffer->free(socket->transmit_buffer);
                    socket->receive_buffer->free(socket->receive_buffer);
                    kfree(socket);
                }
                socket = next;
            }

            mutex_release(&tcp_binders[port]->mutex);
        }
    }

    //this code can't be executed... just a failsafe
    kfree(transmit_payload);
    //debug("[tcp] end of process_tcp_data()...\n");
}

void init_tcp_protocol()
{
    tcp_binders = kmalloc(sizeof(tcp_socket_binder_t*) * 0xFFFF);
    memset(tcp_binders, 0, sizeof(tcp_socket_binder_t*) * 0xFFFF);
    start_thread(process_tcp_data, 0);
}

uint8_t accept_tcp_connection(tcp_socket_binder_t *binder, tcp_socket_t **out, uint32_t timeout)
{
    uint32_t finish = get_pit_ticks() + timeout;
    while(finish > get_pit_ticks())
    {
        // TODO: accept from end of list & use mutex
        if (binder->accept_wait != 0)
        {
            mutex_lock(&binder->mutex);
            *out = binder->accept_wait;
            delete_from_list((void*)&binder->accept_wait, *out);
            add_to_list(binder->connected, *out);
            mutex_release(&binder->mutex);
            return TCP_SOCKET_SUCCESS;
        }
    }

    return TCP_SOCKET_TIMEOUT;
}

void fill_tcp_checksum(tcp_packet_t *packet, ip4_addr_t *source, ip4_addr_t *dest, uint16_t data_size)
{
    uint32_t checksum = 0;
    tcp_pseudo_header_t ps;
    ps.destination = *dest;
    ps.source = *source;
    ps.protocol = PROTOCOL_TCP;
    ps.reserved = 0;
    ps.tcp_length = bswap16(sizeof(tcp_header_t) + data_size);

    uint16_t *p = (uint16_t*)&ps;
    for(uint8_t i = 0; i < sizeof(tcp_pseudo_header_t) / 2; i++)
    {
        checksum += *p;
        while(checksum > 0xFFFF)
        {
            checksum = (checksum >> 16) + (checksum & 0x0FFFF);
        }
        p++;
    }

    p = (uint16_t*)&packet->tcp;
    for(uint32_t i = 0; i < (sizeof(tcp_header_t) + data_size) / 2; i++)
    {
        checksum += *p; // we dont worry about checksum field, because it's 0
        while(checksum > 0xFFFF)
        {
            checksum = (checksum >> 16) + (checksum & 0x0FFFF);
        }
        p++;
    }

    if ((sizeof(tcp_header_t) + data_size) % 2 != 0)
    {
        checksum += *(uint8_t*)p;
    }

    while(checksum > 0xFFFF)
    {
        checksum = (checksum >> 16) + (checksum & 0x0FFFF);
    }

    checksum = ~checksum;
    packet->tcp.checksum = (uint16_t)checksum;
}

uint8_t send_tcp_packet(tcp_socket_t *socket, uint8_t flags, void* payload, uint16_t size)
{
    tcp_packet_t *packet = kmalloc(sizeof(tcp_packet_t) + size);
    memset(packet, 0, sizeof(tcp_packet_t) + size);
    packet->tcp.flags = flags;
    packet->tcp.destination_port = bswap16(socket->remote_port);
    packet->tcp.source_port = bswap16(socket->port);
    packet->tcp.window_size = bswap16(socket->receive_buffer->get_free_space(socket->receive_buffer));
    packet->tcp.data_offset = sizeof(tcp_header_t) / 4; //in uint32_t
    // probably must be added mutex lock or we can go out of sync
    packet->tcp.seq_number = bswap32(socket->seq_number);
    packet->tcp.ack_number = bswap32(socket->ack_number);

    memcpy((void*)(packet + 1), payload, size);

    fill_tcp_checksum(packet, &socket->net_dev->ip4_addr, &socket->remote_host, size);

    if (flags & TCP_FLAG_SYN || flags & TCP_FLAG_FIN || flags & TCP_FLAG_RST)
    {
        __sync_fetch_and_add(&socket->seq_number, 1);
    }
    __sync_fetch_and_add(&socket->seq_number, size);

    send_ip4_packet(socket->net_dev, &socket->remote_host, PROTOCOL_TCP, packet, sizeof(tcp_packet_t) + size);

    kfree(packet);
    return TCP_SOCKET_SUCCESS;
}

uint32_t read_from_tcp_socket(tcp_socket_t *socket, void *buffer, uint32_t size, uint32_t timeout)
{
    uint32_t finish = get_pit_ticks() + timeout;
    while(finish > get_pit_ticks())
    {
        uint32_t received = socket->receive_buffer->get(socket->receive_buffer, buffer, size);
        if (received > 0)
        {
            return received;
        }
    }

    return 0;
}

uint8_t write_to_tcp_socket(tcp_socket_t *socket, void *buffer, uint32_t size)
{
    if (socket->state != TCP_CONNECTION_ESTABLISHED)
    {
        return TCP_SOCKET_WRONG_STATE;
    }

    uint8_t result = socket->transmit_buffer->add(socket->transmit_buffer, buffer, size);

    if (result == BUFFER_FULL)
    {
        return TCP_SOCKET_BUFFER_IS_FULL;
    }
    return TCP_SOCKET_SUCCESS;
}

uint8_t tcp_bind(network_device_t* net_dev, uint16_t port, tcp_socket_binder_t **out)
{
    mutex_lock(&tcp_binders_mutex);
    if (tcp_binders[port] != 0)
    {
        mutex_release(&tcp_binders_mutex);
        return TCP_SOCKET_PORT_BUSY;
    }

    tcp_binders[port] = kmalloc(sizeof(tcp_socket_binder_t));
    mutex_release(&tcp_binders_mutex);

    memset(tcp_binders[port], 0, sizeof(tcp_socket_binder_t));
    tcp_binders[port]->port = port;
    *out = tcp_binders[port];
    //debug("[tcp] port %i binded\n", port);
    return TCP_SOCKET_SUCCESS;
}

uint8_t tcp_connect(network_device_t *net_dev, ip4_addr_t *ip, uint16_t port, tcp_socket_binder_t *binder, tcp_socket_t **out)
{
    tcp_socket_t *socket = kmalloc(sizeof(tcp_socket_t));
    memset(socket, 0, sizeof(tcp_socket_t));
    socket->state = TCP_CONNECTION_SYN_SENT;
    socket->remote_port = port;
    socket->remote_host = *ip;
    socket->port = binder->port;
    socket->net_dev = net_dev;
    socket->ring = create_ring(TCP_SOCKET_RING_SIZE);
    socket->receive_buffer = create_buffer(TCP_SOCKET_BUFFER_SIZE);
    socket->transmit_buffer = create_buffer(TCP_SOCKET_BUFFER_SIZE);

    mutex_lock(&binder->mutex);
    add_to_list(binder->handshake, socket);
    mutex_release(&binder->mutex);

    send_tcp_packet(socket, TCP_FLAG_SYN, 0, 0);
    uint32_t finish = get_pit_ticks() + TCP_CONNECT_TIMEOUT;
    while(finish > get_pit_ticks())
    {
        if (socket->state == TCP_CONNECTION_ESTABLISHED)
        {
            *out = socket;
            return TCP_SOCKET_SUCCESS;
        }
    }
    return TCP_SOCKET_TIMEOUT;
}

uint8_t close_tcp_connection(tcp_socket_t *socket)
{
    if (socket->state != TCP_CONNECTION_ESTABLISHED)
    {
        return TCP_SOCKET_WRONG_STATE;
    }

    socket->state = TCP_CONNECTION_FIN_1;
    return TCP_SOCKET_SUCCESS;
}

uint8_t tcp_listen(tcp_socket_binder_t *binder)
{
    binder->is_listening = 1;
    //debug("[tcp] listens on port %i\n", binder->port);
    return TCP_SOCKET_SUCCESS;
}
