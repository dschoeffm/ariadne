#include "naive.hpp"

Naive::Naive(Table& table) : entries(table.get_sorted_entries()) {
	uint32_t mask = 0;
	for(int i=0; i<33; i++){
		masks[i] = mask;
		mask = mask >> 1;
		mask |= 1 << 31;
	}
};

uint32_t Naive::route(uint32_t addr) {
	for(int len=32; len>=0; len--){
		for(auto& e : entries[len]){
			if(e.first == (addr & masks[len])){
				return e.second;
			}
		}
	};
	return 0xffffffff;
};

