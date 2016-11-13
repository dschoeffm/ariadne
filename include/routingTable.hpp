#ifndef TABLE_HPP
#define TABLE_HPP

#include <iostream>
#include <vector>
#include <limits>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <stdint.h>

#include "util.hpp"

class RoutingTable {
public:
	struct route {
		uint32_t base;
		uint32_t next_hop;
		uint32_t prefix_length;
		uint16_t interface;
		uint16_t index; // Number of nextHop

		route() :
			base(uint32_t_max),
			next_hop(uint32_t_max),
			prefix_length(uint32_t_max),
			interface(uint16_t_max) {};

		route(const route& route) :
			base(route.base),
			next_hop(route.next_hop),
			prefix_length(route.prefix_length),
			interface(route.interface) {};

		operator bool(){
			if(base == uint32_t_max){
				return false;
			} else {
				return true;
			}
		};
	};

	struct nextHop {
		uint8_t mac[6] = {0};
		uint16_t interface = uint16_t_max;
	};

	static const route invalidRoute;

protected:
	std::shared_ptr<std::vector<std::vector<route>>>
		entries = std::make_shared<std::vector<std::vector<route>>>();

	std::shared_ptr<std::vector<nextHop>>
		nextHopList = std::make_shared<std::vector<nextHop>>();

	std::shared_ptr<std::vector<uint32_t>>
		nextHopMapping = std::make_shared<std::vector<uint32_t>>();

	std::shared_ptr<std::vector<std::unordered_map<uint32_t, uint8_t[6]>>>
		directlyConnected = std::make_shared<std::vector<std::unordered_map<uint32_t, uint8_t[6]>>>();

	virtual void updateInfo() {};

private:
	void buildNextHopList();
	void aggregate();
	std::unordered_set<uint16_t> interfaces;

public:
	void print_table();

	std::shared_ptr<std::vector<std::vector<route>>> getSortedRoutes();

	// From index to MAC / interface
	std::shared_ptr<std::vector<nextHop>> getNextHopList();

	// From index to IPv4
	std::shared_ptr<std::vector<uint32_t>> getNextHopMapping();

	// Array of mappings from IPv4 to MAC
	std::shared_ptr<std::vector<std::unordered_map<uint32_t, uint8_t[6]>>> getDirectlyConnected();

	void update(){
		updateInfo();
		aggregate();
		buildNextHopList();
		directlyConnected->resize(interfaces.size());
	};
};

#endif /* ROUTINGTABLE_HPP */
