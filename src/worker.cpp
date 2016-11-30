#include "worker.hpp"

void Worker::process(){
	// Get frames
	vector<frame> batch;
	ingressQ.pop(batch);

	// First version, not very efficient
	for(frame f : batch){
		// Cast all the things
		ether* ether_hdr = reinterpret_cast<ether*>(f.buf_ptr);
		ipv4* ipv4_hdr = reinterpret_cast<ipv4*>(f.buf_ptr + sizeof(ether));
		arp* arp_hdr = reinterpret_cast<arp*>(f.buf_ptr + sizeof(ether));

		if(ether_hdr->ethertype == htons(0x0800)){
			goto ipv4;
		} else if(ether_hdr->ethertype == htons(0x0806)){
			goto arp;
		} else {
			goto discard;
		}

ipv4: {
			// Do header verification (rfc 1812)
			// Step 1
			if(f.len < 20){
				goto discard;
			}

			// Step 2
			uint16_t orig_chksum = ipv4_hdr->checksum;
			/* // TODO need checksum function
			ip->hdr_checksum = 0;
			if(rte_ipv4_cksum(ip) != orig_chksum){
				goto discard;
			}
			ip->hdr_checksum = orig_chksum;
			*/

			// Step 3
			if(IPv4_VERSION(ipv4_hdr) != 4){
				goto discard;
			}

			// Step 4
			if(IPv4_IHL(ipv4_hdr) < 5){
				goto discard;
			}

			// Step 5
			if(ntohs(ipv4_hdr->total_length) < (IPv4_IHL(ipv4_hdr)*4)){
				goto discard;
			}

			// Check for TTL > 1
			if(ipv4_hdr->ttl <= 1){
				goto discard;
			}

			// From here on, all checks were successful
			ipv4_hdr->ttl--;
			ipv4_hdr->checksum++;

			// Route the packet
			nh_index index = cur_lpm->route(ntohl(ipv4_hdr->destination_ip));

			// Look up the next hop
			array<uint8_t, 6> nh_mac;
			if(index != NH_DIRECTLY_CONNECTED){
				nh_mac = cur_arp_table->nextHops[index].mac;
			} else {
				nh_mac = cur_arp_table->directlyConnected[ntohl(ipv4_hdr->destination_ip)];
				// TODO which interface ?
				cur_arp_table->directlyConnected
			}

			continue;
		};
arp: {};

		continue;

discard: {};
		//Just do nothing, this L3 protocol is not implemented
		continue;

	}
};
