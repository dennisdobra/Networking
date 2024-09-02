#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);

// struct Client {
//   int socket;
//   int connected;
//   std::string id;
//   std::vector<std::string> subscribed_topics;
// };

/* Dimensiunea maxima a mesajului TCP */
#define MSG_MAXSIZE 2000

/* structure for TCP message */
struct chat_packet {
  uint16_t len;
  char message[MSG_MAXSIZE + 1];
};

/* Dimensiunea maxima a topicului UDP */
#define MAX_TOPIC_SIZE 50

/* Dimensiunea maxima a mesajului UDP */
#define MSG_UDP_MAXSIZE 1501

/* structure for UDP message */
struct udp_packet {
  char topic[MAX_TOPIC_SIZE];
  uint8_t data_type;
  char message[MSG_UDP_MAXSIZE];
};

#endif
