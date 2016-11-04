#ifndef BASICTRIE_HPP
#define BASICTRIE_HPP

#include <limits>
#include <memory>

#include "routingTable.hpp"
#include "util.hpp"

class BasicTrie {
private:

	class Internal;

	class Internal {
	public:
		Internal* left;
		Internal* right;
		Internal* parent;
		RoutingTable::route leaf;

		Internal(Internal* left, Internal* right, Internal* parent);
	};

	Internal* root;
	RoutingTable& table;

	void buildTrie();

public:
	BasicTrie(RoutingTable& table);

	const RoutingTable::route& route(uint32_t);
};

#endif /* BASICTRIE_HPP */
