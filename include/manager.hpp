#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <memory>
#include <vector>
#include "ring.hpp"
#include "worker.hpp"
#include "routingTable.hpp"
#include "arpTable.hpp"
#include "frame.hpp"

#include "netmap_user.h"
#include <sys/ioctl.h>

/*! Core manager class.
 * This class manages the whole router.
 * It sets up the worker threads, cares for up-to-date routing and ARP tables.
 * Handling of the network low-level network functions (netmap/dpdk) is done here.
 */
class Manager {
private:
	std::vector<Worker> worker;
	std::vector<Ring<frame>> rings;
	Ring<frame> hostQ;

	std::vector<netmap_ring> netmap_rings;
	std::vector<netmap_if> netmaps_ifs;
	nmreq nmreq_root;

	shared_ptr<RoutingTable> routingTable;
	ARPTable arpTable;

	enum enum_state {RUN, STOP};
	std::atomic<enum_state> state;

	void process();

public:
	/*! Initialize new Manager.
	 * Nothing big really
	 */
	Manager() {};

	/*! Start the router.
	 * This function enters the main action loop
	 */
	void run(){
		// TODO start the worker threads
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
