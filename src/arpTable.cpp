#include "arpTable.hpp"

using namespace std;

void ARPTable::createCurrentTable(std::shared_ptr<RoutingTable> routingTable){
	// create a new table;
	shared_ptr<table> newTable = make_shared<table>(std::vector<nextHop>(), directlyConnected);
	auto next_hop_addresses = routingTable->getNextHopMapping();

	// Fill in the new table
	newTable->nextHops.resize(next_hop_addresses->size());
	for(unsigned int i=0; i<next_hop_addresses->size(); i++){
		auto it = mapping.find((*next_hop_addresses)[i]);
		if(it == mapping.end()){
			continue;
		}

		newTable->nextHops[i] = it->second;;
	}

	// Set the new table as the default
	currentTable = newTable;
}

void ARPTable::prepareRequest(uint32_t ip, uint16_t interface, frame& frame){
	ether* ether_hdr = reinterpret_cast<ether*>(frame.buf_ptr);
	arp* arp_hdr = reinterpret_cast<arp*>(frame.buf_ptr + sizeof(ether));

	ether_hdr->s_mac = interface_macs[interface];
	ether_hdr->d_mac = {{0xff}};

	arp_hdr->hw_type = htons(0x0001);
	arp_hdr->proto_type = htons(0x0800);
	arp_hdr->hw_len = 6;
	arp_hdr->proto_len = 4;
	arp_hdr->op = ARP_OP_REQUEST;

	arp_hdr->s_hw_addr = interface_macs[interface];
	arp_hdr->s_proto_addr = own_IPs[interface];
	arp_hdr->t_hw_addr = {{0}};
	arp_hdr->t_proto_addr = htonl(ip);

	frame.len = sizeof(ether) + sizeof(arp);
}

void ARPTable::handleReply(frame& frame){
	// ARP is stateless -> doesn't mapper if we actually sent a request
	uint32_t ip;
	std::array<uint8_t, 6> mac;
	ether* ether_hdr = reinterpret_cast<ether*>(frame.buf_ptr);
	arp* arp_hdr = reinterpret_cast<arp*>(frame.buf_ptr + sizeof(ether));

	if(ether_hdr->ethertype != htons(0x0806)){
		// This is not an ARP frame
		return;
	}

	if(arp_hdr->op != ARP_OP_REPLY){
		// This is not a reply
		return;
	}

	ip = arp_hdr->s_proto_addr;
	mac = arp_hdr->s_hw_addr;

	nextHop nextHop;

	nextHop.mac = mac;
	nextHop.interface = frame.iface;

	auto next_hop_addresses = routingTable->getNextHopMapping();
	auto it = find(next_hop_addresses->begin(), next_hop_addresses->end(), ip);
	if(it == next_hop_addresses->end()){
		// This is a next hop inside the routing table
		mapping.insert({ip, nextHop});
		currentTable->nextHops[distance(next_hop_addresses->begin(), it)] = nextHop;
	} else {
		// This is a directly connected node
		directlyConnected.insert({ip, nextHop});
	}
}

void ARPTable::handleRequest(frame& frame){
	ether* ether_hdr = reinterpret_cast<ether*>(frame.buf_ptr);
	arp* arp_hdr = reinterpret_cast<arp*>(frame.buf_ptr + sizeof(ether));

	if(ether_hdr->ethertype != htons(0x0806)){
		return;
	}

	if(arp_hdr->op != ARP_OP_REQUEST){
		return;
	}

	// Check if we are asked
	if(arp_hdr->t_proto_addr != own_IPs[frame.iface]){
		return;
	}

	// Turn the request into a reply
	arp_hdr->op = ARP_OP_REPLY;

	arp_hdr->t_hw_addr = arp_hdr->s_hw_addr;
	arp_hdr->t_proto_addr = arp_hdr->s_proto_addr;

	arp_hdr->s_hw_addr = interface_macs[frame.iface];
	arp_hdr->s_proto_addr = own_IPs[frame.iface];

	ether_hdr->s_mac = interface_macs[frame.iface];
	ether_hdr->d_mac = {{0xff}};
}
