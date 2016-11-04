#include "routingTable.hpp"

using namespace std;

const RoutingTable::route RoutingTable::invalidRoute;

void RoutingTable::print_table(){
	auto entries = this->getSortedRoutes();
	for(int len=32; len>=0; len--){
		for(auto& a : (*entries)[len]){
			cout << ip_to_str(a.base) << "/" << len
				<< " via " << ip_to_str(a.next_hop)
				<< " iface " << a.interface << endl;
		}
	}
};

shared_ptr<vector<vector<RoutingTable::route>>> RoutingTable::getSortedRoutes() {
	return entries;
};

void RoutingTable::aggregate() {
	int counter = 0;
	for(int len=30; len > 0; len--){
		cerr << "aggregating length " << len << endl;

		if((*entries)[len].size() < 2){
			continue;
		}

		for(unsigned int i=0; i < (*entries)[len].size()-1; i++){

			route& first = (*entries)[len][i];
			route& second = (*entries)[len][i+1];

			if(((first.base ^ second.base) == ((uint32_t) 1 << (32-len)))
				&& (first.next_hop == second.next_hop)){
				(*entries)[len].erase((*entries)[len].begin()+i, (*entries)[len].begin()+i+1);
				route newRoute;
				newRoute.base = first.base & (~(1 << (32-len)));
				newRoute.next_hop = first.next_hop;
				newRoute.prefix_length = len;
				newRoute.interface = std::numeric_limits<uint32_t>::max();
				counter++;
			}
		}
	}

	cerr << "aggregated networks: " << counter << endl;
};

