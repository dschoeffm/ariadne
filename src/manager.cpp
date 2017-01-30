#include "manager.hpp"

using namespace std;
using namespace moodycamel;
using namespace headers;
using namespace chrono;

void Manager::initNetmap(){

	if(geteuid() != 0){
		fatal("You need to be root in order to use Netmap!");
	}

	nmreq_root.nr_version = NETMAP_API;
	nmreq_root.nr_tx_slots = 2048;
	nmreq_root.nr_rx_slots = 2048;
	nmreq_root.nr_tx_rings = numWorkers;
	nmreq_root.nr_rx_rings = numWorkers;
	nmreq_root.nr_ringid = 0;
	nmreq_root.nr_flags = NR_REG_NIC_SW;
	nmreq_root.nr_cmd = 0;
	nmreq_root.nr_arg1 = 0;
	nmreq_root.nr_arg2 = 1;
	nmreq_root.nr_arg3 = 2048;

	int iface_num=0;

	for(auto iface : interfacesToUse){
		logInfo("Preparing interface " + iface);
		int fd;
		fds.push_back(fd = open("/dev/netmap", O_RDWR));
		strncpy(nmreq_root.nr_name, iface.c_str(), 16);
		ioctl(fd, NIOCREGIF, &nmreq_root);
		if(!mmapRegion){
			mmapRegion = mmap(0, nmreq_root.nr_memsize,
					PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
			if(mmapRegion == MAP_FAILED){
				logErr("Manager::initNetmap() mmap() failed");
				logErr("error: " + string(strerror(errno)));
				abort();
			}
		}
		if(nmreq_root.nr_tx_rings != numWorkers){
			stringstream sstream;
			sstream << "Could not set the correct amount of TX rings!" << endl;
			sstream << "Present: " << nmreq_root.nr_tx_rings << endl;
			sstream << "Expected: " << numWorkers << endl;
			sstream << "Please use the following command" << endl;
			sstream << "ethtool -L " << iface  <<" combined " << numWorkers << endl;
			fatal(sstream.str());
		}
		if(nmreq_root.nr_rx_rings != numWorkers){
			stringstream sstream;
			sstream << "Could not set the correct amount of RX rings!" << endl;
			sstream << "Present: " << nmreq_root.nr_rx_rings << endl;
			sstream << "Expected: " << numWorkers << endl;
			sstream << "Please use the following command" << endl;
			sstream << "ethtool -L " << iface  <<" combined " << numWorkers << endl;
			fatal(sstream.str());
		}

		netmap_if* nifp = NETMAP_IF(mmapRegion, nmreq_root.nr_offset);
		netmapIfs.push_back(nifp);

		netmapTxRings.resize(netmapTxRings.size()+1);
		netmapRxRings.resize(netmapRxRings.size()+1);

		for(uint32_t i=0; i<nifp->ni_tx_rings+1; i++){
			netmapTxRings[iface_num].push_back(NETMAP_TXRING(nifp, i));
		}
		for(uint32_t i=0; i<nifp->ni_rx_rings+1; i++){
			netmapRxRings[iface_num].push_back(NETMAP_RXRING(nifp, i));
		}

		for(uint32_t bufIdx = nifp->ni_bufs_head; bufIdx;
				bufIdx = * reinterpret_cast<uint32_t*>(NETMAP_BUF(netmapTxRings[0][0], bufIdx))){
			freeBufs.push_back(bufIdx);
		}
		auto it = find_if(interfaces->begin(), interfaces->end(),
			[iface](interface& i){
				return i == iface;
			});

		if(it == interfaces->end()){
			fatal("something went wrong: netmap interface not found in netlink context");
		}

		it->netmapIndex = iface_num;

		iface_num++;
	}
	logInfo("Interface preparations finished.\n");

	for(unsigned int i=0; i<numWorkers; i++){
		inRings.emplace_back(make_shared<ConcurrentQueue<frame>>(RING_SIZE));
		outRings.emplace_back(make_shared<ConcurrentQueue<frame>>(RING_SIZE));
	}

	routingTable = make_shared<LinuxTable>();
	routingTable->print_table();
	curLPM = make_shared<LPM>(*(routingTable.get()));
	numInterfaces = netmapIfs.size();
};

std::shared_ptr<std::vector<interface>> Manager::fillNetLink(){
	shared_ptr<vector<interface>> interfaces = Netlink::getAllInterfaces();
	sort(interfaces->begin(), interfaces->end());
	/*
	uint32_t max_index = interfaces->back().netlinkIndex;
	if(max_index != interfaces->size()){
		for(uint32_t index=0; index<max_index; index++){
			auto it = find_if(interfaces->begin(), interfaces->end(),
					[index](interface& i){
						return i == index;
					});

			if (it == interfaces->end()){
				interfaces->emplace(interfaces->end(), index);
			}
		}
	}
	sort(interfaces->begin(), interfaces->end());
	*/
	return interfaces;
};

void Manager::startWorkerThreads(){
	for(unsigned i=0; i<numWorkers; i++){
		workers.push_back(new Worker(curLPM, arpTable.getCurrentTable(),
			inRings[i], outRings[i], interfaces, *this, arpTable));
	}
};

void Manager::process(){
	// Prepare poll fd set
	pollfd* pfds = new pollfd[numInterfaces];
	for(unsigned int i=0; i<numInterfaces; i++){
		pfds[i].fd = fds[i];
		pfds[i].events = POLLOUT | POLLIN;
	}
	// use poll to get new frames
	if(0 > poll(pfds, numInterfaces, -1)){
		fatal("poll() failed");
	}

	// Run over all interfaces and rings -> enqueue new frames for workers
	for(uint16_t iface=0; iface < numInterfaces; iface++){
		// numWorkers+1 for host ring
		for(unsigned int worker=0; worker < numWorkers+1; worker++){
			netmap_ring* ring = netmapRxRings[iface][worker];
			uint32_t numFrames = nm_ring_space(ring);
			numFrames = min(numFrames, (uint32_t) freeBufs.size());
			uint32_t slotIdx = ring->head;

			for(uint32_t frameIdx=0; frameIdx < numFrames; frameIdx++){
				frame f;
				f.buf_ptr = reinterpret_cast<uint8_t*>(NETMAP_BUF(ring, slotIdx));
				f.len = ring->slot[slotIdx].len;
				f.iface = iface;
				f.vlan = 0;
				inRings[iface]->try_enqueue(f);

				ring->slot[slotIdx].buf_idx = freeBufs.back();
				freeBufs.pop_back();
				slotIdx = nm_ring_next(ring, slotIdx);
				ring->head = slotIdx;
				ring->cur = slotIdx;
			}
		}
	}

	// Run over all current MAC requests
	/*
	vector<ARPTable::request> requests;
	arpTable.getRequests(requests);
	for(auto req : requests){
		netmap_ring* ring = netmapRxRings[req.interface][0];
		if(nm_ring_space(ring)){
			frame f(NETMAP_BUF(ring, ring->head), 0, req.interface);
			arpTable.prepareRequest(req, f);
			ring->head = nm_ring_next(ring, ring->head);
			ring->cur = ring->head;
		}
	}
	*/

	// Run over all frames processed by the workers and enqueue to netmap
	for(unsigned int worker=0; worker < numWorkers; worker++){
		frame frame;
		while(outRings[worker]->try_dequeue(frame)){
			// Check for discard/host/arp flag
			unsigned int ringid;
			if(frame.iface & frame::IFACE_HOST){
				ringid = numWorkers;
			} else if(frame.iface & frame::IFACE_ARP){
				ringid = numWorkers;
				arpTable.handleFrame(frame);
			} else if(frame.iface & frame::IFACE_DISCARD){
				// Just reclaim buffer
				freeBufs.push_back(NETMAP_BUF_IDX(netmapTxRings[0][0], frame.buf_ptr));
				continue;
			} else if(frame.iface & frame::IFACE_NOMAC){
				ringid = worker;
				ipv4* ipv4_hdr = reinterpret_cast<ipv4*>(frame.buf_ptr + sizeof(ether));
				uint32_t ip = ipv4_hdr->d_ip;
				auto it = missingMACs.find(ip);
				if(it == missingMACs.end()){
					macRequest mr;
					mr.ip = ip;
					mr.iface = frame.iface & frame::IFACE_ID;
					mr.time = steady_clock::now();
				} else {
					duration<double> diff = it->second.time - steady_clock::now();
					if(diff.count() < 0.5){
						// Give it a bit more time and just discard the frame
						freeBufs.push_back(NETMAP_BUF_IDX(netmapTxRings[0][0], frame.buf_ptr));
						continue;
					} else {
						// Send out a new ARP Request
						arpTable.prepareRequest(ip, frame.iface & frame::IFACE_ID, frame);
					}
				}
			} else {
				ringid = worker;
			}

			uint16_t iface = frame.iface & frame::IFACE_ID;
			netmap_ring* ring = netmapTxRings[iface][ringid];
			uint32_t slotIdx = ring->head;
			freeBufs.push_back(ring->slot[slotIdx].buf_idx);
			ring->slot[slotIdx].buf_idx = NETMAP_BUF_IDX(ring, frame.buf_ptr);
			ring->head = nm_ring_next(ring, ring->head);
			ring->cur = ring->head;

		}
	}


}
