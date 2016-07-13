#include <iostream>
#include <fstream>
#include <list>

#include <time.h>

#include "util.hpp"
#include "table.hpp"
#include "naive.hpp"
#include "dxr.hpp"

#define DUMP_FIB 0
#define RANDOM 0
#define CHALLENGE 1

int main(int argc, char** argv){
#if CHALLENGE == 1
	if(argc < 3){
		cout << "Usage: " << argv[0] << " <fib_file> <challenge_file>" << endl;
		return 0;
	}
#else
	if(argc < 2){
		cout << "Usage: " << argv[0] << " <rib file>" << endl;
		return 0;
	}
#endif

	string filename(argv[1]);
	Table table(filename);

#if DUMP_FIB == 1
	table.aggregate();
	table.print_table();
#endif

#if RANDOM == 1
	Naive naive(table);

	clock_t start = clock();
	for(int i=0; i<1000; i++){
		uint32_t addr = random();
		uint32_t res = naive.route(addr);
		cout << ip_to_str(addr) << " " << ip_to_str(res) << endl;
	}
	clock_t end = clock();

	cerr << "Lookups took " << (1.0*(end-start)) / CLOCKS_PER_SEC << " seconds" << endl;
#endif

#if CHALLENGE == 1
	list<pair<uint32_t, uint32_t>> challenge;
	ifstream dump(argv[2]);
	regex regex("^(\\d+\\.\\d+\\.\\d+\\.\\d+)\\s+(\\d+\\.\\d+\\.\\d+\\.\\d+)$");
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
		struct in_addr in_addr;

		regex_match(line, m, regex);

		inet_aton(m[1].str().c_str(), &in_addr);
		addr = ntohl(in_addr.s_addr);
		inet_aton(m[2].str().c_str(), &in_addr);
		next_hop = ntohl(in_addr.s_addr);

		challenge.push_back({addr, next_hop});
	}

	int failed = 0;
	int success = 0;
	DXR lpm(table);
	//lpm.print_tables();
	//lpm.print_expansion();
	clock_t start = clock();
	for(auto& a : challenge){
		uint32_t res = lpm.route(a.first);
		if(res != a.second){
			cout << "Failed IP: " << ip_to_str(a.first) << endl;
			cout << "Expected: " << ip_to_str(a.second) << endl;
			cout << "Got     : " << ip_to_str(res) << endl << endl;
			failed++;
		} else {
			success++;
		}
	}
	clock_t end = clock();

	cerr << "Lookups took " << (1.0*(end-start)) / CLOCKS_PER_SEC << " seconds" << endl;

	cout << "Successful lookups: " << success << endl;
	cout << "Failed lookups: " << failed << endl;
#endif

	return 0;
}
