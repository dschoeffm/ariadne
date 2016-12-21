#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <unordered_set>
#include <array>
#include <algorithm>

#include "ring.hpp"
#include "worker.hpp"
#include "routingTable.hpp"
#include "arpTable.hpp"
#include "frame.hpp"
#include "netlink.hpp"
#include "interface.hpp"
#include "linuxTable.hpp"

#include <net/netmap_user.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>
#include <signal.h>

/*! Core manager class.
 * This class manages the whole router.
 * It sets up the worker threads, cares for up-to-date routing and ARP tables.
 * Handling of the network low-level network functions (netmap/dpdk) is done here.
 */
class Manager {
private:
	std::vector<Worker*> workers;
	Ring<frame>* inRings;
	Ring<frame>* outRings;
	std::vector<uint32_t> freeBufs;
	std::shared_ptr<LPM> curLPM;
	unsigned int numWorkers;

	// Last ring is host ring
	std::vector<std::vector<netmap_ring*>> netmapRxRings;
	std::vector<std::vector<netmap_ring*>> netmapTxRings;
	std::vector<netmap_if*> netmapIfs;
	nmreq nmreq_root;
	std::vector<int> fds;
	void* mmapRegion;
	unsigned int numInterfaces;

	std::vector<std::string> interfacesToUse;
	std::shared_ptr<std::vector<interface>> interfaces = fillNetLink();

	shared_ptr<RoutingTable> routingTable;
	ARPTable arpTable;

	enum enum_state {RUN, STOP};
	std::atomic<enum_state> state;

	void process();

	static std::shared_ptr<std::vector<interface>> fillNetLink();

	void initNetmap();
	void startWorkerThreads();

public:
	/*! Initialize new Manager.
	 * Nothing big really
	 */
	Manager(std::vector<std::string> interfacesToUse) :
		numWorkers(thread::hardware_concurrency()-1),
		interfacesToUse(interfacesToUse),
		arpTable(interfaces) {};

	/*! Start the router.
	 * This function enters the main action loop
	 */
	void run(){
		initNetmap();
		startWorkerThreads();
		while(state.load() == RUN){
			process();
		}
	}

	/*! Stops the router
	 * This function gracefully stops the router
	 */
	void stop(){
		state.store(STOP);
	}
};

#endif /* MANAGER_HPP */
