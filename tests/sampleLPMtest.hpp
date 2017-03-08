#include "basicTrie.hpp"
#include "naive.hpp"

#include "sampleRoutingTable.hpp"

#include <vector>
#include <sstream>
#include <memory>
#include <cassert>

using namespace std;

template<typename LPM>
void testLPM(){
	TestTable tt;
	LPM lpm(tt);

	// This makes some assumptions which should be met...
	assert(lpm.route(0x10000040) == 0);
	assert(lpm.route(0x10000500) == 0);
	assert(lpm.route(0x20000670) == 1);
	assert(lpm.route(0x30000123) == 2);
	assert(lpm.route(0x11111111) == 0);
	assert(lpm.route(0xeeeeeeee) == 0);
	assert(lpm.route(0x0a000330) & RoutingTable::route::NH_DIRECTLY_CONNECTED);
	assert(lpm.route(0x0a000110) & RoutingTable::route::NH_DIRECTLY_CONNECTED);
	assert(lpm.route(0x0a000500) & RoutingTable::route::NH_DIRECTLY_CONNECTED);

};
