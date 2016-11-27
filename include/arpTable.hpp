#ifndef ARPTABLE_HPP
#define ARPTABLE_HPP

#include <memory>
#include <vector>
#include <stdint.h>
#include <unordered_map>
#include <array>

#include "util.hpp"
#include "routingTable.hpp"
#include "frame.hpp"

/*! Manages the complete IPv4 -> MAC mapping.
 * Collected mapping as its internal state.
 * Produces distilled tables for specific routing tables.
 */
class ARPTable {
public:
	/*! Describes one next Hop.
	 */
	struct nextHop {
		uint8_t mac[6] = {0}; //!< MAC address
		uint16_t interface = uint16_t_max; //!< interface number
	};

	/*! ARP table corresponding to one routing table.
	 * Indices given by the RT need to be used.
	 */
	struct table {
		std::vector<nextHop> nextHops; //!< next hops, index by RT
		std::vector<std::unordered_map<uint32_t, std::array<uint8_t,6>>>&
			directlyConnected; //!< directly connected next hops, indexed by interface -> IPv4
	};

private:

	// Current routing table
	std::shared_ptr<RoutingTable> routingTable;

	// Current Table
	std::shared_ptr<table> currentTable = std::make_shared<table>();

	// Inter-table Mapping Interface/IPv4 -> MAC (this is core data)
	// Next Hops are directly attached and NOT inside the routing table
	std::vector<std::unordered_map<uint32_t, std::array<uint8_t,6>>> directlyConnected;

	// Inter-table mapping IPv4 -> MAC/Interface (this is the core data)
	// Next Hops are inside the routin table
	std::unordered_map<uint32_t, nextHop> mapping;

public:
	/*! Create new empty ARP table.
	 * This create an ARP table which is not associated with any next hop data
	 */
	ARPTable(){};

	/*! Adapt to new routing table.
	 * Update the internal state of the ARP table to match the new routing table
	 */
	void createCurrentTable(std::shared_ptr<RoutingTable> routingTable);

	/*! Return the current ARP table.
	 * This function return the ARP table in correspondance to the current
	 * routing table. Indices given by the routing table need to be used.
	 *
	 * \return The current ARP lookup table
	 */
	std::shared_ptr<table> getCurrentTable(){
		return currentTable;
	};

	/*! Prepare a ARP request.
	 * An ARP request is prepared for the given IPv4 address
	 * The ethernet header is set accordingly.
	 *
	 * \param ip IPv4 address
	 * \param interface interface to prepare request for
	 * \param frame frame to write the request in
	 */
	void prepareRequest(uint32_t ip, uint16_t interface, frame& frame);

};

#endif /* ARPTABLE_HPP */
