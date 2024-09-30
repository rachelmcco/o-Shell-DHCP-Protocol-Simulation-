#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "dhcp.h"
#include "format.h"
#include "port_utils.h"

static bool debug = false;

int main(int argc, char **argv)
{
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    int counter = 0;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address = {0};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    char *p = get_port();
    server_address.sin_port = htons(atoi(p));
    bind(sock, (const struct sockaddr *)&server_address, sizeof(server_address));
    socklen_t addr_len = sizeof(server_address);

    char *addr = "10.0.2.";
    char result[9];
    sprintf(result, "%s%d", addr, counter);

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    uint8_t *response = calloc(MAX_DHCP_LENGTH, sizeof(uint8_t));
    ssize_t rbytes;

    while (1)
    {
        rbytes = recvfrom(sock, response, MAX_DHCP_LENGTH, 0, (struct sockaddr *)&server_address, &addr_len);
        if (rbytes < 0)
            break;

        msg_t msg;
        memcpy(&msg, response, sizeof(msg));
        uint8_t dhcp_msg;

        printf("++++++++++++++++\nSERVER RECEIVED:\n");
        dump_packet(response, rbytes);
        dump_options(response, rbytes, true);

        msg.op = BOOTREPLY;

        counter++;
        // printf("COUNTER: %d", counter);
        if (counter <= 10)
        {
            addr = "10.0.2.";
            sprintf(result, "%s%d", addr, counter);
            inet_aton(result, &msg.yiaddr);
            dhcp_msg = 2;
            uint8_t server_ip[4] = {10, 0, 2, 0};
            uint8_t req_ip[4] = {10, 0, 2, 1};
            uint8_t lease_time[4] = {0x00, 0x27, 0x8d, 0x00};
            set_options(&msg, dhcp_msg, server_ip, req_ip, lease_time);
        }
        else
        {
            addr = "10.0.2.1";
            inet_aton(addr, &msg.yiaddr);
            dhcp_msg = 6;
            uint8_t server_ip[4] = {10, 0, 2, 0};
            uint8_t req_ip[4] = {10, 0, 2, 1};
            uint8_t lease_time[4] = {0x00, 0x27, 0x8d, 0x00};
            set_options(&msg, dhcp_msg, server_ip, req_ip, lease_time);
        }
        dhcp_msg = 5;
        addr = "10.0.2.";
        sprintf(result, "%s%d", addr, counter);
        inet_aton(result, &msg.yiaddr);
        uint8_t server_ip[4] = {10, 0, 2, 0};
        uint8_t req_ip[4] = {10, 0, 2, 1};
        uint8_t lease_time[4] = {0x00, 0x27, 0x8d, 0x00};
        set_options(&msg, dhcp_msg, server_ip, req_ip, lease_time);

        uint8_t *message = (uint8_t *)&msg;
        int size = 236;

        if (dhcp_msg == 1)
            size += 8;
        else if (dhcp_msg == 2 || dhcp_msg == 3 || dhcp_msg == 4 || dhcp_msg == 5)
            size += 20;
        else if (dhcp_msg == 6 || dhcp_msg == 7)
            size += 14;

        printf("++++++++++++++++\nSERVER SENDING:\n");
        
        dump_packet(message, size);
        dump_options(message, size, true);

        sendto(sock, message, size, 0, (struct sockaddr *)&server_address, sizeof(server_address));
    }

    counter = 0;
    close(sock);

    if (debug)
        fprintf(stderr, "Shutting down\n");

    return EXIT_SUCCESS;
}