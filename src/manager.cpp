#include "manager.hpp"

using namespace std;

std::shared_ptr<std::vector<interface>> fillNetLink(){
	shared_ptr<vector<interface>> interfaces = Netlink::getAllInterfaces();
	sort(interfaces->begin(), interfaces->end());
	uint32_t max_index = interfaces->back().index;
	if(max_index != interfaces->size()){
		for(uint32_t index=0; index<max_index; index++){
			auto it = find_if(interfaces->begin(), interfaces->end(),
					[index](interface& i){
						return i == index;
					});

			if (it == interfaces->end()){
				interfaces->emplace(interfaces->end(), index);
			}
		}
	}
	sort(interfaces->begin(), interfaces->end());
	return interfaces;
};

void Manager::startWorkerThreads(){
	for(unsigned i=0; thread::hardware_concurrency()-1; i++){
		worker.push_back(new Worker(cur_lpm, arpTable.getCurrentTable(),
			in_rings[i], out_rings[i], interfaces));
	}
};

