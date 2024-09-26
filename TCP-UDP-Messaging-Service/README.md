# TCP AND UDP CLIENT-SERVER APPLICATION FOR MESSAGE MANAGEMENT 

# Objectives of the project
The purpose of this assignment is to develop an application following the client-server model for managing messages.
The objectives are:
 - Understanding the mechanisms used for developing network applications using TCP and UDP;
 - Multiplexing TCP and UDP connections;
 - Defining and using a data type for a predefined protocol over UDP;
 - Developing a client-server application using sockets.

# General Description
This project can be seen as a platform consisting of three components:

 - Server (single): Acts as an intermediary between clients in the platform, facilitating message publishing and subscription.
 - TCP Clients: These clients connect to the server and can receive commands from the keyboard (interact with a human user)
   such as 'subscribe' and 'unsubscribe' and display messages received from the server.
 - UDP Clients: publish messages to the platform by sending them to the server using a predefined protocol.

The desired functionality is for each TCP client to receive messages from UDP clients related to the topics they are subscribed
to. The application should also include a feature for subscribing to multiple topics simultaneously using a wildcard operator.

# Server

The server will act as a broker in the message management platform. It will open two sockets (one TCP and one UDP) on a port
provided as a parameter and wait for connections/datagrams on all available local IP addresses.

Communication with UDP Clients:
 - The server will receive messages from UDP clients that follow the format defined below:

Topic	                        Data Type	                     Content
50 bytes	                    1 byte	                         Max 1500 bytes
Format: String of max           unsigned int on 1 byte           Variable depending
50 characters, terminated       used to specify the data         on the data type
with \0 for shorter sizes       type of the content

## Theory - listen() function in server
 - A socket is an endpoint for communication between two machines over a network.
   It is defined by an IP address and a port number.
 - A port is a numerical value that identifies a specific process or service on a machine. Ports allow multiple network services
   to run on a single machine and be distinguished from one another.

 - When a program wants to listen for incoming network connections or send data over the network, it creates a socket. The socket
   is then associated with a specific port number. This association allows the operating system to route incoming data to the
   correct program based on the port number.

# TCP clients

TCP clients can subscribe and unsubscribe from topics by sending messages to the server.

Accepted Commands:
TCP clients can receive the following commands from the keyboard:

 - subscribe <TOPIC> - Informs the server that a client wishes to subscribe to messages on the specified topic.
 - unsubscribe <TOPIC> - Informs the server that a client wishes to unsubscribe from the specified topic.
 - exit - Used to disconnect the client from the server and shut it down.

# Application Functionality

The application starts with the server, which then allows variable numbers of TCP and UDP clients to connect. The server must
allow new clients to connect/disconnect at any time.

Each TCP client will be identified by the ID with which it was started (CLIENT_ID), which can be a string of up to 10 ASCII
characters. At any time, there should not be two TCP clients connected with the same ID. If a TCP client tries to connect to
the server with an ID already in use, the server will display the message:  Client <CLIENT_ID> already connected. And the
client will close.

The server should keep track of the topics (with or without wildcards) to which each subscriber (client) is subscribed.
Upon receiving a valid UDP message, the server should ensure it is sent to all TCP clients subscribed to the respective topic.

# Testing

## server
./server 12345

## subscriber
./subscriber <ID> 127.0.0.1 12345

## client UDP -> generating messages
python3 udp_client.py 127.0.0.1 12345

-------------------------------------

## manual testing
sudo python3 topo.py

## running the set of tests
sudo python3 test.py

-------------------------------------

## command to terminate a process that is currently using the specified TCP port
sudo fuser -k 12345/tcp
