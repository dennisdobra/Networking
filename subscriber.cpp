#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <iostream>
#include <iomanip>
using namespace std;

#include "common.hpp"
#include "helpers.hpp"

void setupSocket(int &sockfd, uint16_t port)
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	/* Disable Nagle */
    int flag = 1;
    int rc = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const void*) &flag, sizeof(int));
    DIE(rc < 0, "nagle");

	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);
	memset(&serv_addr, 0, socket_len);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "connect");
}

void run_client(int sockfd)
{
	char buf[MSG_MAXSIZE + 1];
	memset(buf, 0, MSG_MAXSIZE + 1);

	struct chat_packet recv_packet;

	struct pollfd fds[2];
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;
	fds[1].fd = sockfd;
	fds[1].events = POLLIN;

	int rc;
	while (1) {
		rc = poll(fds, 2, -1);
		DIE(rc < 0, "poll");

		/* Check events on STDIN file descriptor */
		if (fds[0].revents & POLLIN) {
			if (!fgets(buf, sizeof(buf), stdin) || isspace(buf[0])) {
                break;
            }

			/* Check if the client wants to disconnect */
			if (strcmp(buf, "exit\n") == 0) {
				close(sockfd);
				break;
			}

			/* Check if the client wants to subscribe to a certain topic */
			if (strncmp(buf, "subscribe", 9) == 0) {
				/* Get the topic */
				char topic[55];
				sscanf(buf, "subscribe %s", topic);

				/* copy the data from the buffer in the packet that will be sent */
				struct chat_packet packet;
				memcpy(packet.message, buf, sizeof(packet));

				/* send the TCP packet */
				send_all(sockfd, &packet, sizeof(packet));

				cout << "Subscribed to topic " << topic << '\n';
			}

			/* Check if the client wants to unsubscribe from a certain topic */
			if (strncmp(buf, "unsubscribe", 11) == 0) {
				/* Get the topic */
				char topic[55];
				sscanf(buf, "unsubscribe %s", topic);

				/* copy the data from the buffer in the packet that will be sent */
				struct chat_packet packet;
				memcpy(packet.message, buf, sizeof(packet));

				/* send the TCP packet */
				send_all(sockfd, &packet, sizeof(packet));

				cout << "Unsubscribed from topic " << topic << '\n';
			}
		}

		/* Check events on sockfd file descriptor */
		if (fds[1].revents & POLLIN) {
			/* receive a message from server = data about a topic
			   that the client is subscribed to */
			rc = recv_all(sockfd, &recv_packet, sizeof(recv_packet));
			if (rc <= 0) {
				break;
			}

			struct udp_packet udp_message = *(struct udp_packet*)recv_packet.message;

			/* parse the udp_message */
			if (udp_message.data_type == 0) {
                /* get the sign byte */
                uint8_t sign_byte = (uint8_t)udp_message.message[0];

                int32_t value;
                memcpy(&value, udp_message.message + sizeof(uint8_t), sizeof(uint32_t));
                value = ntohl(value);

                if (sign_byte == 1) {
                    value = -value;
                    cout << udp_message.topic << " - INT - " << value << '\n';
                } else {
                    cout << udp_message.topic << " - INT - " << value << '\n';
                }
            } else if (udp_message.data_type == 1) {
                uint16_t value;

                memcpy(&value, udp_message.message, sizeof(uint16_t));
                value = ntohs(value);

                cout << fixed << setprecision(2);
                cout << udp_message.topic << " - SHORT_REAL - " << (float) value / 100 << '\n'; 
            } else if (udp_message.data_type == 2) {
                uint8_t sign_byte = (uint8_t)udp_message.message[0];

                int32_t val;
                memcpy(&val, udp_message.message + sizeof(uint8_t), sizeof(int32_t));

                uint8_t pow;
                memcpy(&pow, udp_message.message + sizeof(uint8_t) + sizeof(int32_t), sizeof(uint8_t));

                float final_val = ntohl(val);

                for (int i = 0; i < pow; i++)
                    final_val /= 10;

                if (sign_byte == 1) {
                    final_val = -final_val;
                }

                cout << fixed << setprecision(pow);
                cout << udp_message.topic << " - FLOAT - " << final_val << '\n';            
            } else if (udp_message.data_type == 3) {
                udp_message.topic[strlen(udp_message.topic)] = '\0';
                cout << udp_message.topic << " - STRING - " << (char *)udp_message.message << '\n';
            }
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		printf("\n Usage: %s <id_client> <ip_server> <port>\n", argv[0]);
		return 1;
	}

	/* Disable buffering */
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	/* Get the port */
	uint16_t port;
	int rc = sscanf(argv[3], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	/* set TCP socket */
	int sockfd;
	setupSocket(sockfd, port);

	/* send Client_ID on sockfd */
	struct chat_packet id_packet;
	id_packet.len = strlen(argv[1]);
	strcpy(id_packet.message, argv[1]);
	send_all(sockfd, &id_packet, sizeof(id_packet));

	/* run client */
	run_client(sockfd);

	close(sockfd);

	return 0;
}
