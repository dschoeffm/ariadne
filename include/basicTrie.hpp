#ifndef BASICTRIE_HPP
#define BASICTRIE_HPP

#include "table.hpp"
#include "util.hpp"

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

public:
	BasicTrie(Table& table);

	uint32_t route(uint32_t);
	void routeBatch(uint32_t* in, uint32_t* out, int count);
};

#endif /* BASICTRIE_HPP */
