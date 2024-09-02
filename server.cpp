#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <vector>
#include <string>
#include <iostream>
using namespace std;

#include "common.hpp"
#include "helpers.hpp"

struct Client {
	int socket;
	bool connected;
	std::string id;
	std::vector<std::string> subscribed_topics;
};

void setupTCPSocket(int &tcp_socket, uint16_t port)
{
	/* Creating the socket */
	tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_socket < 0, "socket");

	/* make socket reusable */
	const int enable = 1;
	if (setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	/*
		Disable Nagle algorithm

		- Nagle algorithm is used in TCP to improve network efficiency
		  by combining small packets.
		- Disabling it is done after creating the socket, but before
		  using it for communication.
	*/
	int flag = 1;
	int rc = setsockopt(tcp_socket, IPPROTO_TCP, TCP_NODELAY, (const void*) &flag, sizeof(int));
	DIE(rc < 0, "nagle");

	/* Initializing the sockaddr_in structure */
	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);
	memset(&serv_addr, 0, socket_len);

	/* Populating the fields of the sockaddr_in structure */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	/* We associate the server's address with the created socket using bind */
	rc = bind(tcp_socket, (const struct sockaddr *)&serv_addr, socket_len);
	DIE(rc < 0, "bind");
}

void setupUDPSocket(int &udp_socket, uint16_t port)
{
	/* Creating the socket */
	udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udp_socket < 0, "socket");

	/* make socket reusable */
	const int enable = 1;
	if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	/* Initializing the sockaddr_in structure */
	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);
	memset(&serv_addr, 0, socket_len);

	/* Populating the fields of the sockaddr_in structure */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	/* We associate the server's address with the created socket using bind */
	int rc = bind(udp_socket, (const struct sockaddr *)&serv_addr, socket_len);
	DIE(rc < 0, "bind");
}

void run_server(int tcp_socket, int udp_socket)
{
	std::vector<Client> clients;
	std::vector<pollfd> poll_fds(3);

	struct chat_packet received_tcp_packet; // packet received from a TCP client
	struct udp_packet received_udp_packet;  // packet received from a UDP client

	/* set the TCP socket for listening */
	/*
		The 5 in the listen(tcp_socket, 5); function call refers to the backlog,
		which is the maximum number of pending connections that can be queued up
		before the system starts refusing new connections.

		The backlog value in the listen() function does not limit the total
		number of connections your server can handle. Instead, it controls
		how many pending connections can be queued before the server starts
		rejecting new connection attempts.
	*/
	int rc = listen(tcp_socket, 5);
	DIE(rc < 0, "listen");

	/* file descriptor for standard input (keyboard -> 'exit' command) */
	poll_fds[0].fd = STDIN_FILENO;
	poll_fds[0].events = POLLIN;
	/* file descriptor to monitor TCP socket */
	poll_fds[1].fd = tcp_socket;
	poll_fds[1].events = POLLIN;
	/* file descriptor to monitor UDP socket*/
	poll_fds[2].fd = udp_socket;
	poll_fds[2].events = POLLIN;

	while (1) {
		/* We wait to receive something on one of the sockets with no time limit (-1) */
		rc = poll(poll_fds.data(), poll_fds.size(), -1);
		DIE(rc < 0, "poll");

		/* check what we have received */
		for (long unsigned int i = 0; i < poll_fds.size(); i++) {
			if (poll_fds[i].revents & POLLIN) {
				/* we have something on this poll_fds[i].fd file descriptor */
				if (poll_fds[i].fd == tcp_socket) {
					/* I have received a connection request */

					struct sockaddr_in cli_addr;
					socklen_t cli_len = sizeof(cli_addr);
					/* After receiving a connection request on the listening socket, accept() function 
					   returns a new socket which will be used for communication. */
					const int newsockfd = accept(tcp_socket, (struct sockaddr *)&cli_addr, &cli_len);
					DIE(newsockfd < 0, "accept");

					/* Get the CLIENT_ID from the new connection */
					int rc = recv_all(newsockfd, &received_tcp_packet, sizeof(received_tcp_packet));
					DIE(rc <= 0, "recv_all");

					/* Check if a client with the same ID is already connected */
					bool already_connected = false;
					for (const auto& client : clients) {
						if (client.id == received_tcp_packet.message && client.connected == true) {
							cout << "Client " << client.id << " already connected.\n";
							already_connected = true;
							close(newsockfd);
							break;
						}
					}

					/* Check if the client was previosuly connected */
					for (auto& client : clients) {
						if (client.id == received_tcp_packet.message && client.connected == false) {
							client.socket = newsockfd;
							client.connected = true;
							already_connected = true;

							struct pollfd new_poll;
							new_poll.fd = newsockfd;
							new_poll.events = POLLIN;
							poll_fds.push_back(new_poll);

							cout << "New client " << received_tcp_packet.message << " connected from " <<
									inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port) << ".\n";
						}
					}

					/* Connect a completely new client */
					if (already_connected == false) {
						Client new_client;
						new_client.socket = newsockfd;
						new_client.connected = true;
						new_client.id = received_tcp_packet.message;
						clients.push_back(new_client);

						struct pollfd new_poll;
						new_poll.fd = newsockfd;
						new_poll.events = POLLIN;
						poll_fds.push_back(new_poll);

						cout << "New client " << received_tcp_packet.message << " connected from " <<
								inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port) << ".\n";
					}
				} else if (poll_fds[i].fd == udp_socket) {
					/* Send the message only to the subscribed clients */

					/* Get the message */
					rc = recvfrom(udp_socket, &received_udp_packet, sizeof(received_udp_packet), 0, NULL, NULL);
					// rc = the number of bytes actually read
					
					/* a client can only receive TCP messages */
					*((char *)&received_udp_packet + rc) = 0;
					struct chat_packet packet;
					memcpy(packet.message, &received_udp_packet, sizeof(received_udp_packet));

					/* See what clients are subscribed to the topic */
					for (long unsigned int j = 0; j < clients.size(); j++) {
						for (long unsigned int k = 0; k < clients[j].subscribed_topics.size(); k++) {
							if (clients[j].subscribed_topics[k] == received_udp_packet.topic &&
								clients[j].connected == true) {
									/* the client is connected and subscribed to the topic => send message */
									send_all(clients[j].socket, &packet, sizeof(packet));
							}
						}
					}
				} else if (poll_fds[i].fd == STDIN_FILENO) {
					/* we can only have the 'exit' command from STDIN on the server */
					char command[256];
					if (fgets(command, sizeof(command), stdin) != NULL) {
						command[strcspn(command, "\n")] = '\0'; // remove '\n'

						if (strcmp(command, "exit") == 0) {
							/* close all connections and end the program */
							for (long unsigned int j = 0; j < poll_fds.size(); j++) {
								close(poll_fds[j].fd);
							}
							exit(EXIT_SUCCESS);
						}
					}
				} else {
					/* received data from one of the client sockets */
					rc = recv_all(poll_fds[i].fd, &received_tcp_packet, sizeof(received_tcp_packet));
					DIE(rc < 0, "recv_all");

					if (rc == 0) {
						/* This means the client disconnected */
						for (auto& client : clients) {
							if (client.socket == poll_fds[i].fd) {
								cout << "Client " << client.id << " disconnected.\n";
								client.connected = false;
								client.socket = -1;
								break;
							}
						}

						close(poll_fds[i].fd);
						poll_fds[i].revents = 0;
						poll_fds.erase(poll_fds.begin() + i); // remove the i'th element
						i--;
					} else {
						/* client wants to subscribe/unsubscribe */

						char topic[55];

						/* get the topic from the string */
						bool want_to_subscribe = false;
						bool want_to_unsubscribe = false;
						if (strncmp(received_tcp_packet.message, "subscribe", 9) == 0) {
							sscanf(received_tcp_packet.message, "subscribe %s", topic);
							want_to_subscribe = true;
						} else if (strncmp(received_tcp_packet.message, "unsubscribe", 11) == 0) {
							sscanf(received_tcp_packet.message, "unsubscribe %s", topic);
							want_to_unsubscribe = true;
						}

						for (long unsigned int iter = 0; iter < clients.size(); iter++) {
							/* find the client that wants to subscribe/unsubscribe */
							if (clients[iter].socket == poll_fds[i].fd) {
								if (want_to_subscribe) {
									/* check if the client is not already subscribed to the topic */
									bool already_subscribed = false;
									for (long unsigned int k = 0; k < clients[iter].subscribed_topics.size(); k++) {
										if (clients[iter].subscribed_topics[k] == topic) {
											already_subscribed = true;
											break;
										}
									}

									/* subscribe the client to the topic if it is not already subscribed */
									if (already_subscribed == false) {
										clients[iter].subscribed_topics.push_back(topic);
									}
								} else if (want_to_unsubscribe) {
									for (long unsigned int l = 0; l < clients[iter].subscribed_topics.size(); l++) {
										if (clients[iter].subscribed_topics[l] == topic) {
											clients[iter].subscribed_topics.erase(clients[iter].subscribed_topics.begin() + l);
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("\n Usage: %s <port>\n", argv[0]);
		return 1;
	}

	/* Disable buffering */
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	/* Get the port */
	uint16_t port;
	int rc = sscanf(argv[1], "%hu", &port);
	DIE(rc != 1, "Given port is invalid!");
	// OBS: sscanf returns the number of successfully read elements => rc should be 1

	/* Open 2 sockets: one for TCP and one for UDP on the given port */
	int tcp_socket, udp_socket;
	setupTCPSocket(tcp_socket, port);
	setupUDPSocket(udp_socket, port);

	run_server(tcp_socket, udp_socket);

	return 0;
}
