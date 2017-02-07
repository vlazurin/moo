#include "network.h"
#include "ip.h"
#include "log.h"
#include "arp.h"
#include "string.h"

void send_ip4_packet(network_device_t *net_dev, ip4_addr_t* destination, uint8_t protocol, void *packet, size_t size)
{
    ip_packet_t *ip_packet = packet;
    memset(&ip_packet->ip, 0, sizeof(ip4_header_t));
    ip_packet->ip.version = IP_VERSION;
    ip_packet->ip.header_size = sizeof(ip4_header_t) / 4;
    ip_packet->ip.total_size = bswap16(size - sizeof(eth_header_t));
    ip_packet->ip.ttl = IP_DEFAULT_TTL;
    ip_packet->ip.protocol = protocol;
    ip_packet->ip.destination_ip = *destination;
    ip_packet->ip.source_ip = net_dev->ip4_addr;

    uint32_t checksum = 0;
    uint16_t *p = (uint16_t*)&ip_packet->ip;
    for(uint8_t i = 0; i < sizeof(ip4_header_t) / 2; i++)
    {
        checksum += *p; // we dont worry about checksum field, because it's 0
        while(checksum > 0xFFFF)
        {
            checksum = (checksum >> 16) + (checksum & 0x0FFFF);
        }
        p++;
    }
    checksum = ~checksum;
    ip_packet->ip.header_checksum = (uint16_t)checksum;

    eth_addr_t destination_mac;
    uint8_t success = get_mac_by_ip(net_dev, destination, &destination_mac);
    if (success == 0)
    {
        char addr[16];
        ip4_to_str(destination, addr);
        //debug("[ip] MAC address not found for %s\n", addr);
        return;
    }
    ip_packet->eth.destination = destination_mac;
    ip_packet->eth.source = net_dev->mac;
    ip_packet->eth.type = bswap16(PROTOCOL_IP);

    net_dev->send_packet(net_dev, ip_packet, size);
}
