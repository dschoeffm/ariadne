#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <stdint.h>
#include "util.hpp"

/*! Represents one hardware interface.
 * This struct includes all necessary information
 * to characterize one ethernet port
 */
struct Interface {
	std::array<uint8_t, 6> mac = {{0}}; //!< MAC address
	uint32_t netlinkIndex = uint32_t_max; //!< Index of the interface in the netlink context
	uint32_t netmapIndex = uint32_t_max; //!< Index of the interface in the netmap context
	std::vector<uint32_t> IPs; //!< All configured IPs
	std::string name = "noname"; //!< Name of the interface

	/*! Order by netmapIndex.
	 */
	bool operator< (const Interface& i) const {
		return netmapIndex > i.netmapIndex;
	};

	/*! Equal by netmapIndex.
	 */
	bool operator== (const Interface& i) const {
		return netmapIndex == i.netmapIndex;
	};

	/*! Equal by netmapIndex.
	 */
	bool operator== (uint32_t i) const {
		return netmapIndex == i;
	};

	/*! Equal by name.
	 */
	bool operator== (std::string str) const {
		return name == str;
	};

	std::string toString(){
		std::stringstream sstream;
		sstream << "\tName: " << name << std::endl;
		sstream << "\tMAC address: " << mac_to_str(mac) << std::endl;
		sstream << "\tNetlink Index: " << netlinkIndex << std::endl;
		sstream << "\tNetmap Index: " << netmapIndex << std::endl;
		sstream << "\tIP addresses: ";
		for(auto ip : IPs) {
			sstream << ip_to_str(ip) << std::endl << "\t              ";
		}
		return sstream.str();
	};
};

#endif /* INTERFACE_HPP */
