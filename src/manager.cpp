#include "manager.hpp"

using namespace std;

void Manager::initNetmap(){
	nmreq_root.nr_version = NETMAP_API;
	nmreq_root.nr_tx_slots = 2048;
	nmreq_root.nr_rx_slots = 2048;
	nmreq_root.nr_tx_rings = numWorkers;
	nmreq_root.nr_rx_rings = numWorkers;
	nmreq_root.nr_ringid = NR_REG_NIC_SW | NETMAP_DO_RX_POLL;
	nmreq_root.nr_cmd = 0;
	nmreq_root.nr_arg1 = 0;
	nmreq_root.nr_arg2 = 1;
	nmreq_root.nr_arg3 = 2048;

	int iface_num=0;

	for(auto iface : interfacesToUse){
		int fd;
		fds.push_back(fd = open("/dev/netmap", O_RDWR));
		strncpy(nmreq_root.nr_name, iface.c_str(), 16);
		ioctl(fd, NIOCREGIF, &nmreq_root);
		if(!mmapRegion){
			mmapRegion = mmap(0, nmreq_root.nr_memsize,
					PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
		}
		netmap_if* nifp = NETMAP_IF(mmapRegion, nmreq_root.nr_offset);
		netmapIfs.push_back(nifp);
		for(uint32_t i=0; i<nifp->ni_tx_rings; i++){
			netmapTxRings[iface_num].push_back(NETMAP_TXRING(nifp, i));
		}
		for(uint32_t i=0; i<nifp->ni_rx_rings; i++){
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

	inRings = new Ring<frame> [numWorkers];
	outRings = new Ring<frame> [numWorkers];

	routingTable = make_shared<LinuxTable>();
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
	for(unsigned i=0; numWorkers; i++){
		workers.push_back(new Worker(curLPM, arpTable.getCurrentTable(),
			inRings[i], outRings[i], interfaces, this));
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
	for(unsigned int iface=0; iface < numInterfaces; iface++){
		// numWorkers+1 for host ring
		for(unsigned int worker=0; worker < numWorkers+1; worker++){
			netmap_ring* ring = netmapRxRings[iface][worker];
			uint32_t numFrames = nm_ring_space(ring);
			numFrames = min(numFrames, (uint32_t) freeBufs.size());
			vector<frame> frames;
			uint32_t slotIdx = ring->head;

			for(uint32_t frame=0; frame < numFrames; frame++){
				frames.emplace_back(
						NETMAP_BUF(ring, slotIdx),
						ring->slot[slotIdx].len,
						iface,
						0);
				ring->slot[slotIdx].buf_idx = freeBufs.back();
				freeBufs.pop_back();
				slotIdx = nm_ring_next(ring, slotIdx);
				ring->head = slotIdx;
				ring->cur = slotIdx;
			}

			inRings[iface].push(frames);
		}
	}

	// Run over all frames processed by the workers and enqueue to netmap
	for(unsigned int worker=0; worker < numWorkers; worker++){
		vector<frame> frames;
		outRings[worker].pop(frames);
		for(frame& frame : frames){
			// Check for discard/host/arp flag
			if(frame.iface & frame::IFACE_DISCARD){
				// Just reclaim buffer
				freeBufs.push_back(NETMAP_BUF_IDX(netmapTxRings[0][0], frame.buf_ptr));
				continue;
			}

			unsigned int ringid;
			if(frame.iface & frame::IFACE_HOST){
				ringid = numWorkers;
			} else if(frame.iface & frame::IFACE_ARP){
				ringid = numWorkers;
				arpTable.handleFrame(frame);
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
