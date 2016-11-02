#include "fileTable.hpp"

using namespace std;

FileTable::FileTable(string filename) {
	entries.resize(33);

	ifstream dump;
	dump.open(filename);
	if(!dump.is_open() || !dump.good()){
		cout << "Opening file failed" << endl;
		exit(0);
	}

	regex regex("^(\\d+\\.\\d+\\.\\d+\\.\\d+)/(\\d+)\\s+(\\d+\\.\\d+\\.\\d+\\.\\d+)$");
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

		Table::route route;
		route.next_hop = next_hop;
		route.base = addr;
		route.prefix_length = len;
		route.interface = std::numeric_limits<uint32_t>::max();

		entries[len].push_back(route);
	}
};

void FileTable::aggregate() {
	int counter = 0;
	for(int len=30; len > 0; len--){
		cerr << "aggregating length " << len << endl;
		for(unsigned int i=0; i < entries[len].size()-1; i++){

			route& first = entries[len][i];
			route& second = entries[len][i+1];

			if(((first.base ^ second.base) == ((uint32_t) 1 << (32-len)))
				&& (first.next_hop == second.next_hop)){
				entries[len].erase(entries[len].begin()+i, entries[len].begin()+i+1);
				route newRoute;
				newRoute.base = first.base & (~(1 << (32-len)));
				newRoute.next_hop = first.next_hop;
				newRoute.prefix_length = len;
				newRoute.interface = std::numeric_limits<uint32_t>::max();
				counter++;
			}
		}
	}

	cerr << "aggregated networks: " << counter << endl;
};

const vector<vector<Table::route>>& FileTable::getSortedRoutes() {
	return entries;
};
