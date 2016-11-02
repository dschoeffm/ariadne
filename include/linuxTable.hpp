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
#include "table.hpp"

class LinuxTable : public Table {
private:
	std::vector<std::vector<route>>* entries;

public:
	LinuxTable();

	const std::vector<std::vector<route>>& getSortedRoutes();
};

#endif /* LINUX_HPP */
