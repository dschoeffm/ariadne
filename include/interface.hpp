#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include <array>
#include <vector>
#include <string>
#include <stdint.h>

/*! Represents one hardware interface.
 * This struct includes all necessary information
 * to characterize one ethernet port
 */
struct interface {
	std::array<uint8_t, 6> mac = {{0}}; //!< MAC address
	uint32_t netlinkIndex; //!< Index of the interface in the netlink context
	uint32_t netmapIndex; //!< Index of the interface in the netmap context
	std::vector<uint32_t> IPs; //!< All configured IPs
	std::string name; //!< Name of the interface

	/*! Order by netmapIndex.
	 */
	bool operator< (const interface& i) const {
		return netmapIndex > i.netmapIndex;
	};

	/*! Equal by netmapIndex.
	 */
	bool operator== (const interface& i) const {
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
};

#endif /* INTERFACE_HPP */
