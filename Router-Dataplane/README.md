# Router-Dataplane

In this project, I will implement the dataplane of a router, which comprises two essential components:

1. Dataplane - Responsible for executing the actual packet forwarding process based on the entries in the routing table.
2. Control Plane - Implements the routing algorithms (e.g., RIP, OSPF, BGP) that dynamically compute
                   the entries in the routing table through distributed algorithms.

Functionality Overview:

Upon receiving a packet on one of its interfaces via the recv_from_any_links() function, the router first determines
whether it has received an IPv4 packet or an ARP packet.

Handling IPv4 Packets:

 - If the received packet is an IPv4 packet, the router initially checks whether it is an ICMP echo request intended
   for the router itself. If so, it responds on the same interface with an ICMP echo reply.

 - For other IPv4 packets, the router undertakes the following steps:

1. Checksum Verification: It inspects the checksum to ensure the packet's integrity has not been compromised during transit.
2. Next-Hop Determination: It identifies the appropriate next hop and the interface through which the packet should be forwarded.
3. TTL Management: The Time to Live (TTL) field is decremented to prevent routing loops.
4. Checksum Recalculation: After modifying the TTL, the checksum must be recomputed to reflect the change.
5. Layer 2 Address Rewriting: The router attempts to rewrite the Layer 2 (MAC) addresses to forward the packet to the next hop.

 - If the router cannot locate an entry for the next hop in its ARP cache during step 5, the packet is queued, and an ARP request
   is dispatched to obtain the MAC address of the next hop. If a corresponding entry is found in the ARP cache, the Layer 2 addresses
   are updated, and the packet is forwarded to the next hop.

Handling ARP Packets:

 - Upon receiving an ARP packet, the router determines whether it is an ARP request or an ARP reply.
 - For an ARP request, the router responds by crafting an ARP reply, supplying the requested MAC address information.
 - If the received ARP packet is a reply (opcode 2), the router updates its ARP cache with the new information.
   Subsequently, it dequeues the first packet from the queue and, using the get_best_route() function, identifies
   the next hop for the packet.
 - The router then checks whether the IP address in the newly added ARP cache entry matches the next hop's IP address. If it does,
   the packet is forwarded via the best_route->interface. If not, the packet is requeued for further processing.

This process ensures efficient and accurate packet forwarding while maintaining the integrity and consistency of the routing process.