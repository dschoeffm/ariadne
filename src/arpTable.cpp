#include "arpTable.hpp"

using namespace std;
using namespace headers;

constexpr array<uint8_t, 6> ARPTable::nextHop::invalidMac;

void ARPTable::createCurrentTable(std::shared_ptr<RoutingTable> routingTable){
#ifdef DEBUG
	logDebug("ARPTable::createCurrentTable constructing tables for mapping");
#endif
	this->routingTable = routingTable;

	// create a new table;
	shared_ptr<table> newTable = make_shared<table>(std::vector<nextHop>(), directlyConnected);
	auto next_hop_addresses = routingTable->getNextHopMapping();

	newTable->nextHops.resize(next_hop_addresses->size());
	for(auto nh : *next_hop_addresses){
		if(nh.interface->netmapIndex == uint16_t_max){
#ifdef DEBUG
			logDebug("invalid nh interface");
#endif
			abort();
		}
		newTable->nextHops[nh.index].netmapInterface = nh.interface->netmapIndex;
		//newTable->nextHops[nh.index].mac = {{0}}; // just initialize
		for(int i=0; i<6; i++){
			newTable->nextHops[nh.index].mac[i] = 0;
		}
		if(mapping.count(nh.nh_ip)){
			newTable->nextHops[nh.index].mac = mapping[nh.nh_ip].mac;
		}
	}

	currentTable = newTable;
};

void ARPTable::prepareRequest(uint32_t ip, uint16_t iface, frame& frame){
//#ifdef DEBUG
	logInfo("ARPTable::prepareRequest Preparing ARP request now, IP: " + ip_to_str(ntohl(ip)));
//#endif

	ether* ether_hdr = reinterpret_cast<ether*>(frame.buf_ptr);
	arp* arp_hdr = reinterpret_cast<arp*>(frame.buf_ptr + sizeof(ether));

	shared_ptr<Interface> iface_ptr;
	for(auto i : interfaces){
		if(i->netmapIndex == iface){
			iface_ptr = i;
			break;
		}
	}

	if(iface_ptr == nullptr){
		abort();
	}

	ether_hdr->s_mac = iface_ptr->mac;
	for(int i=0; i<6; i++){
		ether_hdr->d_mac[i] = 0xff;
	}
	ether_hdr->ethertype = htons(0x0806);

	arp_hdr->hw_type = htons(0x0001);
	arp_hdr->proto_type = htons(0x0800);
	arp_hdr->hw_len = 6;
	arp_hdr->proto_len = 4;
	arp_hdr->op = htons(arp::OP_REQUEST);

	arp_hdr->s_hw_addr = iface_ptr->mac;
	if(iface_ptr->IPs.empty()){
		fatal("Cannot send ARP request without an IP address on interface");
	}
	arp_hdr->s_proto_addr = htonl(iface_ptr->IPs.front());
	for(int i=0; i<6; i++){
		arp_hdr->t_hw_addr[i] = 0;
	}
	arp_hdr->t_proto_addr = ip;

	frame.len = sizeof(ether) + sizeof(arp);
}

void ARPTable::handleReply(frame& frame){
//#ifdef DEBUG
	logInfo("ARPTable::handleReply Looking at ARP reply now");
//#endif

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
#ifdef DEBUG
	logDebug("ARPTable::handleRequest looking at request now");
#endif
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

	shared_ptr<Interface> iface_ptr;
	for(auto i : interfaces){
		if(i->netmapIndex == (frame.iface & frame::IFACE_ID)){
			iface_ptr = i;
			break;
		}
	}

	if(iface_ptr == nullptr){
		abort();
	}

	// Check if we are asked
	if(!count(iface_ptr->IPs.begin(), iface_ptr->IPs.end(),
			ntohl(arp_hdr->t_proto_addr))){
#ifdef DEBUG
		logDebug("Got ARP request for some other node (IP: "
				 + ip_to_str(ntohl(arp_hdr->t_proto_addr)) + "), discarding");
#endif
		frame.iface = frame::IFACE_DISCARD;
		return;
	}

	// Save the info in the request
	uint32_t s_ip = ntohl(arp_hdr->s_proto_addr);
	array<uint8_t, 6> s_mac = arp_hdr->s_hw_addr;

	nextHop nextHop;

	nextHop.mac = s_mac;
	nextHop.netmapInterface = frame.iface & frame::IFACE_ID;

	auto next_hop_addresses = routingTable->getNextHopMapping();

	// Check if this is a registered next hop
	for(auto& nh : *next_hop_addresses){
		if(nh.nh_ip == s_ip && nh.interface->netmapIndex == (frame.iface & frame::IFACE_ID)){
			mapping.insert({s_ip, nextHop});
			currentTable->nextHops[nh.index].mac = s_mac;

			return;
		}
	}

	// In case we reach this, the node is directly connected
	directlyConnected.insert({s_ip, nextHop});


	// Turn the request into a reply
	arp_hdr->op = htons(arp::OP_REPLY);

	arp_hdr->t_hw_addr = arp_hdr->s_hw_addr;
	uint32_t t_ip = arp_hdr->t_proto_addr;
	arp_hdr->t_proto_addr = arp_hdr->s_proto_addr;

	arp_hdr->s_hw_addr = iface_ptr->mac;
	arp_hdr->s_proto_addr = t_ip;

	ether_hdr->s_mac = iface_ptr->mac;
	for(int i=0; i<6; i++){
		ether_hdr->d_mac[i] = 0xff;
	}

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
