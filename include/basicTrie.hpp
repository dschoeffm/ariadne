#ifndef BASICTRIE_HPP
#define BASICTRIE_HPP

#include <limits>

#include "table.hpp"
#include "util.hpp"

class BasicTrie {
private:

	class Internal;

	class Internal {
	public:
		Internal* left;
		Internal* right;
		Internal* parent;
		Table::route leaf;

		Internal(Internal* left, Internal* right, Internal* parent);
	};

	Internal* root;
	Table& table;

	void buildTrie();

public:
	BasicTrie(Table& table);

	const Table::route& route(uint32_t);
};

#endif /* BASICTRIE_HPP */
