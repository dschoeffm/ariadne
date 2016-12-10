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

#include "netmap_user.h"
#include <sys/ioctl.h>

#include <libmnl/libmnl.h>
//#include <linux/if.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>


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

	shared_ptr<RoutingTable> routingTable;
	ARPTable arpTable;

	std::vector<std::array<uint8_t, 6>> interface_macs;
	std::vector<std::unordered_set<uint32_t>> own_IPs;
	std::vector<std::string> interface_names;
	std::unordered_set<std::string> interfaces;

	enum enum_state {RUN, STOP};
	std::atomic<enum_state> state;

	void process();

	void fillNetLink();

	void startWorkerThreads();

public:
	/*! Initialize new Manager.
	 * Nothing big really
	 */
	Manager(std::unordered_set<std::string> interfaces)
		: arpTable(interface_macs, own_IPs)
		, interfaces(interfaces) {
		fillNetLink();};

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
