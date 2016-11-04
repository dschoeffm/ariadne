#ifndef TABLE_HPP
#define TABLE_HPP

#include <iostream>
#include <vector>
#include <limits>
#include <memory>

#include "util.hpp"

class RoutingTable {
public:
	struct route {
		uint32_t base;
		uint32_t next_hop;
		uint32_t prefix_length;
		uint32_t interface;

		route() :
			base(std::numeric_limits<uint32_t>::max()),
			next_hop(std::numeric_limits<uint32_t>::max()),
			prefix_length(std::numeric_limits<uint32_t>::max()),
			interface(std::numeric_limits<uint32_t>::max()) {};

		route(const route& route) :
			base(route.base),
			next_hop(route.next_hop),
			prefix_length(route.prefix_length),
			interface(route.interface) {};

		operator bool(){
			if(base == std::numeric_limits<uint32_t>::max()){
				return false;
			} else {
				return true;
			}
		};

	};

	static const route invalidRoute;

public:
	void print_table();
	virtual std::shared_ptr<std::vector<std::vector<route>>> getSortedRoutes() = 0;
};

#endif /* ROUTINGTABLE_HPP */
