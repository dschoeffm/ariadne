#include "basicTrie.hpp"

using namespace std;

BasicTrie::Internal::Internal(
	Internal* left, Internal* right, Internal* parent) :
	left(left), right(right), parent(parent), leaf(RoutingTable::route::NH_INVALID) {};

void BasicTrie::buildTrie() {
	// Allocate root node
	root.reset(new Internal(NULL, NULL, NULL));

	// Build trie
	shared_ptr<vector<vector<RoutingTable::route>>> tbl = table.getSortedRoutes();
	for(int len=0; len<=32; len++){
		for(auto& route : (*tbl)[len]){
			// The Internal node is always at level len
			Internal* cur = root.get();
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
			cur->leaf = route.index;
		}
	}

};

BasicTrie::BasicTrie(RoutingTable& table) : table(table) {
	buildTrie();
};

nh_index BasicTrie::route(uint32_t addr) const {
	// Bootstrap first iteration
	Internal* cur = root.get();
	int pos = 0;
	uint32_t bit = extractBit(addr, pos);
	Internal* next = (bit) ? cur->right : cur->left;
	uint16_t lastLeaf = root->leaf;

	// Traverse downwards
	while(next){
		cur = next;
		if(cur->leaf != RoutingTable::route::NH_INVALID){
			lastLeaf = cur->leaf;
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

	return lastLeaf;
};
