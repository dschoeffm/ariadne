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
	vector<frame> batchIn;
	vector<frame> batchOut;
	ingressQ.pop(batchIn);
	auto now = std::chrono::steady_clock::now();

	// First version, not very efficient
	for(frame f : batchIn){
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

				// Is the next hop valid?
				if(!nh){
					std::chrono::seconds oneSecond(1);
					backlog.emplace_back(backlogFrame {f, index, now + oneSecond});
					continue;
				}

				// Set MAC addresses
				ether_hdr->d_mac = nh.mac;
				ether_hdr->s_mac = interface.mac;
				f.iface = nh.interface;
				batchOut.push_back(f);
			}
		} else if(ether_hdr->ethertype == htons(0x0806)){
			f.iface |= frame::IFACE_ARP;
		} else {
			// This router currently doesn't support L3 Protocol $foo
			f.iface = frame::IFACE_DISCARD;
		}
	}

	vector<backlogFrame> newBacklog;
	for(auto blf : backlog){
		if(blf.timeout <= now){
			blf.frame.iface = frame::IFACE_DISCARD;
			continue;
		}

		ether* ether_hdr = reinterpret_cast<ether*>(blf.frame.buf_ptr);
		ipv4* ipv4_hdr = reinterpret_cast<ipv4*>(blf.frame.buf_ptr + sizeof(ether));
		interface& interface = interfaces->at(blf.frame.iface);

		ARPTable::nextHop nh;
		if(blf.nh != RoutingTable::route::NH_DIRECTLY_CONNECTED){
			nh = cur_arp_table->nextHops[blf.nh];
		} else {
			nh = cur_arp_table->directlyConnected.at(ntohl(ipv4_hdr->d_ip));
		}

		// Is the next hop valid?
		if(!nh){
			newBacklog.push_back(blf);
			continue;
		}

		// Set MAC addresses
		ether_hdr->d_mac = nh.mac;
		ether_hdr->s_mac = interface.mac;
		blf.frame.iface = nh.interface;
		batchOut.push_back(blf.frame);
	}

	swap(backlog, newBacklog);

	egressQ.push(batchOut);
};
