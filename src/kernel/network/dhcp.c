#include "dhcp.h"
#include "udp.h"
#include "liballoc.h"
#include "arp.h"
#include "string.h"
#include "debug.h"

#define DHCP_RANDOM_NUMBER 0x1863A3F6
#define DHCP_OPTIONS_SIZE 100

uint32_t extract_option(uint8_t *options, uint8_t id)
{
    while(*options != 255)
    {
        if(*options == 0) // NOP option
        {
            options++;
            continue;
        }

        if (*options == id)
        {
            uint32_t value = 0;
            options++;
            uint8_t size = *options;
            options++;
            for(uint8_t i = 0; i < size; i++)
            {
                value = value | (*options << (i * 8));
                options++;
            }
            return value;
        }

        options++;
        options += *options + 1; //next item (size) +1 (because we need index, not count of bytes in current value)
    }

    return 0;
}

uint8_t validate_dhcp_response(dhcp_packet_t *response, uint8_t message_type)
{
    if (response == 0)
    {
        return DHCP_NO_REPLY;
    }

    uint32_t cookie = bswap32(*(uint32_t*)((void*)&response->dhcp + sizeof(dhcp_header_t))); // dhcp_header_t + 1 and cast to integer
    if (response->dhcp.code != DHCP_RESPONSE || bswap32(response->dhcp.transaction_id) != DHCP_RANDOM_NUMBER || cookie != 0x63825363)
    {
        return DHCP_WRONG_TRANSACTION;
    }

    uint8_t dhcp_message_type = extract_option(((void*)&response->dhcp + sizeof(dhcp_header_t)) + 4, 53); // +4 because of cookie
    if (dhcp_message_type != message_type)
    {
        return DHCP_WRONG_MESSAGE_TYPE;
    }

    return DHCP_OK;
}

uint8_t configure_dhcp(network_device_t *net_dev)
{
    udp_socket_t *socket = 0;
    uint8_t result = create_udp_socket(net_dev, 68, &socket);
    if (result != UDP_SOCKET_SUCCESS)
    {
        debug("[dhcp] can't create udp socket, status: %i\n", result);
        return DHCP_SOCKET_ERROR;
    }

    dhcp_header_t *header = kmalloc(sizeof(dhcp_header_t) + DHCP_OPTIONS_SIZE);
    memset(header, 0, sizeof(dhcp_header_t) + DHCP_OPTIONS_SIZE);
    header->code = 0x01;
    header->hardware_type = 0x01;
    header->length = 0x06;
    header->hops = 0x00;
    header->transaction_id = bswap32(DHCP_RANDOM_NUMBER);
    header->flags = bswap16(0x8000);
    header->client_mac = net_dev->mac;

    uint8_t *options = (uint8_t*)(header + 1);
    *(uint32_t*)options = bswap32(0x63825363);
    options += 4;
    *options++ = 53; // request type
    *options++ = 1;
    *options++ = 1; // discover
    *options++ = 55; // parameters request
    *options++ = 2;
    *options++ = 1; // subnet mast
    *options++ = 3; // router
    *options++ = 255; // end of options

    ip4_addr_t dest = IP4_ADDR_BROADCAST;
    send_udp_packet(socket, &dest, 67, header, (uint32_t)options - (uint32_t)header);

    dhcp_packet_t *response = 0;
    result = receive_udp_packet(socket, 3000, (void*)&response);
    if (result != UDP_SOCKET_SUCCESS)
    {
        debug("[dhcp] no reply from server\n");
        release_udp_socket(socket);
        kfree(header);
        kfree(response);
        return DHCP_NO_REPLY;
    }

    uint8_t valid = validate_dhcp_response(response, DHCP_TYPE_OFFER);
    if (valid != DHCP_OK)
    {
        debug("[dhcp] configure error: DISCROVER response error %i\n", valid);
        release_udp_socket(socket);
        kfree(header);
        kfree(response);
        return valid;
    }

    ip4_addr_t my_ip = response->dhcp.your_ip;

    void *response_options = ((void*)&response->dhcp + sizeof(dhcp_header_t)) + 4;
    uint32_t value = extract_option(response_options, 1);
    ip4_addr_t subnet_mask = *(ip4_addr_t*)&value;
    value = extract_option(response_options, 3);
    ip4_addr_t default_router = *(ip4_addr_t*)&value;
    uint32_t tmp = extract_option(response_options, 54);
    ip4_addr_t dhcp_server = *(ip4_addr_t*)&tmp;

    kfree(response);
    response = 0;

    options = (uint8_t*)(header + 1);
    memset(options, 0, 100);
    *(uint32_t*)options = bswap32(0x63825363);
    options += 4;
    *options++ = 53;
    *options++ = 1;
    *options++ = 3; // dhcp request
    *options++ = 50;
    *options++ = 4;
    *(ip4_addr_t*)options = my_ip;
    options += 4;
    *options++ = 54;
    *options++ = 4;
    *(ip4_addr_t*)options = dhcp_server;
    options += 4;
    *options++ = 255;
    send_udp_packet(socket, &dest, 67, header, (uint32_t)options - (uint32_t)header);

    result = receive_udp_packet(socket, 3000, (void*)&response);
    if (result != UDP_SOCKET_SUCCESS)
    {
        debug("[dhcp] no reply from server\n");
        release_udp_socket(socket);
        kfree(header);
        kfree(response);
        return DHCP_NO_REPLY;
    }

    valid = validate_dhcp_response(response, DHCP_TYPE_ACK);
    if (valid != DHCP_OK)
    {
        debug("[dhcp] configure error: DISCROVER response error %i\n", valid);
        release_udp_socket(socket);
        kfree(header);
        kfree(response);
        return valid;
    }

    eth_addr_t mac;
    uint8_t arp_result = get_mac_by_ip(net_dev, &my_ip, &mac);
    if (arp_result != 0)
    {
        debug("[dhcp] IP address assigned by DHCP server is already in use.\n");
        release_udp_socket(socket);
        kfree(header);
        kfree(response);
        return DHCP_INVALID_IP;
    }

    mutex_lock(&net_dev->mutex);
    net_dev->subnet_mask = subnet_mask;
    net_dev->router = default_router;
    net_dev->ip4_addr = my_ip;
    mutex_release(&net_dev->mutex);

    release_udp_socket(socket);
    kfree(header);
    kfree(response);
    return DHCP_OK;
}
