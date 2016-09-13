#include "flatroute.hpp"	
#include "util.hpp"

#include <cassert>
#include <limits>
#include <algorithm>

static uint32_t min_ip(uint32_t ip, uint32_t subnet) {
	return ip & PREFIX_MASK(subnet);
}

static uint32_t max_ip(uint32_t ip, uint32_t subnet) {
	return ip | ~PREFIX_MASK(subnet);
}

static void print_routes( std::vector<std::pair<uint32_t, uint32_t>> routes) {
	for (auto& pair: routes) {
		std::cout << "(" << ip_to_str(pair.first) << ", " << ip_to_str(pair.second) << "), ";
	}
	std::cout << std::endl;
}

static void insert(std::vector<std::pair<uint32_t, uint32_t>>& routes, uint32_t ip, uint32_t subnet, uint32_t next_hop) {
	uint32_t min = min_ip(ip, subnet);
	uint32_t max = max_ip(ip, subnet);
	//std::cout << "inserting " << ip_to_str(ip) << "/" << subnet << " range from " << ip_to_str(min) << " to " << ip_to_str(max) << std::endl;
	auto it = std::lower_bound(routes.begin(), routes.end(), min, [](std::pair<uint32_t, uint32_t> route, uint32_t ip) {
		return route.first < ip;
	});
	assert(it != routes.end());
	auto cur = it++;
	auto next = it;
	auto tmp = cur;
	auto prev = --tmp;
    //std::cout << "insertion position: "	<< ip_to_str(cur->first) << std::endl;
    //std::cout << "next position: "	<< ip_to_str(next->first) << std::endl;
	if (!subnet) {
		// allow overriding default route
		cur->second = next_hop;
		next->second = next_hop;
		return;
	}
	if (cur->first == min) { // insert at beginning of a route
		// precondition: no duplicates and ordered insert
		if (min == std::numeric_limits<uint32_t>::max()) { // annoying special case at end of array
			cur->second = next_hop;
		} else {
			cur->first = max + 1;
			routes.insert(cur, std::make_pair(min, next_hop));
		}
	} else if (next->first == max + 1 // insert at end of a route
			|| next->first == std::numeric_limits<uint32_t>::max()
			&& max == std::numeric_limits<uint32_t>::max()) { // annoying special case at end of array
		routes.insert(cur, std::make_pair(min, next_hop));
	} else { // insert in the middle of a route
		auto newEntry = routes.insert(cur, std::make_pair(max + 1, prev->second));
		routes.insert(newEntry, std::make_pair(min, next_hop));
	}
	//print_routes(routes);
	//std::cout << std::endl;
};

static void test(uint32_t ip, FlatRoute* lpm) {
	std::cout << "route to " << ip_to_str(ip) << " is via " << ip_to_str(lpm->route(ip)) << std::endl;
}


FlatRoute::FlatRoute(Table& table) {
	//std::list<std::pair<uint32_t, uint32_t>> routeList;
	// somhow faster with vector despite obvious O(n^2) fail
	std::vector<std::pair<uint32_t, uint32_t>> routeList;
	routeList.emplace_back(0, std::numeric_limits<uint32_t>::max());
	routeList.emplace_back(std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max());
	auto entries = table.get_sorted_entries();
	uint32_t inserted = 0;
	for (uint32_t i = 0; i < 33; ++i) {
		for (auto& e: entries[i]) {
			insert(routeList, e.first, i, e.second);
			if ((inserted++ & 0x4FFF) == 0) std::cout << "inserted " << inserted << std::endl;
		}
	}
	for (auto& e: routeList) {
		routes.push_back(e);
	}
	test(0x0A << 24, this);
	test(0x0B << 24, this);
	test(0x0A00000A, this);
	test(0x0A00003B, this);
	test(0x00000000, this);
	test(0xFFFFFFFF, this);
	test(0x00800000, this);
	test(0x00800008, this);
};

uint32_t FlatRoute::route(uint32_t addr) {
	auto it = std::lower_bound(routes.begin(), routes.end(), addr, [](std::pair<uint32_t, uint32_t> route, uint32_t ip) {
		return route.first < ip;
	});
	if (it != routes.begin() && it->first != addr) {
		return (it - 1)->second;
	} else {
		return it->second;
	}
};

