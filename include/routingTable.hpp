#ifndef TABLE_HPP
#define TABLE_HPP

#include <iostream>
#include <vector>
#include <limits>
#include <memory>
#include <algorithm>

#include "util.hpp"

class RoutingTable {
public:
	struct route {
		uint32_t base;
		uint32_t next_hop;
		uint32_t prefix_length;
		uint16_t interface;
		uint16_t index;

		route() :
			base(std::numeric_limits<uint32_t>::max()),
			next_hop(std::numeric_limits<uint32_t>::max()),
			prefix_length(std::numeric_limits<uint32_t>::max()),
			interface(std::numeric_limits<uint16_t>::max()) {};

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

	struct nextHop {
		uint8_t mac[6] = {0};
		uint16_t interface = std::numeric_limits<uint16_t>::max();
	};

	static const route invalidRoute;

protected:
	std::shared_ptr<std::vector<std::vector<route>>> entries =
		std::make_shared<std::vector<std::vector<route>>>();
	std::shared_ptr<std::vector<nextHop>> nextHopList =
		std::make_shared<std::vector<nextHop>>();
	std::shared_ptr<std::vector<uint32_t>> nextHopMapping=
		std::make_shared<std::vector<uint32_t>>();
	virtual void updateInfo() {};

private:
	void buildNextHopList();
	void aggregate();

public:
	void print_table();
	std::shared_ptr<std::vector<std::vector<route>>> getSortedRoutes();
	std::shared_ptr<std::vector<nextHop>> getNextHopList();
	std::shared_ptr<std::vector<uint32_t>> getNextHopMapping();
	void update(){
		updateInfo();
		aggregate();
		buildNextHopList();
	};
};

#endif /* ROUTINGTABLE_HPP */
