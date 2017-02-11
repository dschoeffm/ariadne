#include "fileTable.hpp"

using namespace std;

FileTable::FileTable(string filename) {
	entries->resize(33);

	ifstream dump;
	dump.open(filename);
	if(!dump.is_open() || !dump.good()){
		fatal("Opening file for routing table failed");
	}

	regex regex("^(\\d+\\.\\d+\\.\\d+\\.\\d+)/(\\d+)\\s+"
			"(\\d+\\.\\d+\\.\\d+\\.\\d+)\\s+");
	while(1) {
		bool finished = false;
		string line;
		if(dump.eof()){
			finished = true;
		} else {
			getline(dump, line);
		}
		if(dump.eof()){
			finished = true;
		}

		if(finished){
			break;
		}

		smatch m;
		uint32_t addr, next_hop;
		int len;
		struct in_addr in_addr;

		regex_match(line, m, regex);

		inet_aton(m[1].str().c_str(), &in_addr);
		addr = ntohl(in_addr.s_addr);
		len = atoi(m[2].str().c_str());
		inet_aton(m[3].str().c_str(), &in_addr);
		next_hop = ntohl(in_addr.s_addr);

		RoutingTable::route route;
		route.next_hop = next_hop;
		route.base = addr;
		route.prefix_length = len;

		(*entries)[len].push_back(route);
	}
};

