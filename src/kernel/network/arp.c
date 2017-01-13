#include "arp.h"
#include "network.h"
#include "liballoc.h"
#include "string.h"
#include "debug.h"
#include "timer.h"
#include "mutex.h"

// probably arp_cache must be in net_dev, but i have no plans to support machines with 2+ network cards
arp_entry_t *arp_cache = 0;
mutex_t arp_cache_mutex = 0;

void process_arp_request(network_device_t *net_dev, void *packet)
{
    arp_packet_t *arp_packet = packet;
    if (COMPARE_IP4_ADDR(net_dev->ip4_addr, arp_packet->arp.destination_ip))
    {
        // TODO: must be in separate thread or not wait for packet send, because it lock receive thread
        debug("[arp] Got ARP request for my IP\n");
        arp_packet_t *reply = kmalloc(sizeof(arp_packet_t));
        memset(reply, 0, sizeof(arp_packet_t));
        reply->arp.hardware_type = bswap16(1);
        reply->arp.protocol = bswap16(PROTOCOL_IP);
        reply->arp.hardware_address_length = ETH_ADDR_LENGTH;
        reply->arp.protocol_length = IP4_ADDR_LENGTH;
        reply->arp.operation = bswap16(ARP_REPLY);
        reply->arp.source = net_dev->mac;
        reply->arp.source_ip = net_dev->ip4_addr;
        reply->arp.destination_ip = arp_packet->arp.source_ip;
        reply->arp.destination = arp_packet->arp.source;

        reply->eth.destination = arp_packet->arp.source;
        reply->eth.source = net_dev->mac;
        reply->eth.type = bswap16(PROTOCOL_ARP);

        net_dev->send_packet(net_dev, reply, sizeof(arp_packet_t));
        kfree(reply);
    }
}

void send_arp_request(network_device_t *net_dev, ip4_addr_t *ip)
{
    eth_addr_t mac_empty = ETH_ADDR_EMPTY;

    arp_packet_t *packet = kmalloc(sizeof(arp_packet_t));
    memset(packet, 0, sizeof(arp_packet_t));
    packet->arp.hardware_type = bswap16(ARP_HARDWARE_TYPE_ETHERNET);
    packet->arp.protocol = bswap16(PROTOCOL_IP);
    packet->arp.hardware_address_length = ETH_ADDR_LENGTH;
    packet->arp.protocol_length = IP4_ADDR_LENGTH;
    packet->arp.operation = bswap16(ARP_REQUEST);
    packet->arp.source = net_dev->mac;
    packet->arp.source_ip = net_dev->ip4_addr;
    packet->arp.destination_ip = *ip;
    packet->arp.destination = mac_empty;

    eth_addr_t mac_broadcast = ETH_ADDR_BROADCAST;
    packet->eth.destination = mac_broadcast;
    packet->eth.source = net_dev->mac;
    packet->eth.type = bswap16(PROTOCOL_ARP);

    net_dev->send_packet(net_dev, packet, sizeof(arp_packet_t));
}

void add_mac_to_arp_cache(ip4_addr_t *ip, eth_addr_t *mac)
{
    mutex_lock(&arp_cache_mutex);
    arp_entry_t *current = arp_cache;
    while(current != 0)
    {
        if (COMPARE_IP4_ADDR(current->ip_addr, *ip))
        {
            current->mac = *mac;
            mutex_release(&arp_cache_mutex);
            return;
        }
        current = (arp_entry_t*)current->list.next;
    }
    arp_entry_t *node = kmalloc(sizeof(arp_entry_t));
    memset(node, 0, sizeof(arp_entry_t));
    node->ip_addr = *ip;
    node->mac = *mac;
    add_to_list(arp_cache, node);
    mutex_release(&arp_cache_mutex);
}

uint8_t get_mac_from_cache(const ip4_addr_t *ip, eth_addr_t *out)
{
    mutex_lock(&arp_cache_mutex);
    arp_entry_t *current = arp_cache;
    while(current != 0)
    {
        if (COMPARE_IP4_ADDR(current->ip_addr, *ip))
        {
            *out = current->mac;
            mutex_release(&arp_cache_mutex);
            return 1;
        }
        current = (arp_entry_t*)current->list.next;
    }
    mutex_release(&arp_cache_mutex);
    return 0;
}

uint8_t get_mac_by_ip(network_device_t *net_dev, ip4_addr_t *ip, eth_addr_t *out)
{
    ip4_addr_t broadcast = IP4_ADDR_BROADCAST;
    if (COMPARE_IP4_ADDR(*ip, broadcast))
    {
        eth_addr_t eth_broadcast = ETH_ADDR_BROADCAST;
        *out = eth_broadcast;
        return 1;
    }

    ip4_addr_t needed = *ip;

    if (!COMPARE_SUBNETS(*ip, net_dev->ip4_addr, net_dev->subnet_mask))
    {
        needed = net_dev->router;
    }

    uint8_t found = get_mac_from_cache(&needed, out);
    if (found)
    {
        return 1;
    }

    uint8_t tries = 3;
    while(tries > 0)
    {
        send_arp_request(net_dev, &needed);
        uint16_t sub_tries = 100;
        while(sub_tries > 0)
        {
            found = get_mac_from_cache(&needed, out);
            if (found)
            {
                return 1;
            }
            sleep(10);
            sub_tries--;
        }
        tries--;
    }
    return 0;
}
