#ifndef __BASICTRIE_HPP__
#define __BASICTRIE_HPP__

#include <vector>
#include <utility>
#include <memory>

#include "table.hpp"

using namespace std;

class BasicTrie {
private:

	class Internal;
	class Leaf;

	class Internal {
	public:
		Internal* left;
		Internal* right;
		Internal* parent;
		Leaf* leaf;

		Internal(Internal* left, Internal* right, Internal* parent, Leaf* leaf);
	};

	class Leaf {
	public:
		uint32_t prefix_length;
		uint32_t base;
		uint32_t next_hop;
		uint32_t mask;

		Leaf(uint32_t prefix_length, uint32_t base, uint32_t next_hop);
	};

	Internal* root;

	Table& table;

	void buildTrie();

	inline uint32_t extractBit(uint32_t addr, int pos) {
		uint32_t mask = ((uint32_t) 1) << (31-pos);
		uint32_t bit = addr & mask;
		return (bit >> (31-pos));
	};

public:
	BasicTrie(Table& table);

	uint32_t route(uint32_t);
	void routeBatch(uint32_t* in, uint32_t* out, int count);
};

#endif /* __BASICTRIE_HPP__ */
