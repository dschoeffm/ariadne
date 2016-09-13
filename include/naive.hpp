#ifndef NAIVE_HPP
#define NAIVE_HPP

#include <vector>
#include <map>

#include "table.hpp"
#include "lpm.hpp"

class Naive : public LPM {
private:
	const std::vector<std::map<uint32_t, uint32_t>>& entries;
	uint32_t masks[33];

public:
	Naive(Table& table, bool texOutput = false);

	uint32_t route(uint32_t);
};

#endif /* NAIVE_HPP */
