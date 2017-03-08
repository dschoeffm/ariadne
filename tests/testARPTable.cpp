#include "arpTable.hpp"
#include "interface.hpp"
#include "routingTable.hpp"

#include "sampleRoutingTable.hpp"

#include <vector>
#include <sstream>
#include <memory>
#include <cassert>

using namespace std;

int main(int argc, char** argv){
	(void) argc;
	(void) argv;

	vector<shared_ptr<Interface>> interfaces;

	for(uint8_t i=1; i<6; i++){
		shared_ptr<Interface> iface = make_shared<Interface>();
		iface->mac = {{i,i,i,i,i,i}};
		iface->netlinkIndex = i;
		iface->netmapIndex = i;
		iface->IPs.push_back(0x0a000001 + (i << 8));
		stringstream name;
		name << "iface_" << i;
		iface->name = name.str();
		interfaces.push_back(iface);
	}

	ARPTable arpTable(interfaces);

	shared_ptr<ARPTable::table> table;

	// This table should be empty
	table = arpTable.getCurrentTable();
	assert(table->nextHops.size() == 0);
	assert(table->directlyConnected.size() == 0);

	cout << "Uninitialized table behaves correct" << endl;

	// Use the test table from above
	auto tt = make_shared<TestTable>();
	arpTable.createCurrentTable(tt);
	tt->print_table();

	table = arpTable.getCurrentTable();

	cout << "Next Hop table:" << endl;
	for(auto a : table->nextHops){
		cout << " Next Hop:" << endl;
		cout << "   Mac: " << mac_to_str(a.mac) << endl;
		cout << "   Interface: " << a.netmapInterface << endl;
	}

	cout << endl << "Next Hop Mapping:" << endl;
	for(auto a : *tt->getNextHopMapping()){
		cout << "   IP: " << ip_to_str(a.nh_ip);
		cout << "  index: " << a.index;
		cout << "  interface: " << a.interface << endl;
	}

	assert(table->nextHops.size() == 4);
	assert(table->directlyConnected.size() == 0);

	// Check if the nh_index numbers 1 & 2 are given
	bool found_nh_1 = false;
	bool found_nh_2 = false;

	for(auto a : *tt->getSortedRoutes()){
		for(auto &r : a){
			if(r.index == 1){
				found_nh_1 = true;
			} else if(r.index == 2){
				found_nh_2 = true;
			}
		}
	}

	assert(found_nh_1);
	assert(found_nh_2);

	return 0;
}
