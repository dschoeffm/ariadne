#ifndef NETLINK_HPP
#define NETLINK_HPP

#include <vector>
#include <memory>
#include <algorithm>

#include "interface.hpp"

#include <libmnl/libmnl.h>
//#include <linux/if.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <string.h>

/*! Handle Netlink calls.
 */
class Netlink {
public:
	/*! Return all interfaces of the router (unordered).
	 */
	static std::shared_ptr<std::vector<interface>> getAllInterfaces();
};

#endif /* NETLINK_HPP */
