#ifndef FLAT_ROUTE_HPP
#define FLAT_ROUTE_HPP

#include <vector>
#include <map>

#include "table.hpp"

class FlatRoute {
private:
	std::vector<std::pair<uint32_t, uint32_t>> routes;

public:
	FlatRoute(Table& table);

	uint32_t route(uint32_t);
};

#endif
