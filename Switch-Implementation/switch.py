#!/usr/bin/python3
import sys
import struct
import wrapper
import threading
import time
import os
from wrapper import recv_from_any_link, send_to_link, get_switch_mac, get_interface_name

# Adresa MAC destinatie prestabilita pentru BPDU
DST_MAC = bytes([0x01, 0x80, 0xC2, 0x00, 0x00, 0x00]) 
SRC_MAC = bytes([0x00, 0x00, 0x00, 0x00, 0x00, 0x00])

# Structura BPDU Config - parte a cadrului BPDU
def create_bpdu_config(root_bridge_id, root_path_cost, bridge_id):
    # Creez BPDU Config conform structurii specificate
    return struct.pack(
        '!III',
        root_bridge_id,
        root_path_cost,
        bridge_id,
    )

def create_bpdu_frame(root_bridge_id, sender_bridge_id, sender_path_cost):
    # Construieste cadrul BPDU complet
    bpdu_config = create_bpdu_config(root_bridge_id, sender_path_cost, sender_bridge_id)
    bpdu_frame = DST_MAC + SRC_MAC + bpdu_config
    return bpdu_frame

def send_bdpu_every_sec():
    while True:
        # Daca switch-ul este root_bridge, trimitem BPDU la fiecare secunda
        if own_bridge_id == root_bridge_id:
            data = create_bpdu_frame(
                root_bridge_id = own_bridge_id,
                sender_bridge_id = own_bridge_id,
                sender_path_cost = 0
            )

            # Trimit BPDU pe toate porturile trunk
            for interface in interfaces:
                if get_interface_name(interface) in trunk_ports:
                    send_to_link(interface, len(data), data)

        # Pauză de 1 secundă
        time.sleep(1)

def parse_bpdu(data):
    bpdu_root_bridge_id, bpdu_sender_bridge_id, bpdu_sender_path_cost = struct.unpack('!III', data[12:])
    return bpdu_root_bridge_id, bpdu_sender_bridge_id, bpdu_sender_path_cost

def parse_vlan_config(file):
    access_ports = {}
    trunk_ports = {}

    with open(file, 'r') as f:
        # sw_priority este prioritatea switch-ului si este
        # trecuta pe prima linie in fisierul de configurare
        sw_priority = int(next(f).strip())

        for line in f:
            # impart fiecare linie in 'cuvinte'
            parts = line.strip().split()
            # verific daca linia din fisier era goala
            if not parts:
                continue
                
            # porturi acces
            if parts[0].startswith("r-"):
                interface = parts[0]    # ex: 'r-0'
                vlan_id = int(parts[1]) # ex: 1
                access_ports[interface] = vlan_id
            
            # porturi trunk
            if parts[0].startswith("rr-"):
                interface = parts[0]    # ex: 'rr-0-1'
                trunk_ports[interface] = 'T'  # ex: 'T
    
    return access_ports, trunk_ports, sw_priority

def parse_ethernet_header(data):
    # Unpack the header fields from the byte array
    #dest_mac, src_mac, ethertype = struct.unpack('!6s6sH', data[:14])
    dest_mac = data[0:6]
    src_mac = data[6:12]
    
    # Extract ethertype. Under 802.1Q, this may be the bytes from the VLAN TAG
    ether_type = (data[12] << 8) + data[13]

    vlan_id = -1
    # Check for VLAN tag (0x8100 in network byte order is b'\x81\x00')
    if ether_type == 0x8200:
        vlan_tci = int.from_bytes(data[14:16], byteorder='big')
        vlan_id = vlan_tci & 0x0FFF  # extract the 12-bit VLAN ID
        ether_type = (data[16] << 8) + data[17]

    return dest_mac, src_mac, ether_type, vlan_id

def create_vlan_tag(vlan_id):
    # 0x8100 for the Ethertype for 802.1Q
    # vlan_id & 0x0FFF ensures that only the last 12 bits are used
    return struct.pack('!H', 0x8200) + struct.pack('!H', vlan_id & 0x0FFF)

def main():
    switch_id = sys.argv[1]         # numarul switch-ului
    global interfaces, trunk_ports, root_bridge_id, own_bridge_id

    # iau fisierul care trebuie configurat si il parsez
    vlan_config_file = os.path.join("configs", f"switch{switch_id}.cfg")
    access_ports, trunk_ports, sw_priority = parse_vlan_config(vlan_config_file)

    # init returns the max interface number. Our interfaces
    # are 0, 1, 2, ..., init_ret value + 1
    num_interfaces = wrapper.init(sys.argv[2:])
    interfaces = range(0, num_interfaces)

    print("# Starting switch with id {}".format(switch_id), flush=True)
    # print("[INFO] Switch MAC", ':'.join(f'{b:02x}' for b in get_switch_mac()))


    # INITIALIZARE STP
    ports_status = {}

    # Punem pe BLOCKING port-urile trunk pentru ca doar de acolo pot aparea bucle.
    # Port-urile catre statii sunt pe LISTENING
    for interface in interfaces:
        interface_name = get_interface_name(interface)
        if interface_name in trunk_ports:
            ports_status[interface_name] = "BLOCKING"
        else:
            ports_status[interface_name] = "LISTENING"
        
    # fiecare switch se considera Root Bridge
    own_bridge_id = int(sw_priority)
    root_bridge_id = own_bridge_id
    root_path_cost = 0

    # daca switch-ul devine root bridge setam toate porturile pe LISTENING
    if own_bridge_id == root_bridge_id:
        for interface in interfaces:
            interface_name = get_interface_name(interface)
            ports_status[interface_name] = "LISTENING"

    # Creare și pornire a unui nou thread pentru gestionarea trimiterii BDPU
    t = threading.Thread(target=send_bdpu_every_sec)
    t.start()

    # Afisarea interfetelor
    for i in interfaces:
        print(get_interface_name(i))

    # Initializare tabela de comutare (MAC table): Adresa MAC -> Port
    # Are intrari de forma Adresa MAC -> Port
    mac_table = {}

    while True:
        recv_interface, data, length = recv_from_any_link()
        # recv_interface = interfata/portul switch-ului pe care s a primit cadrul (0, 1 sau 2)
        # data = cadrul primit
        # length = lungimea cadrului primit

        dest_mac, src_mac, ethertype, vlan_id = parse_ethernet_header(data)
        # dest_mac = adresa MAC unde trb sa ajunga cadrul primit
        # src_mac = adresa MAC de unde a plecat cadrul primit
        # vlan_id = ID-ul VLAN-ului din care face parte cadrul; -1 dacă nu există un VLAN ID

        # actualizez tabela de comutare
        # ex: adresa mac -> port/interfata 0
        mac_table[src_mac] = recv_interface

        # am primit pe interfata recv_interface cadrul data
        # => trebuie sa vad daca e BPDU sau altecva
        destination_mac = data[:6]
        if destination_mac == DST_MAC:

            # am primit pachet BPDU
            # Extragem informațiile din BPDU primit
            bpdu_root_bridge_id, bpdu_sender_bridge_id, bpdu_sender_path_cost = parse_bpdu(data)

            if bpdu_root_bridge_id < root_bridge_id:
                i_was_root_bridge = own_bridge_id == root_bridge_id

                root_bridge_id = bpdu_root_bridge_id
                root_path_cost = bpdu_sender_path_cost + 10
                root_port = get_interface_name(recv_interface) # portul pe care a fost primit BPDU

                # daca eram root_bridge => 
                # seteaza toate porturi de tip trunk pe BLOCKING, cu exceptia portului radacina
                if i_was_root_bridge:
                    for interface in interfaces:
                        interface_name = get_interface_name(interface)
                        if interface_name in trunk_ports and interface_name != root_port:
                            ports_status[interface_name] = "BLOCKING"

                # daca starea lui root_port este BLOCKING => o setez pe LISTENING
                if ports_status[root_port] == "BLOCKING":
                    ports_status[root_port] = "LISTENING"
                        
                # fac update la cadrul BPDU si trimit pe toate porturile trunk
                bpdu_sender_bridge_id = own_bridge_id
                bpdu_sender_path_cost = root_path_cost
                        
                updated_bpdu = create_bpdu_frame(root_bridge_id, bpdu_sender_bridge_id, bpdu_sender_path_cost)

                # trimit BPDU-ul modificat pe toate trunk-urile
                for interface in interfaces:
                    if interface != recv_interface and get_interface_name(interface) in trunk_ports:
                        send_to_link(interface, len(updated_bpdu), updated_bpdu)
                    
            elif bpdu_root_bridge_id == root_bridge_id:
                if get_interface_name(recv_interface) == root_port and bpdu_sender_path_cost + 10 < root_path_cost:
                    root_path_cost = bpdu_sender_path_cost + 10
                elif get_interface_name(recv_interface) != root_port:
                    if bpdu_sender_path_cost > root_path_cost:
                        ports_status[get_interface_name(interface)] = "LISTENING"
                    
            elif bpdu_sender_bridge_id == own_bridge_id:
                ports_status[get_interface_name(recv_interface)] = "BLOCKING"
            else:
                continue
                        
            if own_bridge_id == root_bridge_id:
                for interface in interfaces:
                    interface_name = get_interface_name(interface)
                    ports_status[interface_name] = "LISTENING"
        else:
            # logica de VLAN

            # cadrul a fost primit pe un port acces
            if get_interface_name(recv_interface) in access_ports:
                # inseamna ca vlan_id ul frame ului este -1 => adaug header 802.1q
                # acest header va contine vlan_id ul din care face parte frame ul primit pe recv_interface
                vlan_tag = create_vlan_tag(access_ports[get_interface_name(recv_interface)])
                modified_frame = data[:12] + vlan_tag + data[12:]

                # extrag VLAN_ID
                vlan_tci = int.from_bytes(modified_frame[14:16], byteorder='big')
                vlan_id_from_tag = vlan_tci & 0x0FFF

                if dest_mac in mac_table:
                    if get_interface_name(mac_table[dest_mac]) in access_ports:
                        if ports_status[get_interface_name(recv_interface)] != "BLOCKING":
                            send_to_link(mac_table[dest_mac], length, data)
                    else:
                        if ports_status[get_interface_name(recv_interface)] != "BLOCKING":
                            send_to_link(mac_table[dest_mac], len(modified_frame), modified_frame)
                else:
                    # trimit pe toate porturile cu acelasi vlan si pe trunk
                    for interface in interfaces:
                        # formatez interfata curenta ca in access_ports/trunk_ports
                        curr_interface = get_interface_name(interface)

                        if interface != recv_interface:
                            # vad daca interfata curenta e de tip acces ca sa stiu daca verific VLAN_ID
                            if curr_interface in access_ports:
                                if access_ports[curr_interface] == vlan_id_from_tag:
                                    if ports_status[curr_interface] != "BLOCKING":
                                        send_to_link(interface, len(modified_frame), modified_frame)
                            elif curr_interface in trunk_ports:
                                if ports_status[curr_interface] != "BLOCKING":
                                    send_to_link(interface, len(modified_frame), modified_frame)
            
            # cadrul a fost primit pe un port trunk
            elif get_interface_name(recv_interface) in trunk_ports:

                # scot headerul 802.1q pentru cazul in care trimit de pe trunk pe access
                modified_frame = data[:12] + data[16:]

                if dest_mac in mac_table:
                    # trb sa verific, in functie de interfata, daca e acces sau trunk
                    # daca trimit mai departe cu tot cu headerul 802.1q sau fara
                    if get_interface_name(mac_table[dest_mac]) in access_ports:
                        # dau jos headerul 802.1q
                        if ports_status[get_interface_name(recv_interface)] != "BLOCKING":
                            send_to_link(mac_table[dest_mac], len(modified_frame), modified_frame)
                    elif get_interface_name(mac_table[dest_mac]) in trunk_ports:
                        # trimit cu tot cu headerul 802.1q
                        if ports_status[get_interface_name(recv_interface)] != "BLOCKING":
                            send_to_link(mac_table[dest_mac], length, data)
                else:
                    for interface in interfaces:
                        # formatez interfata curenta ca in access_ports/trunk_ports
                        curr_interface = get_interface_name(interface)

                        if interface != recv_interface:
                            if curr_interface in access_ports:
                                if vlan_id == access_ports[curr_interface]:
                                    if ports_status[curr_interface] != "BLOCKING":
                                        send_to_link(interface, len(modified_frame), modified_frame)
                            elif curr_interface in trunk_ports:
                                if ports_status[curr_interface] != "BLOCKING":
                                    send_to_link(interface, length, data)

if __name__ == "__main__":
    main()
