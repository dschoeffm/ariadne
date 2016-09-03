#include "basicTrie.hpp"

using namespace std;

BasicTrie::Internal::Internal(Internal* left, Internal* right, Internal* parent, Leaf* leaf) :
	left(left), right(right), parent(parent), leaf(leaf) {};

BasicTrie::Leaf::Leaf(uint32_t prefix_length, uint32_t base, uint32_t next_hop) :
	prefix_length(prefix_length), base(base), next_hop(next_hop) {
		mask = ~((((uint64_t) 1) << (32-prefix_length)) -1);
	};

void BasicTrie::buildTrie() {
	// Allocate root node
	root = new Internal(NULL, NULL, NULL, NULL);

	// Build trie
	vector<map<uint32_t,uint32_t>> tbl = table.get_sorted_entries();
	for(int len=0; len<=32; len++){
		for(auto& e : tbl[len]){
			// The Internal node is always at level len
			Internal* cur = root;
			for(int level=0; level<len; level++){
				// If the next level does not exist, create the internal node
				uint32_t bit = extractBit(e.first, level);
				Internal** next = (bit) ? &cur->right : &cur->left;
				if(!*next){
					*next = new Internal(NULL, NULL, cur, NULL);
				}
				cur = *next;
			}

			// cur is at the position of the Leaf
			cur->leaf = new Leaf(len, e.first, e.second);
		}
	}

};

BasicTrie::BasicTrie(Table& table) : table(table) {
	buildTrie();
};

uint32_t BasicTrie::route(uint32_t addr){
	// Bootstrap first iteration
	Internal* cur = root;
	int pos = 0;
	uint32_t bit = extractBit(addr, pos);
	Internal* next = (bit) ? cur->right : cur->left;

	// Traverse downwards
	while(next){
		cur = next;
		bit = extractBit(addr, ++pos);
		next = (bit) ? cur->right : cur->left;
	}

	// Traverse upwards, until a matching prefix is found
	while(cur){
		if(cur->leaf && ((cur->leaf->mask & addr) == cur->leaf->base)){
			return cur->leaf->next_hop;
		}
		cur = cur->parent;
	}

	return 0xffffffff;
};

void BasicTrie::routeBatch(uint32_t* in, uint32_t* out, int count){
	// Mask - which addresses are finished
	uint64_t cur_mask = 0;
	uint64_t finished_mask = (((uint64_t) 1) << count) -1;

	// Mask - which traversals go upwards again
	uint64_t dir_mask = 0;

	// Bootstrap first iteration
	Internal* cur[64];
	int pos[64] = {0};
	uint32_t bit[64] = {0};
	Internal* next[64];

	for(int i=0; i<64; i++){
		cur[i] = root;
	}

	for(int i=0; i<count; i++){
		bit[i] = extractBit(in[i], pos[i]);
		next[i] = (bit[i]) ? cur[i]->right : cur[i]->left;
	}

	// Set all results to not-found
	for(int i=0; i<count; i++){
		out[i] = 0xffffffff;
	}

	// Until all addresses are flagged finished
	while(cur_mask != finished_mask){
		// Iterate over the complete batch
		for(int i=0; i<count; i++){
			// Filter/Mask for not finished addresses
			if(!(cur_mask & (((uint64_t) 1) << i))){
				// Decide if traversal is downwards or upwards
				if(!(dir_mask & (((uint64_t) 1) << i))){
					// Traverse downwards
					cur[i] = next[i];
					bit[i] = extractBit(in[i], ++pos[i]);
					next[i] = (bit[i]) ? cur[i]->right : cur[i]->left;
					if(!next[i]){
						dir_mask |= ((uint64_t) 1) << i;
					}
					__builtin_prefetch(next[i]);
				} else {
					// Traverse upwards, until a matching prefix is found
					if(cur[i]->leaf &&
							((cur[i]->leaf->mask & in[i])
							 == cur[i]->leaf->base)){
						out[i] = cur[i]->leaf->next_hop;
						cur_mask |= ((uint64_t) 1) << i;
					}
					cur[i] = cur[i]->parent;
					__builtin_prefetch(cur[i]);
				}
			}
		}
	}
}

