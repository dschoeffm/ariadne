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
		uint16_t leaf;

		Internal(Internal* left, Internal* right, Internal* parent);
	};

	Internal* root;
	RoutingTable& table;

	void buildTrie();

public:
	BasicTrie(RoutingTable& table);

	uint16_t route(uint32_t) const;
};

#endif /* BASICTRIE_HPP */
