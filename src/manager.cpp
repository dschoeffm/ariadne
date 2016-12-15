#include "manager.hpp"

using namespace std;

void Manager::initNetmap(){
	nmreq_root.nr_version = NETMAP_API;
	nmreq_root.nr_tx_slots = 2048;
	nmreq_root.nr_rx_slots = 2048;
	nmreq_root.nr_tx_rings = numWorkers;
	nmreq_root.nr_rx_rings = numWorkers;
	nmreq_root.nr_ringid = NR_REG_ALL_NIC | NETMAP_DO_RX_POLL;
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
			cerr << "something went wrong: netmap interface not found in netlink context" << endl;
			exit(0);
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
		worker.push_back(new Worker(curLPM, arpTable.getCurrentTable(),
			inRings[i], outRings[i], interfaces));
	}
};

void Manager::process(){
	pollfd* pfds = new pollfd[numInterfaces];
	for(unsigned int i=0; i<numInterfaces; i++){
		pfds[i].fd = fds[i];
		pfds[i].events = POLLOUT | POLLIN;
	}
	if(0 > poll(pfds, numInterfaces, -1)){
		cerr << "poll() failed" << endl;
		exit(0);
	}

	// TODO rest
}
