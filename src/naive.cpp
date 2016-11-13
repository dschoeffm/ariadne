#include "naive.hpp"

Naive::Naive(RoutingTable& table) : entries(table.getSortedRoutes()) {
	uint32_t mask = 0;
	for(int i=0; i<33; i++){
		masks[i] = mask;
		mask = mask >> 1;
		mask |= 1 << 31;
	}
};

uint16_t Naive::route(uint32_t addr) const {
	for(int len=32; len>=0; len--){
		for(auto& route : (*entries)[len]){
			if(route.base == (addr & masks[len])){
				return route.next_hop;
			}
		}
	};
	return uint16_t_max;
};
