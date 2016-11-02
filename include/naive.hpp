#ifndef NAIVE_HPP
#define NAIVE_HPP

#include <vector>
#include <map>

#include "table.hpp"

class Naive {
private:
	const std::vector<std::vector<Table::route>>& entries;
	uint32_t masks[33];

public:
	Naive(Table& table);

	const Table::route& route(uint32_t addr);
};

#endif /* NAIVE_HPP */
