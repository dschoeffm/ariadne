#ifndef NAIVE_HPP
#define NAIVE_HPP

#include <vector>
#include <map>

#include "table.hpp"

class Naive {
private:
	const std::vector<std::map<uint32_t, uint32_t>>& entries;
	uint32_t masks[33];

public:
	Naive(Table& table);

	uint32_t route(uint32_t);
};

#endif /* NAIVE_HPP */
