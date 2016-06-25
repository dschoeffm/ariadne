#ifndef __DXR_HPP__
#define __DXR_HPP__

#include <list>
#include <iostream>

#include "table.hpp"

class DXR {
private:
	struct expand_entry {
		uint32_t start;
		uint32_t end;
		uint32_t next_hop;
	};
	list<struct expand_entry> expansion;
	const vector<map<uint32_t, uint32_t>>& entries;

public:
	DXR(Table& table);

	uint32_t route(uint32_t);

	void print_expansion();
};

#endif /*__DXR_HPP__*/
