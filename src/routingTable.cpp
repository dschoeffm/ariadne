#include "routingTable.hpp"

using namespace std;

const RoutingTable::route RoutingTable::invalidRoute;

void RoutingTable::print_table(){
	stringstream stream;
	stream << "Routing Table content:" << endl;
	auto entries = this->getSortedRoutes();
	for(int len=32; len>=0; len--){
		for(auto& a : (*entries)[len]){
			stream << ip_to_str(a.base) << "/" << len
				<< " via " << ip_to_str(a.next_hop)
				<< " iface " << a.interface
			    << " nh_index " << a.index << endl;
		}
	}
	logInfo(stream.str());
};

shared_ptr<vector<vector<RoutingTable::route>>> RoutingTable::getSortedRoutes() {
	return entries;
};

void RoutingTable::aggregate() {
	for(int len=30; len > 0; len--){

		if((*entries)[len].size() < 2){
			continue;
		}

		for(unsigned int i=0; i < (*entries)[len].size()-1; i++){

			route& first = (*entries)[len][i];
			route& second = (*entries)[len][i+1];

			if(((first.base ^ second.base) == ((uint32_t) 1 << (32-len)))
				&& (first.next_hop == second.next_hop)
				&& (first.interface == second.interface)){
				(*entries)[len].erase((*entries)[len].begin()+i, (*entries)[len].begin()+i+1);
				route newRoute;
				newRoute.base = first.base & (~(1 << (32-len)));
				newRoute.next_hop = first.next_hop;
				newRoute.prefix_length = len;
				newRoute.interface = uint16_t_max;
			}
		}
	}

};

void RoutingTable::buildNextHopList(){
	vector<uint32_t> new_nextHopList;
	vector<ARPTable::nextHop> nextHopList;
	for(auto a : *entries){
		for(auto& r : a){

			if(r.next_hop == 0){
				r.index = route::NH_DIRECTLY_CONNECTED;
				continue;
			}

			auto it = find(nextHopMapping->begin(), nextHopMapping->end(), r.next_hop);

			/*
			// For the directly connected case - one IPv4 -> multiple interfaces
			while(r.interface != nextHopList[*it].interface){
				find(it, nextHopMapping->end(), r.next_hop);
			}
			*/

			if(it == nextHopMapping->end()){
				// Next hop is not yet in the list
				r.index = nextHopMapping->size();
				ARPTable::nextHop nh;
				nh.interface = r.interface;

				nextHopList.push_back(nh);
				nextHopMapping->push_back(r.next_hop);

				interfaces.insert(r.interface);
			} else {
				r.index = distance(nextHopMapping->begin(), it);
			}
		}
	}
}
