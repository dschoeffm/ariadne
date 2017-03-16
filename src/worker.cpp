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
#ifdef DEBUG
		logDebug("IPv4 verification failed");
#endif
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

	frame f[WORKER_BULK_SIZE];

	while(1){
		size_t numFrames = ingressQ->try_dequeue_bulk(f, WORKER_BULK_SIZE);
		if(!numFrames){
			return;
		}

		statsNumBatches++;
		statsNumFrames += numFrames;

		for(size_t i=0; i<numFrames; i++){
#ifdef DEBUG
			logDebug("Worker::process Processing packet now\n");
#endif

			statsNumBytes += f[i].len;

			// Cast all the things
			ether* ether_hdr = reinterpret_cast<ether*>(f[i].buf_ptr);
			ipv4* ipv4_hdr = reinterpret_cast<ipv4*>(f[i].buf_ptr + sizeof(ether));

			//Interface& interface = interfaces->at(f.iface & frame::IFACE_ID);
			shared_ptr<Interface> iface_ptr;
			for(auto iface : interfaces){
				if(iface->netmapIndex == (f[i].iface & frame::IFACE_ID)){
					iface_ptr = iface;
					break;
				}
			}
			if(iface_ptr == nullptr){
				fatal("Worker::process something went wrong: netmap interface not found in netlink context");
			}

			if(ether_hdr->ethertype == htons(0x0800)){
				if(!IPv4HdrVerification(ipv4_hdr, f[i].len)){
#ifdef DEBUG
					logDebug("Worker::process Discarding frame - Header verification failed");
#endif
					f[i].iface = frame::IFACE_DISCARD;
				} else {
					// Check if the packet is targeted at the router
					if(count(iface_ptr->IPs.begin(), iface_ptr->IPs.end(), ipv4_hdr->d_ip)){
#ifdef DEBUG
						logDebug("Worker::process Frame is destined at the host");
#endif
						f[i].iface |= frame::IFACE_HOST;
						continue;
					}

					// From here on, all checks were successful
					ipv4_hdr->ttl--;
					ipv4_hdr->checksum++;

					// Route the packet
					nh_index index = cur_lpm->route(ntohl(ipv4_hdr->d_ip));
#ifdef DEBUG
					logDebug("Worker::process nh_index: " + int2strHex(index));
#endif
					// Check if the index is invalid
					if(index == RoutingTable::route::NH_INVALID){
						fatal("Worker::process next hop is invalid");
					} else {
#ifdef DEBUG
						logDebug("Worker::process frame will use next hop " + int2strHex(index));
#endif
					}

					// Look up the next hop
					ARPTable::nextHop nh;
					if(!(index & RoutingTable::route::NH_DIRECTLY_CONNECTED)){
						nh = cur_arp_table->nextHops[index];
					} else {
						//nh = cur_arp_table->directlyConnected.at(ntohl(ipv4_hdr->d_ip));
						auto it = cur_arp_table->directlyConnected.find(ntohl(ipv4_hdr->d_ip));
						if(it != cur_arp_table->directlyConnected.end()){
							nh = it->second;
						} else {
							nh.mac = ARPTable::nextHop::invalidMac;
							nh.netmapInterface = index ^ RoutingTable::route::NH_DIRECTLY_CONNECTED;
						}
					}

					// Is the next hop valid?
					if(!nh){
						// Let the manager handle this
#ifdef DEBUG
						logDebug("Worker::process There is no MAC for this IP ("
								+ ip_to_str(htonl(ipv4_hdr->d_ip)) + ")");
						logDebug("Worker::process Interface for ARP: " + int2str(nh.netmapInterface));
#endif
						if(nh.netmapInterface == uint16_t_max){
							abort();
						}
						f[i].iface = nh.netmapInterface;
						f[i].iface |= frame::IFACE_NOMAC;
					} else {
						// Set MAC addresses
#ifdef DEBUG
						logDebug("Worker::process There is nothing special about this frame");
#endif
						ether_hdr->d_mac = nh.mac;
						ether_hdr->s_mac = iface_ptr->mac;
						f[i].iface = nh.netmapInterface;
					}
					egressQ->try_enqueue(f[i]);
				}
			} else if(ether_hdr->ethertype == htons(0x0806)){
#ifdef DEBUG
				logDebug("Worker::process This frame contains some kind of ARP payload");
#endif
				f[i].iface |= frame::IFACE_ARP;
				egressQ->try_enqueue(f[i]);
			} else {
				// This router currently doesn't support L3 Protocol $foo
#ifdef DEBUG
				std::stringstream stream;
				stream << std::hex << htons(ether_hdr->ethertype);
				logDebug("Worker::process L3 protocol is currently not supported, given: "
						+ stream.str());
#endif
				f[i].iface = frame::IFACE_DISCARD;
				egressQ->try_enqueue(f[i]);
			}
		}
	}
};

void Worker::printAndClearStats(){
	stringstream sstream;
	sstream << "Worker " << workerId << ": ";
	if((statsNumFrames > 0) && (statsNumBatches > 0)){
		sstream << "Avg. batch size: " << statsNumFrames / statsNumBatches << ", ";
	} else {
		sstream << "Avg. batch size: -, ";
	}
	sstream << "#Frames: " << statsNumFrames << ", ";
	sstream << "#Batches: " << statsNumBatches << ", ";
	sstream << "#Bytes: " << statsNumBytes;

	statsNumFrames = 0;
	statsNumBatches = 0;
	statsNumBytes = 0;

	logInfo(sstream.str());
};
