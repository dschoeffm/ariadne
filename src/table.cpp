#include "table.hpp"

using namespace std;

const Table::route Table::invalidRoute;

void Table::print_table(){
	auto entries = this->getSortedRoutes();
	for(int len=32; len>=0; len--){
		for(auto& a : entries[len]){
			cout << ip_to_str(a.base) << "/" << len
				<< " via " << ip_to_str(a.next_hop)
				<< " iface " << a.interface << endl;
		}
	}
};

