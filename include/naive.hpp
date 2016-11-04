#ifndef NAIVE_HPP
#define NAIVE_HPP

#include <vector>
#include <map>

#include "routingTable.hpp"

class Naive {
private:
	std::shared_ptr<std::vector<std::vector<RoutingTable::route>>> entries;
	uint32_t masks[33];

public:
	Naive(RoutingTable& table);

	const RoutingTable::route& route(uint32_t addr);
};

#endif /* NAIVE_HPP */
