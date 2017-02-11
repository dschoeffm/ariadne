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
		logDebug("IPv4 verification failed");
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

	frame f;
	while(1){
		if(!ingressQ->try_dequeue(f)){
			return;
		}

		logDebug("Worker::process Processing packet now");

		// Cast all the things
		ether* ether_hdr = reinterpret_cast<ether*>(f.buf_ptr);
		ipv4* ipv4_hdr = reinterpret_cast<ipv4*>(f.buf_ptr + sizeof(ether));
		Interface& interface = interfaces->at(f.iface & frame::IFACE_ID);

		if(ether_hdr->ethertype == htons(0x0800)){
			if(!IPv4HdrVerification(ipv4_hdr, f.len)){
				logDebug("Discarding frame - Header verification failed");
				f.iface = frame::IFACE_DISCARD;
			} else {
				// Check if the packet is targeted at the router
				if(count(interface.IPs.begin(), interface.IPs.end(), ipv4_hdr->d_ip)){
					logDebug("Frame is destined at the host");
					f.iface |= frame::IFACE_HOST;
					continue;
				}

				// From here on, all checks were successful
				ipv4_hdr->ttl--;
				ipv4_hdr->checksum++;

				// Route the packet
				nh_index index = cur_lpm->route(ntohl(ipv4_hdr->d_ip));

				// Check if the index is invalid
				if(index == RoutingTable::route::NH_INVALID){
					fatal("next hop is invalid");
				}

				// Look up the next hop
				ARPTable::nextHop nh;
				if(index != RoutingTable::route::NH_DIRECTLY_CONNECTED){
					nh = cur_arp_table->nextHops[index];
				} else {
					//nh = cur_arp_table->directlyConnected.at(ntohl(ipv4_hdr->d_ip));
					auto it = cur_arp_table->directlyConnected.find(ntohl(ipv4_hdr->d_ip));
					if(it != cur_arp_table->directlyConnected.end()){
						nh = it->second;
					} else {
						nh.mac = ARPTable::nextHop::invalidMac;
					}
				}

				// Is the next hop valid?
				if(!nh){
					// Let the manager handle this
					logDebug("There is no MAC for this IP (" + ip_to_str(htonl(ipv4_hdr->d_ip)) + ")");
					f.iface = nh.netmapInterface;
					f.iface &= frame::IFACE_ID;
					f.iface = frame::IFACE_NOMAC;
				} else {
					// Set MAC addresses
					logDebug("There is nothing special about this frame");
					ether_hdr->d_mac = nh.mac;
					ether_hdr->s_mac = interface.mac;
					f.iface = nh.netmapInterface;
				}
				egressQ->try_enqueue(f);
			}
		} else if(ether_hdr->ethertype == htons(0x0806)){
			logDebug("This frame contains some kind of ARP payload");
			f.iface |= frame::IFACE_ARP;
			egressQ->try_enqueue(f);
		} else {
			// This router currently doesn't support L3 Protocol $foo
			std::stringstream stream;
			stream << std::hex << htons(ether_hdr->ethertype);
			logDebug("L3 protocol is currently not supported, given: " + stream.str());
			f.iface = frame::IFACE_DISCARD;
			egressQ->try_enqueue(f);
		}
	}
};
