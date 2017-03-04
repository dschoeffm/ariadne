#ifndef TABLE_HPP
#define TABLE_HPP

#include <iostream>
#include <vector>
#include <limits>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <stdint.h>
#include <sstream>

#include "util.hpp"
#include "arpTable.hpp"
#include "interface.hpp"

using nh_index = uint16_t;

/*! Abstract Routing Table class.
 * This class should be inherited from, when implementing a source of routing information.
 */
class RoutingTable {
public:
	/*! One route.
	 * This struct holds all the necessary information to decribe one specific route.
	 */
	struct route {
		uint32_t base; //!< base address of the route
		uint32_t next_hop; //!< next hop IPv4 address
		uint32_t prefix_length; //!< prefix length of the route
		std::shared_ptr<Interface> interface; //!< interface number
		nh_index index; //!< Index of the next Hop
		static constexpr nh_index NH_INVALID = uint16_t_max;
		static constexpr nh_index NH_DIRECTLY_CONNECTED = (uint16_t_max -1);

		route() :
			base(uint32_t_max),
			next_hop(uint32_t_max),
			prefix_length(uint32_t_max),
			interface(new struct Interface()),
			index(NH_INVALID) {};

		/*! Copy Constructor
		 */
		route(const route& route) :
			base(route.base),
			next_hop(route.next_hop),
			prefix_length(route.prefix_length),
			interface(route.interface),
	   		index(route.index) {};

		/*! Return if this route is valid
		 * Valid routes have a base address of not all 1s
		 */
		operator bool(){
			if(base == uint32_t_max){
				return false;
			} else {
				return true;
			}
		};

		bool operator< (const route& route) const {
			if(prefix_length < route.prefix_length){
				return true;
			}

			if(base < route.base){
				return true;
			}

			return false;
		};
	};

	struct nh_abstract {
		uint32_t nh_ip;
		uint16_t interface;
		nh_index index;
	};


	static const route invalidRoute; //!< invalid Route

protected:
	/*! All of the routing information is contained here. */
	std::shared_ptr<std::vector<std::vector<route>>>
		entries = std::make_shared<std::vector<std::vector<route>>>();

	/*! Mapping from next hop index to IPv4 addresses. */
	std::shared_ptr<std::vector<nh_abstract>>
		nextHopMapping = std::make_shared<std::vector<nh_abstract>>();

	/*! Update the underlying routing infomation.
	 * Implementation is up to the child class
	 */
	virtual void updateInfo() {};

private:
	void aggregate();
	std::unordered_set<uint16_t> interfaces;

public:
	/*! Print the routing table.
	 * Similiar to "ip r"
	 */
	void print_table();

	/*! Get the routes currently held inside the routing table.
	 * \return routes sorted by prefix length (first index), and base address
	 */
	std::shared_ptr<std::vector<std::vector<route>>> getSortedRoutes();

	/*! From index to IPv4.
	 * Get a mapping from the next hop index to an IPv4 address
	 * \return mapping
	 */
	std::shared_ptr<std::vector<nh_abstract>> getNextHopMapping(){
		return nextHopMapping;
	};

	/*! Get the set of used interfaces.
	 * Return the set of used interface
	 * \return set of interfaces
	 */
	std::unordered_set<uint16_t> getInterfaceSet();

	/*! Update the routing information.
	 * This function updates the routing information and preprocesses it for further usage.
	 */
	void update(){
		updateInfo();
		aggregate();
		buildNextHopList();
	};

	/*! Rebuild the next hop list without updating the RIB.
	 * This is useful in case the interface information within routes change
	 */
	void buildNextHopList();

};

#endif /* ROUTINGTABLE_HPP */
