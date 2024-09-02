#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);

/* Maximum size of the TCP message */
#define MSG_MAXSIZE 2000

/* structure for TCP message */
struct chat_packet {
  uint16_t len;
  char message[MSG_MAXSIZE + 1];
};

/* Maximum size of the UDP topic */
#define MAX_TOPIC_SIZE 50

/* Maximum size of the UDP message */
#define MSG_UDP_MAXSIZE 1501

/* structure for UDP message */
struct udp_packet {
  char topic[MAX_TOPIC_SIZE];
  uint8_t data_type;
  char message[MSG_UDP_MAXSIZE];
};

#endif
