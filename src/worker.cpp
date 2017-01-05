#include "worker.hpp"

using namespace headers;

static bool IPv4HdrVerification(ipv4* ipv4_hdr, uint16_t f_len){
	// Do header verification (rfc 1812)
	// Step 1
	if(f_len < 20){
		return false;
	}

	// Step 2
	if(IPv4HdrChecksum(ipv4_hdr) != ipv4_hdr->checksum){
		return false;
	}

	// Step 3
	if(ipv4_hdr->version() != 4){
		return false;
	}

	// Step 4
	if(ipv4_hdr->ihl() < 5){
		return false;
	}

	// Step 5
	if(ntohs(ipv4_hdr->total_length) < ipv4_hdr->ihl()*4){
		return false;
	}

	// Check for TTL > 1
	if(ipv4_hdr->ttl <= 1){
		return false;
	}

	return true;
};

void Worker::process(){
	// TODO
	// ICMP STUFF

	// Get frames
	vector<frame> batch;
	ingressQ.pop(batch);

	// First version, not very efficient
	for(frame f : batch){
		// Cast all the things
		ether* ether_hdr = reinterpret_cast<ether*>(f.buf_ptr);
		ipv4* ipv4_hdr = reinterpret_cast<ipv4*>(f.buf_ptr + sizeof(ether));
		interface& interface = interfaces->at(f.iface);

		if(ether_hdr->ethertype == htons(0x0800)){
			if(!IPv4HdrVerification(ipv4_hdr, f.len)){
				f.iface = frame::IFACE_DISCARD;
			} else {
				// Check if the packet is targeted at the router
				if(count(interface.IPs.begin(), interface.IPs.end(), ipv4_hdr->d_ip)){
					f.iface |= frame::IFACE_HOST;
					continue;
				}

				// From here on, all checks were successful
				ipv4_hdr->ttl--;
				ipv4_hdr->checksum++;

				// Route the packet
				nh_index index = cur_lpm->route(ntohl(ipv4_hdr->d_ip));

				// Look up the next hop
				ARPTable::nextHop nh;
				if(index != RoutingTable::route::NH_DIRECTLY_CONNECTED){
					nh = cur_arp_table->nextHops[index];
				} else {
					nh = cur_arp_table->directlyConnected.at(ntohl(ipv4_hdr->d_ip));
				}

				// Set MAC addresses
				ether_hdr->d_mac = nh.mac;
				ether_hdr->s_mac = interface.mac;
				f.iface = nh.interface;
			}
		} else if(ether_hdr->ethertype == htons(0x0806)){
			f.iface |= frame::IFACE_ARP;
		} else {
			// This router currently doesn't support L3 Protocol $foo
			f.iface = frame::IFACE_DISCARD;
		}
	}

	egressQ.push(batch);
};
