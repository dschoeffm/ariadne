#include "manager.hpp"
#include <string.h>

using namespace std;
using namespace moodycamel;
using namespace headers;
using namespace chrono;

void Manager::initNetmap(){

	if(geteuid() != 0){
		fatal("You need to be root in order to use Netmap!");
	}

	routingTable = make_shared<LinuxTable>(this->interfaces);
	//routingTable->print_table();

	memset(&nmreq_root, 0, sizeof(struct nmreq));

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
	nmreq_root.nr_arg3 = 256;

	int iface_num=0;

	for(auto iface : interfacesToUse){
		logInfo("Manager::initNetmap Preparing interface " + iface);
		int fd = 0;
		fds.push_back(fd = open("/dev/netmap", O_RDWR));
		strncpy(nmreq_root.nr_name, iface.c_str(), 16);
		ioctl(fd, NIOCREGIF, &nmreq_root);
		if(!mmapRegion){
			mmapRegion = mmap(0, nmreq_root.nr_memsize,
					PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			if(mmapRegion == MAP_FAILED){
				logErr("Manager::initNetmap() mmap() failed (netmap module loaded?)");
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
#ifdef DEBUG
		logDebug("Having " + int2str(freeBufs.size()) + " free buffers at the moment");
#endif

		shared_ptr<Interface> iface_ptr;
		for(auto i : interfaces){
			if(iface == i->name){
				iface_ptr = i;
				break;
			}
		}
		if(iface_ptr == nullptr){
			fatal("Manager::initNetmap something went wrong: netmap interface not found in netlink context");
		}

		iface_ptr->netmapIndex = iface_num++;
	}

	logInfo("Manager::initNetmap Interface preparations finished.\n");

	for(unsigned int i=0; i<numWorkers; i++){
		inRings.emplace_back(make_shared<ConcurrentQueue<frame>>(RING_SIZE));
		outRings.emplace_back(make_shared<ConcurrentQueue<frame>>(RING_SIZE));
	}

	routingTable->buildNextHopList();
	routingTable->print_table();
	numInterfaces = netmapIfs.size();

	curLPM = make_shared<LPM>(*(routingTable.get()));
};

std::vector<shared_ptr<Interface>> Manager::fillNetLink(){
	vector<shared_ptr<Interface>> interfaces = Netlink::getAllInterfaces();
	sort(interfaces.begin(), interfaces.end());
	return interfaces;
};

void Manager::startWorkerThreads(){
	arpTable.createCurrentTable(routingTable);
	for(unsigned i=0; i<numWorkers; i++){
		workers.push_back(new Worker(curLPM, arpTable.getCurrentTable(),
			inRings[i], outRings[i], interfaces, i));
	}
};

void Manager::startStatsThread(){
	auto statsFun = [&](vector<Worker*> workers){
		while(1){
			// Print workers
			for(auto w : workers){
				w->printAndClearStats();
			}

			// Print manager
			stringstream sstream;
			sstream << "Manager: ";
			sstream << "#Recv: " << statsNumRecv << ", ";
			sstream << "#Dropped: " << statsNumDropped << ", ";
			sstream << "#Transmitted: " << statsNumTransmitted;

			statsNumRecv = 0;
			statsNumDropped = 0;
			statsNumTransmitted = 0;

			logInfo(sstream.str());

			this_thread::sleep_for(chrono::milliseconds(1000));
		}
	};
	statsThread = new thread(statsFun, workers);
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
		// netmap rings
		for(unsigned int worker=0; worker < numWorkers; worker++){
			netmap_ring* ring = netmapRxRings[iface][worker];
			while(nm_ring_space(ring)){
				uint32_t numFrames = nm_ring_space(ring);
				numFrames = min(numFrames, (uint32_t) freeBufs.size());
				if(numFrames == 0){
					break;
				}
				if(numFrames > MANAGER_BULK_SIZE){
					numFrames = MANAGER_BULK_SIZE;
				}
				uint32_t slotIdx = ring->head;
				frame f[MANAGER_BULK_SIZE];
				statsNumRecv += numFrames;
				for(uint32_t i=0; i<numFrames; i++){

					f[i].buf_ptr = reinterpret_cast<uint8_t*>(
							NETMAP_BUF(ring, ring->slot[slotIdx].buf_idx));
					f[i].len = ring->slot[slotIdx].len;
					f[i].iface = iface;
					f[i].vlan = 0;

#ifdef DEBUG
					logDebug("Manager::process received frame from iface: " + int2str(f[i].iface)
							+ ", length: " + int2str(f[i].len)
							+ ", buf_idx: " + int2str(ring->slot[slotIdx].buf_idx)
							+ ", buf_ptr: 0x" + int2strHex((uint64_t) f[i].buf_ptr));
#endif

					ring->slot[slotIdx].buf_idx = freeBufs.back();
					ring->slot[slotIdx].flags = NS_BUF_CHANGED;
					freeBufs.pop_back();
#ifdef DEBUG
					logDebug("Manager::process replacement rx buf_idx: "
							+ int2str(ring->slot[slotIdx].buf_idx));
#endif
					slotIdx = nm_ring_next(ring, slotIdx);
					ring->head = slotIdx;
					ring->cur = slotIdx;
				}

				inRings[worker]->try_enqueue_bulk(f, numFrames);
#ifdef DEBUG
				logDebug("Manager::process enqueue new frames");
#endif
			}
		}
		// TODO host ring
	}

	// Run over all frames processed by the workers and enqueue to netmap
	for(unsigned int worker=0; worker < numWorkers; worker++){
		frame frame;
		while(outRings[worker]->try_dequeue(frame)){
			// Check for discard/host/arp flag
			unsigned int ringid;
			if(frame.iface & frame::IFACE_HOST){
#ifdef DEBUG
				logDebug("Manager::process Forwarding frame to kernel");
#endif
				ringid = numWorkers;
			} else if(frame.iface & frame::IFACE_ARP){
#ifdef DEBUG
				logDebug("Manager::process handling ARP frame");
#endif
				ringid = worker;
				arpTable.handleFrame(frame);
			} else if(frame.iface & frame::IFACE_NOMAC){
#ifdef DEBUG
				logDebug("Manager::process no MAC for target");
#endif
				ringid = worker;
				ipv4* ipv4_hdr = reinterpret_cast<ipv4*>(frame.buf_ptr + sizeof(ether));
				uint32_t ip = ipv4_hdr->d_ip;
				auto it = missingMACs.find(ip);
				if(it == missingMACs.end()){
#ifdef DEBUG
					stringstream sstream;
					sstream << "Manager::process no ARP request sent yet,";
					sstream << " sending now via interface ";
					sstream << (frame.iface & frame::IFACE_ID);
					logDebug(sstream.str());
#endif
					macRequest mr;
					mr.ip = ip;
					mr.iface = frame.iface & frame::IFACE_ID;
					mr.time = steady_clock::now();
					arpTable.prepareRequest(ip, frame.iface & frame::IFACE_ID, frame);
					missingMACs[ip] = mr;
				} else {
					duration<double> diff = it->second.time - steady_clock::now();
					if(diff.count() < 0.5){
					//if(false){
						// Give it a bit more time and just discard the frame
						frame.iface |= frame::IFACE_DISCARD;
#ifdef DEBUG
						logDebug("Manager::process no new ARP request");
#endif
					} else {
						// Send out a new ARP Request
						arpTable.prepareRequest(ip, frame.iface & frame::IFACE_ID, frame);
#ifdef DEBUG
						logDebug("Manager::process prepare new ARP request");
#endif
					}
				}
			} else {
				ringid = worker;
			}

			if(frame.iface & frame::IFACE_DISCARD){
				// Just reclaim buffer
#ifdef DEBUG
				logDebug("Manager::process discarding frame");
#endif
				statsNumDropped++;
				freeBufs.push_back(NETMAP_BUF_IDX(netmapTxRings[0][0], frame.buf_ptr));
				continue;
			}

			uint16_t iface = frame.iface & frame::IFACE_ID;
			netmap_ring* ring = netmapTxRings[iface][ringid];
			uint32_t slotIdx = ring->head;

			// Make sure frame has at least minimum size
			if(frame.len < 60){
				frame.len = 60;
			}

			if(nm_ring_space(ring)){
				freeBufs.push_back(ring->slot[slotIdx].buf_idx);
				ring->slot[slotIdx].buf_idx = NETMAP_BUF_IDX(ring, frame.buf_ptr);
				ring->slot[slotIdx].flags = NS_BUF_CHANGED;
				ring->slot[slotIdx].len = frame.len;
				ring->head = nm_ring_next(ring, ring->head);
				ring->cur = ring->head;
				statsNumTransmitted++;
			} else {
				freeBufs.push_back(NETMAP_BUF_IDX(ring, frame.buf_ptr));
				statsNumDropped++;
			}
#ifdef DEBUG
			logDebug("Manager::process sending frame to netmap,\n    iface: " + int2str(iface)
					+ ", slotIdx: " + int2str(slotIdx)
					+ ", buf_idx: " + int2str(ring->slot[slotIdx].buf_idx)
					+ ", length: " + int2str(ring->slot[slotIdx].len));

			logDebug("Manager::process Hexdump of frame:");

			neolib::hex_dump(NETMAP_BUF(ring, ring->slot[slotIdx].buf_idx),
				ring->slot[slotIdx].len , cerr);
#endif

		}
	}
}

void Manager::printInterfaces(){
#ifdef DEBUG
	for(auto i : interfaces){
		logDebug(i->toString());
	}
#endif
};
