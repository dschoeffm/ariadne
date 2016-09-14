#include "flatroute.hpp"	
#include "util.hpp"

#include <cassert>
#include <limits>
#include <algorithm>
#include <iterator>

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

static auto constexpr DEBUG = false;

static void insert(std::vector<std::pair<uint32_t, uint32_t>>& routes, uint32_t ip, uint32_t subnet, uint32_t nextHop) {
	uint32_t min = min_ip(ip, subnet);
	uint32_t max = max_ip(ip, subnet);
	if (DEBUG) std::cout << "inserting " << ip_to_str(ip) << "/" << subnet << " range from " << ip_to_str(min) << " to " << ip_to_str(max) << std::endl;
	auto it = std::lower_bound(routes.begin(), routes.end(), max, [](std::pair<uint32_t, uint32_t> route, uint32_t ip) {
		return route.first < ip;
	});
	assert(it != routes.end());
	if (DEBUG) std::cout << "insertion position: "	<< ip_to_str(it->first) << std::endl;
	if (!subnet) {
		// allow overriding default route
		assert(routes.size() == 1);
		it->second = nextHop;
		return;
	}
	// insert at the beginning of a route
	if ((it == routes.begin() && min == 0)
	 || (it != routes.begin() && std::prev(it)->first + 1 == min)) {
		if (DEBUG) std::cout << "beginning" << std::endl;
		routes.insert(it, std::make_pair(max, nextHop));
		return;
	}
	// insert at the end of a route
	if (it->first == max) {
		if (DEBUG) std::cout << "end" << std::endl;
		// min cannot be 0 for an insertion at the end, otherwise we have a duplicate route (or the default)
		it->first = min - 1;
		routes.insert(std::next(it), std::make_pair(max, nextHop));
		return;
	}
	if (DEBUG) std::cout << "middle" << std::endl;
	// insert in the middle of a route
	auto curNextHop = it->second;
	auto newIt = routes.insert(it, std::make_pair(max, nextHop));
	routes.insert(newIt, std::make_pair(min - 1, curNextHop)); // min is obviously not 0 in the middle of a route
};

static void test(uint32_t ip, FlatRoute* lpm) {
	std::cout << "route to " << ip_to_str(ip) << " is via " << ip_to_str(lpm->route(ip)) << std::endl;
}


FlatRoute::FlatRoute(Table& table) {
	//std::list<std::pair<uint32_t, uint32_t>> routeList;
	// fail insertion is faster than fail search
	std::vector<std::pair<uint32_t, uint32_t>> routeList;
	routeList.emplace_back(std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max());
	auto entries = table.get_sorted_entries();
	uint32_t inserted = 0;
	for (uint32_t i = 0; i < 33; ++i) {
		for (auto& e: entries[i]) {
			insert(routeList, e.first, i, e.second);
			if ((inserted++ & 0x4FFF) == 0) std::cout << "inserted " << inserted << std::endl;
			if (DEBUG) print_routes(routeList);
			if (DEBUG) std::cout << std::endl;
		}
	}
	for (auto& e: routeList) {
		routes.push_back(e);
	}
	std::cout << routes.size() << std::endl;
	test(0x0A << 24, this);
	test(0x0B << 24, this);
	test(0x0A00000A, this);
	test(0x0A00003B, this);
	test(0x00000000, this);
	test(0xFFFFFFFF, this);
	test(0x00800000, this);
	test(0x00800008, this);
	test(0xFFFFFFFE, this);
};

uint32_t FlatRoute::route(uint32_t addr) {
	auto it = std::lower_bound(routes.begin(), routes.end(), addr, [](std::pair<uint32_t, uint32_t> route, uint32_t ip) {
		return route.first < ip;
	});
	return it->second;
};

