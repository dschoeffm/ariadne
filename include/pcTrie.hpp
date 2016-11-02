#ifndef PCTRIE_HPP
#define PCTRIE_HPP

#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <functional>

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
		struct leafEntry : public Table::route {
			bool operator < (const leafEntry& e) const {
				return prefix_length > e.prefix_length;
			};

			leafEntry() : route() {};
			leafEntry(const Table::route& route) : Table::route(route) {};
		};

		std::vector<leafEntry> entries;

		Leaf(Internal* parent, uint32_t base);
		void pushRoute(const Table::route& route);
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

	const Table::route& route(uint32_t);
};

#endif /* PCTRIE_HPP */
