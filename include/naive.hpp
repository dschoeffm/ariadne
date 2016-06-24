#ifndef __NAIVE_HPP__
#define __NAIVE_HPP__

#include <bitset>

#include "table.hpp"

class Naive {
private:
	const vector<list<pair<uint32_t, uint32_t>>>& entries;
	uint32_t masks[33];

public:
	Naive(Table& table);

	uint32_t route(uint32_t);
};

#endif /*__NAIVE_HPP__*/
