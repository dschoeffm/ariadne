#ifndef NAIVE_HPP
#define NAIVE_HPP

#include <vector>
#include <map>

#include "routingTable.hpp"

/*! Naive LPM implementation.
 * Interates over all the routes
 */
class Naive {
private:
	std::shared_ptr<std::vector<std::vector<RoutingTable::route>>> entries;
	uint32_t masks[33];

public:
	/*! Set up a new Naive LPM object.
	 * \param table Routing table to use
	 */
	Naive(RoutingTable& table);

	/*! Route one IPv4 address
	 * \param addr IPv4 address to route
	 * \return index to the next hop
	 */
	nh_index route(uint32_t addr) const;
};

#endif /* NAIVE_HPP */
