#include "arpTable.hpp"

using namespace std;
using namespace headers;

constexpr array<uint8_t, 6> ARPTable::nextHop::invalidMac;

#if 0
void ARPTable::createCurrentTable(std::shared_ptr<RoutingTable> routingTable){
	logDebug("ARPTable::createCurrentTable constructing tables for mapping");

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

		newTable->nextHops[i] = it->second;
	}

	// Set the new table as the default
	currentTable = newTable;
}
#endif

void ARPTable::createCurrentTable(std::shared_ptr<RoutingTable> routingTable){
	logDebug("ARPTable::createCurrentTable constructing tables for mapping");

	// create a new table;
	shared_ptr<table> newTable = make_shared<table>(std::vector<nextHop>(), directlyConnected);
	auto next_hop_addresses = routingTable->getNextHopMapping();

	newTable->nextHops.resize(next_hop_addresses->size());
	for(auto nh : *next_hop_addresses){
		if(nh.interface->netmapIndex == uint16_t_max){
			logDebug("invalid nh interface");
			abort();
		}
		newTable->nextHops[nh.index].netmapInterface = nh.interface->netmapIndex;
		newTable->nextHops[nh.index].mac = {{0}}; // just initialize
		if(mapping.count(nh.nh_ip)){
			newTable->nextHops[nh.index].mac = mapping[nh.nh_ip].mac;
		}
	}

	currentTable = newTable;
};

void ARPTable::prepareRequest(uint32_t ip, uint16_t iface, frame& frame){
	logDebug("ARPTable::prepareRequest Preparing ARP request now");

	ether* ether_hdr = reinterpret_cast<ether*>(frame.buf_ptr);
	arp* arp_hdr = reinterpret_cast<arp*>(frame.buf_ptr + sizeof(ether));

	ether_hdr->s_mac = interfaces->at(iface).mac;
	ether_hdr->d_mac = {{0xff}};

	arp_hdr->hw_type = htons(0x0001);
	arp_hdr->proto_type = htons(0x0800);
	arp_hdr->hw_len = 6;
	arp_hdr->proto_len = 4;
	arp_hdr->op = arp::OP_REQUEST;

	arp_hdr->s_hw_addr = interfaces->at(iface).mac;
	if(interfaces->at(iface).IPs.empty()){
		fatal("Cannot send ARP request without an IP address on interface");
	}
	arp_hdr->s_proto_addr = interfaces->at(iface).IPs.front();
	arp_hdr->t_hw_addr = {{0}};
	arp_hdr->t_proto_addr = htonl(ip);

	frame.len = sizeof(ether) + sizeof(arp);
}

void ARPTable::handleReply(frame& frame){
	logDebug("ARPTable::handleReply Looking at ARP reply now");

	// ARP is stateless -> doesn't mapper if we actually sent a request
	uint32_t ip;
	std::array<uint8_t, 6> mac;
	ether* ether_hdr = reinterpret_cast<ether*>(frame.buf_ptr);
	arp* arp_hdr = reinterpret_cast<arp*>(frame.buf_ptr + sizeof(ether));

	if(ether_hdr->ethertype != htons(0x0806)){
		logErr("Frame falsely sent to ARPTable::handleReply()");
		return;
	}

	if(ntohs(arp_hdr->op) != arp::OP_REPLY){
		// This is not a reply
		logErr("Frame falsely sent to ARPTable::handleReply()");
		return;
	}

	ip = ntohl(arp_hdr->s_proto_addr);
	mac = arp_hdr->s_hw_addr;

	nextHop nextHop;

	nextHop.mac = mac;
	nextHop.netmapInterface = frame.iface & frame::IFACE_ID;

	auto next_hop_addresses = routingTable->getNextHopMapping();
#if 0
	auto it = find(next_hop_addresses->begin(), next_hop_addresses->end(), ip);
	if(it == next_hop_addresses->end()){
		// This is a next hop inside the routing table
		mapping.insert({ip, nextHop});
		currentTable->nextHops[distance(next_hop_addresses->begin(), it)] = nextHop;
	} else {
		// This is a directly connected node
		directlyConnected.insert({ip, nextHop});
	}
#endif

	// Check if this is a registered next hop
	for(auto& nh : *next_hop_addresses){
		if(nh.nh_ip == ip && nh.interface->netmapIndex == (frame.iface & frame::IFACE_ID)){
			mapping.insert({ip, nextHop});
			currentTable->nextHops[nh.index].mac = mac;

			return;
		}
	}

	// In case we reach this, the node is directly connected
	directlyConnected.insert({ip, nextHop});
}

void ARPTable::handleRequest(frame& frame){
	logDebug("ARPTable::handleRequest looking ar request now");
	ether* ether_hdr = reinterpret_cast<ether*>(frame.buf_ptr);
	arp* arp_hdr = reinterpret_cast<arp*>(frame.buf_ptr + sizeof(ether));

	if(ether_hdr->ethertype != htons(0x0806)){
		logErr("Frame falsely sent to ARPTable::handleRequest()");
		return;
	}

	if(ntohs(arp_hdr->op) != arp::OP_REQUEST){
		logErr("Frame falsely sent to ARPTable::handleRequest()");
		return;
	}

	Interface& interface = interfaces->at(frame.iface ^ frame::IFACE_ARP);
	// Check if we are asked
	if(!count(interface.IPs.begin(), interface.IPs.end(),
			arp_hdr->t_proto_addr)){
		return;
	}

	// Turn the request into a reply
	arp_hdr->op = htons(arp::OP_REPLY);

	arp_hdr->t_hw_addr = arp_hdr->s_hw_addr;
	uint32_t t_ip = arp_hdr->t_proto_addr;
	arp_hdr->t_proto_addr = arp_hdr->s_proto_addr;

	arp_hdr->s_hw_addr = interface.mac;
	arp_hdr->s_proto_addr = t_ip;

	ether_hdr->s_mac = interface.mac;
	ether_hdr->d_mac = {{0xff}};
}

void ARPTable::handleFrame(frame& frame){
	ether* ether_hdr = reinterpret_cast<ether*>(frame.buf_ptr);
	arp* arp_hdr = reinterpret_cast<arp*>(frame.buf_ptr + sizeof(ether));

	if(ether_hdr->ethertype != htons(0x0806)){
		logErr("Frame falsely sent to ARPTable::handleFrame()");
		return;
	}

	if(ntohs(arp_hdr->op) == arp::OP_REQUEST){
		handleRequest(frame);
	} else if(ntohs(arp_hdr->op) == arp::OP_REPLY){
		handleReply(frame);
	} else {
		logErr("Unknown ARP OP detected");
	}
}
