#ifndef BASICTRIE_HPP
#define BASICTRIE_HPP

#include <limits>
#include <memory>

#include "routingTable.hpp"
#include "util.hpp"

/*! LPM implementation based on a Trie.
 * This BasicTrie class implements a very basic trie (max. depth 32), to be used for LPM
 */
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
	/*! Create new Basic Trie.
	 * Counstruct a new Trie based on the given routing table
	 * \param table The routing table to build the Trie for
	 */
	BasicTrie(RoutingTable& table);

	/*! Route one IPv4 address.
	 * \param addr IPv4 address
	 * \return next hop index
	 */
	nh_index route(uint32_t addr) const;
};

#endif /* BASICTRIE_HPP */
