#ifndef __DXR_HPP__
#define __DXR_HPP__

#include <list>
#include <vector>
#include <iostream>
#include <string>

#include "table.hpp"
#include "util.hpp"

class DXR {
private:
	struct expand_entry {
		uint32_t start;
		uint32_t end;
		uint32_t next_hop;
	};
	list<struct expand_entry> expansion;
	void expand();

#define DXR_FORMAT 0x00000001 // 1 = long, 0 = short
#define DXR_SIZE 0x00001ffe
#define DXR_SIZE_SHIFT 1
#define DXR_POSITION 0xffffe000 // in multiple of 4 byte
#define DXR_POSITION_SHIFT 13

	vector<uint32_t> lookup_table;
	struct range_long {
		uint16_t start;
		uint16_t next_hop;
	};
	vector<struct range_long> range_table; // only long format for now
	vector<uint32_t> next_hop_table;
	void reduce();

	const vector<map<uint32_t, uint32_t>>& entries;

public:
	DXR(Table& table);

	uint32_t route(uint32_t);

	void print_expansion();
	void print_tables();
};

#endif /*__DXR_HPP__*/
