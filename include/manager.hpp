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

#include "netmap_user.h"
#include <sys/ioctl.h>

/*! Core manager class.
 * This class manages the whole router.
 * It sets up the worker threads, cares for up-to-date routing and ARP tables.
 * Handling of the network low-level network functions (netmap/dpdk) is done here.
 */
class Manager {
private:
	std::vector<Worker*> worker;
	std::vector<Ring<frame>> in_rings;
	std::vector<Ring<frame>> out_rings;
	Ring<frame> hostQ;
	std::shared_ptr<LPM> cur_lpm;

	std::vector<netmap_ring> netmap_rings;
	std::vector<netmap_if> netmaps_ifs;
	nmreq nmreq_root;

	std::vector<std::string> interfaces_to_use;
	std::shared_ptr<std::vector<interface>> interfaces = fillNetLink();

	shared_ptr<RoutingTable> routingTable;
	ARPTable arpTable;

	enum enum_state {RUN, STOP};
	std::atomic<enum_state> state;

	void process();

	static std::shared_ptr<std::vector<interface>> fillNetLink();

	void startWorkerThreads();

public:
	/*! Initialize new Manager.
	 * Nothing big really
	 */
	Manager(std::vector<std::string> interfaces_to_use)
		: interfaces_to_use(interfaces_to_use),
		arpTable(interfaces) {};

	/*! Start the router.
	 * This function enters the main action loop
	 */
	void run(){
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
