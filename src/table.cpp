#include "table.hpp"

Table::Table(string filename) {
	entries.resize(33);

	ifstream dump;
	dump.open(filename);
	if(!dump.is_open() || !dump.good()){
		cout << "Opening file failed" << endl;
		exit(0);
	}


/*	
	string line;
	regex regex("^(\\d+\\.\\d+\\.\\d+\\.\\d+)/(\\d+) (\\d+\\.\\d+\\.\\d+\\.\\d+)$");
	while(getline(dump, line)) {
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

		entries[len].insert({addr, next_hop});
	}
*/

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

		entries[len].insert({addr, next_hop});
	}
};

void Table::aggregate() {
	int counter = 0;
	for(int len=30; len > 0; len--){
		cerr << "aggregating length " << len << endl;
		list<pair<uint32_t,uint32_t>> aggregated;
		for(map<uint32_t, uint32_t>::iterator it=entries[len].begin();
				it != entries[len].end(); it++){

			auto first = *it;
			auto second = *(next(it));

			if(((first.first ^ second.first) == (1 << (32-len)))
				&& (first.second == second.second)){
				aggregated.push_back(
					{first.first & (~(1 << (32-len))), second.second});
				counter++;
			}
		}
		for(auto& a : aggregated){
			entries[len].erase(a.first);
			entries[len].erase(a.first | (1 << (32-len)));
			entries[len-1].insert(a);
		}
	}

	cerr << "aggregated networks: " << counter << endl;
};

void Table::print_table(){
	for(int len=32; len>=0; len--){
		for(auto& a : entries[len]){
			cout << ip_to_str(a.first) << "/" << len;
			cout <<	" " << ip_to_str(a.second) << endl;
		}
	}
};

const vector<map<uint32_t, uint32_t>>& Table::get_sorted_entries() {
	return entries;
};
