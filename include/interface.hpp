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
	uint32_t index; //!< Index of the Interface
	std::vector<uint32_t> IPs; //!< All configured IPs
	std::string name; //!< Name of the interface

	/*! Order by index.
	 */
	bool operator< (const interface& i) const {
		return index > i.index;
	};

	/*! Equal by index.
	 */
	bool operator== (const interface& i) const {
		return index == i.index;
	};

	/*! Equal by index.
	 */
	bool operator== (uint32_t i) const {
		return index == i;
	};

	/*! Minimalistic constructor.
	 */
	interface(uint32_t index) : index(index) {};
};

#endif /* INTERFACE_HPP */
