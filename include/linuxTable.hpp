#ifndef LINUX_TABLE_HPP
#define LINUX_TABLE_HPP

#include <iostream>
#include <vector>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libmnl/libmnl.h>
#include <net/if.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>

#include "util.hpp"
#include "routingTable.hpp"

/*! Linux kernel based routing table.
 * Extracts the routing information from the Linux kernel via the netlink API
 */
class LinuxTable : public RoutingTable {
protected:
	/*! Update the current state with the kernel information.
	 * Reads the current kernel routing table
	 */
	void updateInfo();

public:
	/*! Create a new kernel based routing table.
	 *
	 * \param ifaces all the interfaces to be used
	 */
	LinuxTable(std::vector<std::shared_ptr<Interface>> ifaces);
};

#endif /* LINUX_HPP */
