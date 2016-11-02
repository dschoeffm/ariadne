#include "basicTrie.hpp"

using namespace std;

BasicTrie::Internal::Internal(
	Internal* left, Internal* right, Internal* parent) :
	left(left), right(right), parent(parent) {};

void BasicTrie::buildTrie() {
	// Allocate root node
	root = new Internal(NULL, NULL, NULL);

	// Build trie
	vector<vector<Table::route>> tbl = table.getSortedRoutes();
	for(int len=0; len<=32; len++){
		for(auto& route : tbl[len]){
			// The Internal node is always at level len
			Internal* cur = root;
			for(int level=0; level<len; level++){
				// If the next level does not exist, create the internal node
				uint32_t bit = extractBit(route.base, level);
				Internal** next = (bit) ? &cur->right : &cur->left;
				if(!*next){
					*next = new Internal(NULL, NULL, cur);
				}
				cur = *next;
			}

			// cur is at the position of the Leaf
			cur->leaf = route;
		}
	}

};

BasicTrie::BasicTrie(Table& table) : table(table) {
	buildTrie();
};

const Table::route& BasicTrie::route(uint32_t addr){
	// Bootstrap first iteration
	Internal* cur = root;
	int pos = 0;
	uint32_t bit = extractBit(addr, pos);
	Internal* next = (bit) ? cur->right : cur->left;
	Table::route* lastLeaf = &root->leaf;

	// Traverse downwards
	while(next){
		cur = next;
		if(cur->leaf){
			lastLeaf = &cur->leaf;
		}
		bit = extractBit(addr, ++pos);
		next = (bit) ? cur->right : cur->left;
	}

	/*
	// Traverse upwards, until a matching prefix is found
	while(cur){
		if(cur->leaf && ((cur->leaf->mask & addr) == cur->leaf->base)){
			return cur->leaf->next_hop;
		}
		cur = cur->parent;
	}
	*/

	if(lastLeaf && ((PREFIX_MASK(lastLeaf->prefix_length) & addr) == lastLeaf->base)){
			return *lastLeaf;
	}



	return Table::invalidRoute;
};
