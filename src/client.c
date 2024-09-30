#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "dhcp.h"
#include "format.h"
#include "port_utils.h"
#include <sys/time.h>




int dhcp_ptr(msg_t *msg, uint32_t xid, uint8_t htype, uint8_t *chaddr, int len, uint8_t dhcp_msg, uint8_t *server_ip, uint8_t *req_ip, int initiate_protocol);
int set_options(msg_t *msg, uint8_t dhcp_msg, uint8_t *server_ip, uint8_t *req_ip, uint8_t *lease_time);


int main(int argc, char **argv)
{
  uint32_t xid = 42;
  uint8_t htype = ETH;
  uint8_t chaddr[MAX_DHCP_CHADDR_LENGTH] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
  uint8_t dhcp_msg = DHCPDISCOVER;
  uint8_t server_ip[4] = {127, 0, 0, 1};
  uint8_t req_ip[4] = {127, 0, 0, 2};
  int size = 236;
  int initiate_protocol = 0;
  int hlen = 6;

  for (int i = 0; i < argc; i++)
  {
    if (strcmp(argv[i], "-x") == 0 && i + 1 < argc)
    {
      xid = (uint32_t)strtoul(argv[i + 1], NULL, 10);
      i++;
    }
    else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc)
    {
      int type = (int)strtol(argv[i + 1], NULL, 10);
      if (type == ETH || type == IEEE802 || type == ARCNET || type == FRAME_RELAY || type == FIBRE)
      {
        htype = type;
      }
      switch (htype)
      {
      case 1:
        hlen = ETH_LEN;
        break;
      case IEEE802:
        hlen = IEEE802_LEN;
        break;
      case ARCNET:
        hlen = ARCNET_LEN;
        break;
      case FRAME_RELAY:
        hlen = FRAME_LEN;
        break;
      case FIBRE:
        hlen = FIBRE_LEN;
        break;
      }
      i++;
    }
    else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc)
    {
      int len = strlen(argv[i + 1]);
      for (int k = 0; k < len / 2 && k < MAX_DHCP_CHADDR_LENGTH; k++)
      {
        sscanf(argv[i + 1] + 2 * k, "%2hhx", &chaddr[k]);
      }
      i++;
    }
    else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc)
    {
      dhcp_msg = (uint8_t)strtol(argv[i + 1], NULL, 10);
      i++;
    }
    else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc)
    {
      inet_pton(AF_INET, argv[i + 1], server_ip);
      i++;
    }
    else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc)
    {
      inet_pton(AF_INET, argv[i + 1], req_ip);
      i++;
    }
    else if (strcmp(argv[i], "-p") == 0)
    {
      initiate_protocol = 1;
    }
  }

  msg_t msg;

  dhcp_ptr(&msg, xid, htype, chaddr, hlen, dhcp_msg, server_ip, req_ip, initiate_protocol);

  if (dhcp_msg == 01)
    size += 8;
  else if (dhcp_msg == 02 || dhcp_msg == 03 || dhcp_msg == 04 || dhcp_msg == 05)
    size += 20;
  else if (dhcp_msg == 06 || dhcp_msg == 07) 
    size += 14;

  dump_packet((uint8_t *)&msg, sizeof(msg));

  struct in_addr server_addr;
  memcpy(&server_addr, server_ip, sizeof(server_addr));
  // Print other options first
  dump_options((uint8_t *)&msg, size, false);


  if (initiate_protocol)
  {
    if (xid != 0)
    {
      printf("++++++++++++++++\nCLIENT RECEIVED:\n");
      int socketfd = socket(AF_INET, SOCK_DGRAM, 0);
      struct sockaddr_in address = {0};
      address.sin_family = AF_INET;
      address.sin_port = htons((uint16_t)strtol(get_port(), NULL, 10));
      address.sin_addr = (struct in_addr){0x100007f};

      uint8_t *message = (uint8_t *)&msg;

      sendto(socketfd, message, size, 0, (struct sockaddr *)&address, sizeof(address));
      uint8_t *response = calloc(MAX_DHCP_LENGTH, sizeof(uint8_t));
      ssize_t received_bytes = recvfrom(socketfd, response, MAX_DHCP_LENGTH, 0, NULL, NULL);

      msg_t msg;
      memcpy(&msg, response, sizeof(msg));

      message = (uint8_t *)&msg;
      dump_packet((uint8_t *)&msg, sizeof(msg));
      dump_options(message, received_bytes, (received_bytes > 0));

      int intValue = (int)msg.options[6];
      printf("++++++++++++++++\n");
      if (intValue != 6)
      {
        msg.op = 1;

          char formatted_server_ip[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, server_ip, formatted_server_ip, INET_ADDRSTRLEN);

        char yiaddr_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(msg.yiaddr), yiaddr_str, INET_ADDRSTRLEN);

        int lastOctet = ((unsigned char *)&(msg.yiaddr))[3];

        inet_aton("0.0.0.0", &msg.yiaddr);
        uint8_t dhcp_msg = 3;
        uint8_t server_ip[16] = {0x0a, 0x00, 0x02, 0x00};
        uint8_t req_ip[16] = {0x0a, 0x00, 0x02, lastOctet};
        uint8_t lease_time[16] = {0x00, 0x27, 0x8d, 0x00};
        set_options(&msg, dhcp_msg, server_ip, req_ip, lease_time);

        size = 256;
        message = (uint8_t *)&msg;

        sendto(socketfd, message, size, 0, (struct sockaddr *)&address, sizeof(address));

        dump_packet((uint8_t *)&msg, sizeof(msg));
        

        dump_options(message, received_bytes, (received_bytes > 0));

        response = calloc(MAX_DHCP_LENGTH, sizeof(uint8_t));
        received_bytes = recvfrom(socketfd, response, MAX_DHCP_LENGTH, 0, NULL, NULL);

        memcpy(&msg, response, sizeof(msg));
        inet_aton(yiaddr_str, &msg.yiaddr);
        message = (uint8_t *)&msg;
        printf("++++++++++++++++\nCLIENT RECEIVED:\n");
        dump_packet((uint8_t *)&msg, sizeof(msg));

        dump_options(message, received_bytes, (received_bytes > 0));
        printf("++++++++++++++++\n");

      }
      close(socketfd);

      free(response);
      close(socketfd);
    }
    else
    {
      int socketfd = socket(AF_INET, SOCK_DGRAM, 0);
      struct sockaddr_in address = {0};
      address.sin_family = AF_INET;
      address.sin_port = htons((uint16_t)strtol(get_port(), NULL, 10));
      address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

      uint8_t buffer[sizeof(msg)];

      memcpy(buffer, &msg, sizeof(msg));

      // size of bootp header(236) and the size of options
      // options_size += 236;
      if (sendto(socketfd, buffer, size, 0, (struct sockaddr *)&address, sizeof(address)) < 0)
      {
        perror("Sendto failed");
        close(socketfd);
        return EXIT_FAILURE;
      }

      close(socketfd);
    }
  }

  return EXIT_SUCCESS;
}

int dhcp_ptr(msg_t *msg, uint32_t xid, uint8_t htype, uint8_t *chaddr, int len, uint8_t dhcp_msg, uint8_t *server_ip, uint8_t *req_ip, int initiate_protocol)
{
  memset(msg, 0, sizeof(msg_t));
  msg->op = BOOTREQUEST;
  msg->htype = htype;
  msg->hlen = len;
  msg->xid = htonl(xid);
  memcpy(msg->chaddr, chaddr, len);
  uint8_t lease_time[16] = {0x00, 0x27, 0x8d, 0x00};

  int index_value = set_options(msg, dhcp_msg, server_ip, req_ip, lease_time);
  return index_value;
}


