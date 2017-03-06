#include <cassert>
#include "linuxTable.hpp"
#include "netlink.hpp"

int main(int argc, char** argv){
	(void) argc;
	(void) argv;

	LinuxTable table(Netlink::getAllInterfaces());

	auto sortedRoutes = table.getSortedRoutes();
	assert(sortedRoutes->size() == 33);

	bool someRoutePresent = false;
	for(auto v : *sortedRoutes){
		if(v.size() != 0){
			someRoutePresent = true;
		}
	}
	assert(someRoutePresent);

	return 0;
};
