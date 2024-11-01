# SWITCH IMPLEMENTATION

## MAC TABLE

- When an Ethernet frame is received, the switch associates the MAC address with the corresponding port.
- It creates a MAC table entry that links the port to the source MAC address found in the Ethernet header.
- If there is no entry for the destination MAC address, the switch forwards the frame to all other ports.

## VLAN

- In VLAN configurations, broadcasts are sent only to ports that share the same VLAN tag or to trunk ports.
- When a frame is received:
  - **On a Trunk Port:** The frame will contain an 802.1Q header.
  - **On an Access Port:** The switch must add an 802.1Q header if necessary.
  - **Matching VLAN ID:** If the VLAN ID of the frame matches that of the access port, the frame is forwarded without the 802.1Q header.
- Frames will not be forwarded if the destination port's VLAN ID does not match the VLAN ID of the incoming frame.

## THE SPANNING TREE PROTOCOL - Simplified Version

- A single STP process manages all VLANs and blocks links that could create loops.
- Frames are referred to as Bridge Protocol Data Units (BPDU) and contain the following information:
  1. **Root Bridge ID**: Identifies the root bridge.
  2. **Sender Bridge ID**: Identifies the switch sending the BPDU.
  3. **Root Path Cost**: The cost to reach the root bridge.
- The root bridge is determined as the switch with the lowest ID.
- Switch ports can operate in either **Blocking** or **Listening** states. Access ports remain in the Listening state.
- Upon startup, each switch considers itself the root bridge and sets all ports to Listening.

## Spanning Tree Algorithm

1. **Initialization**
   - Set all trunk ports to **BLOCKING** and all access ports to **LISTENING**.
   - Each switch acts as the root bridge.

2. **BPDU Transmission**
   - If a switch is the root bridge, it sends a BPDU packet every second.

## BPDU Reception and Processing

### Actions Upon Receiving a BPDU

When a switch receives a BPDU, it performs the following actions based on the contents of the BPDU:

1. **If `root_bridge_ID` is Lower:**
   - Update the switch's `root_bridge_ID` to the received value.
   - Increase the `root_path_cost` by +10.
   - Set the `root_port` to the port from which the BPDU was received.
   - If the switch was previously the root bridge, block any non-root ports.
   - Change the `root_port` to **LISTENING** if it was in the **BLOCKING** state.
   - Forward the updated BPDU to all trunk ports.

2. **If `root_bridge_ID` is the Same:**
   - If the BPDU is received on the `root_port`, check if the `root_path_cost` is lower than the current value.
   - Update the `root_path_cost` if the received cost is lower.
   - Determine the **Designated Port** based on the path costs of the ports.

3. **If `sender_bridge_ID` is Equal:**
   - Set the port state to **BLOCKING** to prevent potential loops.

4. **Otherwise:**
   - Discard the BPDU, as it does not contain relevant or valid information.

### Root Bridge Condition

- If the switch identifies itself as the root bridge, it will set all its ports as **DESIGNATED_PORT** to handle incoming traffic efficiently.


## Running

```bash
sudo python3 checker/topo.py
```

This will open 9 terminals, 6 hosts and 3 for the switches. On the switch terminal you will run 

```bash
make run_switch SWITCH_ID=X # X is 0,1 or 2
```

The hosts have the following IP addresses.
```
host0 192.168.1.1
host1 192.168.1.2
host2 192.168.1.3
host3 192.168.1.4
host4 192.168.1.5
host5 192.168.1.6
```

We will be testing using the ICMP. For example, from host0 we will run:

```
ping 192.168.1.2
```

Note: We will use wireshark for debugging. From any terminal you can run `wireshark&`.
