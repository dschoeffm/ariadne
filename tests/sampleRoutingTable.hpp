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


