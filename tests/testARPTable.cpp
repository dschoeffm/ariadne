#include "arpTable.hpp"
#include "interface.hpp"
#include "routingTable.hpp"

#include <vector>
#include <sstream>
#include <memory>
#include <cassert>

using namespace std;

class TestTable : public RoutingTable {
protected:
	void updateInfo(){};
public:
	TestTable() {
		entries->resize(33);
		RoutingTable::route new_route;
		new_route.prefix_length = 24;

		// The directly connected stuff
		for(unsigned int i=1; i<6; i++){
			new_route.base = 0x0a000000 + (i << 8);
			new_route.next_hop = 0;
			new_route.interface = i;
			//new_route.index = RoutingTable::route::NH_DIRECTLY_CONNECTED;
			entries->at(24).push_back(new_route);
		}

		// Some actual routes
		new_route.base = 0x10000000;
		new_route.next_hop = 0x0a000102;
		new_route.prefix_length = 16;
		new_route.interface = 1;
		//new_route.index = 1;
		entries->at(16).push_back(new_route);

		new_route.base = 0x20000000;
		new_route.next_hop = 0x0a000202;
		new_route.prefix_length = 16;
		new_route.interface = 2;
		//new_route.index = 2;
		entries->at(16).push_back(new_route);

		new_route.base = 0x30000000;
		new_route.next_hop = 0x0a000302;
		new_route.prefix_length = 16;
		new_route.interface = 3;
		//new_route.index = 3;
		entries->at(16).push_back(new_route);

		new_route.base = 0x00000000;
		new_route.next_hop = 0x0a000102;
		new_route.prefix_length = 0;
		new_route.interface = 1;
		//new_route.index = 1;
		entries->at(0).push_back(new_route);

		update();

		assert(this->getSortedRoutes()->at(24).size() == 5);
		assert(this->getSortedRoutes()->at(16).size() == 3);
		assert(this->getSortedRoutes()->at(0).size() == 1);

	};
};

int main(int argc, char** argv){
	(void) argc;
	(void) argv;

	auto interfaces = make_shared<vector<interface>>();

	for(uint8_t i=1; i<6; i++){
		interface iface;
		iface.mac = {{i,i,i,i,i,i}};
		iface.netlinkIndex = i;
		iface.netmapIndex = i;
		iface.IPs.push_back(0x0a000001 + (i << 8));
		stringstream name;
		name << "iface_" << i;
		iface.name = name.str();
		interfaces->push_back(iface);
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
		cout << "   Interface: " << a.interface << endl;
	}

	cout << endl << "Next Hop Mapping:" << endl;
	for(auto a : *tt->getNextHopMapping()){
		cout << "   IP: " << ip_to_str(a.nh_ip);
		cout << "  index: " << a.index;
		cout << "  interface: " << a.interface << endl;
	}

	assert(table->nextHops.size() == 3);
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
