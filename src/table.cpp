#include "table.hpp"

Table::Table(string filename) {
	entries.resize(33);
	vector<map<uint32_t, uint32_t>> map;
	map.resize(33);

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

		map[len].insert({addr, next_hop});
	}

	for(int i=0; i<33; i++){
		for(auto& a : map[i]){
			entries[i].push_back({a.first, a.second});
		}
	}

};

/*
void Table::aggregate() {
	for(int len=30; len > 0; len--){
		cout << "aggregating length " << len << endl;
		list<pair<uint32_t,uint32_t>> aggregated;
		for(auto& a1 : entries[len]){
			for(auto& a2 : entries[len]){
				if(((a1.first ^ a2.first) == (1 << (32-len)))
					&& (a1.second == a2.second)){
					aggregated.push_back(
						{a1.first & (~(1 << (32-len))), a1.second});
				}
			}
		}
		for(auto& a : aggregated){
			entries[len].erase(a.first);
			entries[len].erase(a.first | (1 << (32-len)));
			entries[len-1].insert(a);
		}
	}
};*/

void Table::aggregate() {
	int counter = 0;
	for(int len=30; len > 0; len--){
		cerr << "aggregating length " << len << endl;
		list<pair<uint32_t,uint32_t>> aggregated;
		for(list<pair<uint32_t, uint32_t>>::iterator it=entries[len].begin();
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
			auto pos_a = entries[len].begin();
			auto pos_b = entries[len].begin();
			int status = -1;
			for(;;pos_a++, pos_b++){
				if(a.first == pos_a->first){
					status = 0;
					break;
				}
			}
			if(status == -1){
				cerr << "something went wrong during aggregation" << endl;
			}
			pos_b++;

			entries[len].erase(pos_a);
			entries[len].erase(pos_b);
			entries[len-1].push_back(a);
		}
		entries[len-1].sort(
			[] (pair<uint32_t,uint32_t> a, pair<uint32_t,uint32_t> b){
				return (a.first < b.first);
			});
	}

	cerr << "aggregated networks: " << counter << endl;
};

/*
void Table::print_table(){
	cout << "Destination     \tNext Hop" << endl;
	cout << "-------------------------------------" << endl;
	for(int len=32; len>=0; len--){
		for(auto& a : entries[len]){
			cout << ip_to_str(a.first) << "/" << len;
			//cout << hex << "   " << a.first;
			cout <<	"  \t" << ip_to_str(a.second) << endl;
		}
	}
};
*/

void Table::print_table(){
	for(int len=32; len>=0; len--){
		for(auto& a : entries[len]){
			cout << ip_to_str(a.first) << "/" << len;
			cout <<	" " << ip_to_str(a.second) << endl;
		}
	}
};

const vector<list<pair<uint32_t, uint32_t>>>& Table::get_sorted_entries() {
	return entries;
};
