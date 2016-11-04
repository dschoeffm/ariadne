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
