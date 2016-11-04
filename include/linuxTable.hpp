#ifndef LINUX_TABLE_HPP
#define LINUX_TABLE_HPP

#include <iostream>
#include <vector>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libmnl/libmnl.h>
#include <linux/if.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>

#include "util.hpp"
#include "routingTable.hpp"

class LinuxTable : public RoutingTable {
private:
	std::shared_ptr<std::vector<std::vector<route>>> entries;

public:
	LinuxTable();

	std::shared_ptr<std::vector<std::vector<route>>> getSortedRoutes();
	void update();
};

#endif /* LINUX_HPP */
