#include "manager.hpp"


void Manager::startWorkerThreads(){
	for(unsigned i=0; thread::hardware_concurrency()-1; i++){
		worker.push_back(new Worker(cur_lpm, arpTable.getCurrentTable(),
			in_rings[i], out_rings[i], interfaces));
	}
};

