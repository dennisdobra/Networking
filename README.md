# Networking Projects

This repository contains four networking projects, each focusing on different aspects of networking functionality. Below is a brief description of each project:

## 1. Router-Dataplane

The **Router-Dataplane** project focuses on the implementation of a router's data plane, responsible for processing and forwarding packets. It includes:

- **Packet Forwarding**: Handles the forwarding of IP packets based on a routing table.
- **Routing Protocols**: Integrates basic routing protocols to maintain up-to-date routing information.
- **Error Handling**: Manages incorrect or invalid packets efficiently.

---

## 2. TCP-UDP-Messaging-Service

The **TCP-UDP-Messaging-Service** project implements a messaging service using both TCP and UDP protocols. Key features include:

- **TCP Support**: Reliable message delivery between clients using the Transmission Control Protocol.
- **UDP Support**: Fast message delivery with the User Datagram Protocol, optimized for speed over reliability.
- **Concurrent Connections**: Allows multiple clients to connect simultaneously and exchange messages.

---

## 3. Switch-Implementation

- The **Switch-Implementation** project focuses on implementing a network switch using Mininet, a network simulation tool that operates with real kernel, switch, and application code.
- The primary objectives include developing a MAC address table for frame forwarding, implementing Virtual Local Area Networks (VLANs), and utilizing the Spanning Tree Protocol (STP) to prevent network loops.

---

## 4. HTTP Client for Online Library System

The **HTTP Client for Online Library System** interacts with a digital library system. Key features include:

- **User Registration & Login**: Registers users and stores session cookies.
- **Library Access**: Grants access with a JWT token.
- **Book Management**: Allows viewing, adding, and deleting books.
- **Logout**: Clears session and JWT tokens.

Communication with the server is done via HTTP requests, with JSON responses parsed using the `nlohmann/json` library.

---

This `README.md` serves as a general overview of the projects housed in this repository. For detailed explanations and instructions, please explore the individual project directories.
