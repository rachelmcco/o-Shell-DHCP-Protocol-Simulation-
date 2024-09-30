#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dhcp.h"
#include <arpa/inet.h>
#define DHCP_OPTION_SERVER_IDENTIFIER 54

void print_dhcp_message(msg_t *dhcp_msg) {
    const char *message_type;
    switch (dhcp_msg->op) {
        case BOOTREQUEST:
            message_type = "BOOTREQUEST";
            break;
        case BOOTREPLY:
            message_type = "BOOTREPLY";
            break;
        default:
            message_type = "UNKNOWN";
            break;
    }

    const char *hardware_type;
    switch (dhcp_msg->htype) {
        case 1:
            hardware_type = "Ethernet (10Mb)";
            break;
        case 6:
            hardware_type = "IEEE 802 Networks";
            break;
        case 7:
            hardware_type = "ARCNET";
            break;
        case 15:
            hardware_type = "Frame Relay";
            break;
        case 18:
            hardware_type = "Fibre Channel";
            break;
        case 23:
            hardware_type = "Synchronous Serial";
            break;
        default:
            hardware_type = "UNKNOWN";
            break;
    }

    dhcp_msg->secs = htons(dhcp_msg->secs);

    uint8_t days = dhcp_msg->secs / 86400;
    uint8_t hours = (dhcp_msg->secs - (days * 86400)) / 3600;
    uint8_t minutes = (dhcp_msg->secs - (hours * 3600) - (days * 86400)) / 60;
    uint8_t seconds = dhcp_msg->secs - (minutes * 60) - (hours * 3600) - (days * 86400);

    printf("------------------------------------------------------\n");
    printf("BOOTP Options\n");
    printf("------------------------------------------------------\n");
    printf("Op Code (op) = %d [%s]\n", dhcp_msg->op, message_type);
    printf("Hardware Type (htype) = %d [%s]\n", dhcp_msg->htype, hardware_type);
    printf("Hardware Address Length (hlen) = %d\n", dhcp_msg->hlen);
    printf("Hops (hops) = %d\n", dhcp_msg->hops);
    printf("Transaction ID (xid) = %u (0x%x)\n", ntohl(dhcp_msg->xid), ntohl(dhcp_msg->xid));
    printf("Seconds (secs) = %d Days, %01d:%02d:%02d\n", days, hours, minutes, seconds);
    printf("Flags (flags) = %d\n", ntohs(dhcp_msg->flags));
    printf("Client IP Address (ciaddr) = %s\n", inet_ntoa(dhcp_msg->ciaddr));
    printf("Your IP Address (yiaddr) = %s\n", inet_ntoa(dhcp_msg->yiaddr));
    printf("Server IP Address (siaddr) = %s\n", inet_ntoa(dhcp_msg->siaddr));
    printf("Relay IP Address (giaddr) = %s\n", inet_ntoa(dhcp_msg->giaddr));
    printf("Client Ethernet Address (chaddr) = ");
    for (int i = 0; i < dhcp_msg->hlen; i++) {
        printf("%02x", dhcp_msg->chaddr[i]);
    }
    printf("\n");
}

void dump_packet(uint8_t *ptr, size_t size) {
    msg_t dhcp_msg;
    memcpy(&dhcp_msg, ptr, sizeof(msg_t));
    print_dhcp_message(&dhcp_msg);
}


void dump_options(uint8_t *ptr, size_t size, bool from_server)
{
    printf("------------------------------------------------------\n");
    printf("DHCP Options\n");
    printf("------------------------------------------------------\n");
    printf("Magic Cookie = [OK]\n");

    uint8_t *options_ptr = ptr + 240;
    struct in_addr server_ip;
    bool server_identifier_printed = false;

    while (*options_ptr != 255)
    {
        uint8_t option_code = *options_ptr;
        uint8_t option_len = *(options_ptr + 1);
        uint8_t *option_data = options_ptr + 2;

        switch (option_code)
        {
        case 53:
            printf("Message Type = DHCP ");
            switch (*option_data)
            {
            case DHCPDISCOVER:
                printf("Discover\n");
                break;
            case DHCPOFFER:
                printf("Offer\n");
                break;
            case DHCPREQUEST:
                printf("Request\n");
                break;
            case DHCPDECLINE:
                printf("Decline\n");
                break;
            case DHCPACK:
                printf("ACK\n");
                break;
            case DHCPNAK:
                printf("NAK\n");
                break;
            case DHCPRELEASE:
                printf("Release\n");
                break;
            default:
                printf("Unknown\n");
                break;
            }
            break;
        case 51: // Lease Time
        {
            uint32_t time = 0;
            memcpy(&time, option_data, sizeof(uint32_t));
            time = ntohl(time);

            uint32_t days = time / 86400;
            uint32_t hours = (time - (days * 86400)) / 3600;
            uint32_t minutes = (time - (hours * 3600) - (days * 86400)) / 60;
            uint32_t seconds = (time - (minutes * 60) - (hours * 3600) - (days * 86400));
            printf("IP Address Lease Time = %u Days, %01u:%02u:%02u\n", days, hours, minutes, seconds);
            break;
        }
        case 50: // Requested IP
            struct in_addr requested_ip;
            memcpy(&requested_ip, option_data, sizeof(struct in_addr));
            printf("Request = %s\n", inet_ntoa(requested_ip));
            break;
        case 54: // Server ID
            if (!server_identifier_printed)
            {
                memcpy(&server_ip, option_data, sizeof(struct in_addr));
                server_identifier_printed = true;
            }
            break;
        default:
            break;
        }

        options_ptr += option_len + 2;
    }

    if (server_identifier_printed)
    {
        printf("Server Identifier = %s\n", inet_ntoa(server_ip));
    }
}

int set_options(msg_t *msg, uint8_t dhcp_msg, uint8_t *server_ip, uint8_t *req_ip, uint8_t *lease_time)
{
    uint8_t options[MAX_DHCP_OPTIONS_LENGTH];
    int index = 0;

    // Magic Cookie
    options[index++] = 0x63;
    options[index++] = 0x82;
    options[index++] = 0x53;
    options[index++] = 0x63;

    // Message Type
    options[index++] = 0x35;
    options[index++] = 0x01;
    options[index++] = dhcp_msg;

    // Requested IP Address
    if (dhcp_msg == DHCPREQUEST || dhcp_msg == DHCPDECLINE)
    {
        options[index++] = 0x32;
        options[index++] = 0x04;
        memcpy(&options[index], req_ip, 4);
        index += 4;
    }

    // Lease Time
    if (dhcp_msg == DHCPACK || dhcp_msg == DHCPNAK)
    {
        options[index++] = 0x33;
        options[index++] = 0x04;
        memcpy(&options[index], lease_time, 4);
        index += 4;
    }

    // Server IP Address
    if (dhcp_msg != DHCPDISCOVER && dhcp_msg != 8)
    {
        options[index++] = DHCP_OPTION_SERVER_IDENTIFIER;
        options[index++] = 0x04;
        memcpy(&options[index], server_ip, 4);
        index += 4;
    }

    // End of options
    options[index++] = 0xFF;

    memcpy(msg->options, options, index);

    return index;
}
