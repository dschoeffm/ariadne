#ifndef NETLINK_HPP
#define NETLINK_HPP

#include <vector>
#include <memory>
#include <algorithm>
#include <sstream>
#include <string>

#include "interface.hpp"
#include "util.hpp"

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
	static std::vector<std::shared_ptr<Interface>> getAllInterfaces();
};

#endif /* NETLINK_HPP */
