#include "naive.hpp"

Naive::Naive(Table& table) : entries(table.getSortedRoutes()) {
	uint32_t mask = 0;
	for(int i=0; i<33; i++){
		masks[i] = mask;
		mask = mask >> 1;
		mask |= 1 << 31;
	}
};

const Table::route& Naive::route(uint32_t addr) {
	for(int len=32; len>=0; len--){
		for(auto& route : entries[len]){
			if(route.base == (addr & masks[len])){
				return route;
			}
		}
	};
	return Table::invalidRoute;
};

