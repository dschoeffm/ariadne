#include <iostream>
#include <fstream>
#include <list>

#include <time.h>
#include <string.h>

#include "util.hpp"
#include "table.hpp"
#include "naive.hpp"
//#include "dxr.hpp"

void print_usage(string name){
	cout << "Usage: " << name << endl;
	cout << "\t --dump-fib <rib-file>" << endl;
	cout << "\t --dump-challenge <fib-file>" << endl;
	cout << "\t --run-challenge <fib-file> <challenge>" << endl;
};

void dump_challenge(Table& table){
	Naive naive(table);

	for(int i=0; i<100000; i++){
		uint32_t addr = random();
		uint32_t res = naive.route(addr);
		cout << ip_to_str(addr) << " " << ip_to_str(res) << endl;
	}
};

void run_challenge(Table& table, string challenge_filename){

	// read the challenge file
	list<pair<uint32_t, uint32_t>> challenge;
	ifstream dump(challenge_filename);
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
	//DXR lpm(table);
	Naive lpm(table);
	//lpm.print_expansion();
	//lpm.print_tables();
	clock_t start = clock();
	for(auto& a : challenge){
		uint32_t res = lpm.route(a.first);
		if(res != a.second){
			cout << "Failed IP: " << ip_to_str(a.first) << endl;
			cout << "Expected : " << ip_to_str(a.second) << endl;
			cout << "Got      : " << ip_to_str(res) << endl << endl;
			failed++;
		} else {
			success++;
		}
	}
	clock_t end = clock();

	cerr << "Lookups took " << (1.0*(end-start)) / CLOCKS_PER_SEC << " seconds" << endl;

	cout << "Successful lookups: " << success << endl;
	cout << "Failed lookups: " << failed << endl;

};

int main(int argc, char** argv){
	string challenge_filename = "";
	enum {DUMP_FIB, DUMP_CHALLENGE, RUN_CHALLENGE} mode;

	if(argc < 3){
		print_usage(string(argv[0]));
		return 0;
	}

	if(strcmp(argv[1], "--dump-fib") == 0){
		mode = DUMP_FIB;
	} else if(strcmp(argv[1], "--dump-challenge") == 0){
		mode = DUMP_CHALLENGE;
	} else if(strcmp(argv[1], "--run-challenge") == 0){
		mode = RUN_CHALLENGE;
		challenge_filename = string(argv[3]);
	} else {
		print_usage(string(argv[0]));
		return 0;
	}

	string filename(argv[2]);
	Table table(filename);

	switch(mode){
		case DUMP_FIB:
			table.aggregate();
			table.print_table();
		break;
		case DUMP_CHALLENGE:
			dump_challenge(table);
		break;
		case RUN_CHALLENGE:
			run_challenge(table, challenge_filename);
		break;
	}

	return 0;
}
