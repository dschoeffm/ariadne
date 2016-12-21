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
#include "headers.hpp"
#include "interface.hpp"

// Forward declaration
// circular dependencies...
class RoutingTable;

/*! Manages the complete IPv4 -> MAC mapping.
 * Collected mapping as its internal state.
 * Produces distilled tables for specific routing tables.
 */
class ARPTable {
public:
	/*! Describes one next Hop.
	 */
	struct nextHop {
		std::array<uint8_t,6> mac {{0}}; //!< MAC address
		uint16_t interface = uint16_t_max; //!< interface number
	};

	/*! ARP table corresponding to one routing table.
	 * Indices given by the RT need to be used.
	 */
	struct table {
		std::vector<nextHop> nextHops; //!< next hops, index by RT
		std::unordered_map<uint32_t, nextHop>&
			directlyConnected; //!< directly connected next hops, indexed by IPv4
		/*! Constructor setting all members
		 */
		table(std::vector<nextHop> nextHops,
				std::unordered_map<uint32_t, nextHop>& directlyConnected)
			: nextHops(nextHops), directlyConnected(directlyConnected){};
	};

private:

	// Current routing table
	std::shared_ptr<RoutingTable> routingTable;

	// Current Table
	std::shared_ptr<table> currentTable
		= std::make_shared<table>(std::vector<nextHop>(), directlyConnected);

	// Inter-table Mapping IPv4 -> MAC (this is core data)
	// Next Hops are directly attached and NOT inside the routing table
	std::unordered_map<uint32_t, nextHop> directlyConnected;

	// Inter-table mapping IPv4 -> MAC/Interface (this is the core data)
	// Next Hops are inside the routing table
	std::unordered_map<uint32_t, nextHop> mapping;

	// Interfaces by OS index
	std::shared_ptr<std::vector<interface>> interfaces;

public:
	/*! Create new empty ARP table.
	 * This create an ARP table which is not associated with any next hop data
	 *
	 * \param interfaces Interfaces of the router
	 */
	ARPTable(std::shared_ptr<std::vector<interface>> interfaces)
	: interfaces(interfaces) {};

	/*! Adapt to new routing table.
	 * Update the internal state of the ARP table to match the new routing table
	 *
	 * \param routingTable The current routing Table
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

	/*! Handle an ARP Reply.
	 * This function updates the internal state of the current ARP table
	 * according to the newly arrived ARP frame
	 *
	 * \param frame the frame holding the ARP Response
	 */
	void handleReply(frame& frame);

	/*! Handle an ARP Request.
	 * This function prepares a reply to respond to the incoming request.
	 *
	 * \param frame the frame holding the ARP Request
	 */
	void handleRequest(frame& frame);

	/*! Handle an ARP frame.
	 * This is a wrapper around handleReply and handleResponse.
	 *
	 * \param frame the ARP frame
	 */
	void handleFrame(frame& frame);
};

#endif /* ARPTABLE_HPP */
