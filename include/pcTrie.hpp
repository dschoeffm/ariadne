#ifndef PCTRIE_HPP
#define PCTRIE_HPP

#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>

#include "table.hpp"

class PCTrie {
private:

	class Internal;
	class Leaf;
	class Node;

	enum node_type {
		INTERNAL,
		LEAF
	};

	class Node {
	public:
		Internal* parent;
		enum node_type type;
		uint32_t base;

		Node(
			Internal* parent,
			enum node_type type,
			uint32_t base);
	};

	class Internal : public Node {
	public:
		Node* left;
		Node* right;
		Leaf* leaf;
		uint8_t splitPos;

		Internal(
			Internal* parent,
			uint32_t base,
			Internal* left,
			Internal* right,
			Leaf* leaf,
			int splitPos);
	};

	class Leaf : public Node {
	public:
		struct leaf_entry {
			uint32_t next_hop;
			uint32_t prefix_length;

			bool operator < (const leaf_entry& e) const {
				return prefix_length > e.prefix_length;
			}
		};

		std::vector<struct leaf_entry> entries;

		Leaf(Internal* parent, uint32_t base);
		void pushRoute(uint32_t next_hop, uint32_t prefix_length);
		bool hasMoreGeneralRoute(uint32_t prefix_length);
	};

	Node* root;

	Table& table;

	void buildTrie();

	bool qtree;
	std::stringstream qtree_prev;
	void addQtreeSnapshot();
	std::string getQtreeSnapshot();
	std::string finalizeQtree(std::string tree);

public:
	PCTrie(Table& table, bool qtree = false);

	std::string getQtree();
	std::string getQtreeHistory();

	unsigned int getSize();

	uint32_t route(uint32_t);
};

#endif /* PCTRIE_HPP */
