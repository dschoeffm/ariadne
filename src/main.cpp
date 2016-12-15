#include <iostream>
#include <fstream>
#include <list>

#include <time.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

#include "util.hpp"
#include "routingTable.hpp"
#include "fileTable.hpp"
#include "linuxTable.hpp"
#include "naive.hpp"
#include "basicTrie.hpp"
#include "pcTrie.hpp"
#include "manager.hpp"

using namespace std;

void print_usage(string name) {
	cout  << "Usage: " << name << "list-of-interfaces" << endl;
};

int main(int argc, char** argv){
	if(argc < 2){
		print_usage(argv[0]);
		return 0;
	}

	vector<string> interfaces;
	for(int i=1; i<argc; i++){
		interfaces.push_back(argv[i]);
	}

	Manager manager(interfaces);
	manager.run();
};

// old stuff, only here, because to good to delete
#if 0
#define CHALLENGE_VERSION 1

using namespace std;

struct challenge_header {
	uint32_t version;
	uint32_t num_entries;
	uint32_t reserved_1;
	uint32_t reserved_2;
};

struct challenge_entry {
	uint32_t addr;
	uint32_t next_hop;
};

void print_usage(string name){
	cout << "Usage: " << name << endl;
	cout << "\t --algo <name>\t\t\t\t valid: Naive, BasicTrie (default: BasicTrie)" << endl;
	cout << "\t --dump-fib\t\t\t\t pass rib filename to --fib-file" << endl;
	//cout << "\t --dump-challenge <challenge>" << endl;
	//cout << "\t --run-challenge <challenge>" << endl;
	//cout << "\t --convert-challenge <old> <new>" << endl;
	cout << "\t --fib-file <fib>\t\t\t default: kernel routing table" << endl;
};

#if 0
void dump_challenge(RoutingTable& table, string filename){
#define NUM_ENTRIES 10000000

	ofstream challenge_file (filename, ios::out | ios::binary);
	struct challenge_header header;
	header.version = CHALLENGE_VERSION;
	header.num_entries = NUM_ENTRIES;
	header.reserved_1 = 0;
	header.reserved_2 = 0;
	challenge_file.write((char*) &header, sizeof(challenge_header));

	BasicTrie lpm(table);

	for(int i=0; i<NUM_ENTRIES; i++){
		challenge_entry entry;
		entry.addr = random();
		entry.next_hop = (lpm.route(entry.addr)).next_hop;
		challenge_file.write((char*) &entry, sizeof(challenge_entry));
	}

	challenge_file.close();
};

template <typename LPM>
void run_challenge(RoutingTable& table, string challenge_filename){
	// read the challenge file
	int fd = open(challenge_filename.c_str(), 0);
	challenge_header header;
	int ret = read(fd, &header, sizeof(challenge_header));
	if(ret != sizeof(challenge_header)){
		cerr << "Error while reader challenge_header!" << endl;
		return;
	}
	if(header.version != CHALLENGE_VERSION){
		cerr << "challenge version is not supported!" << endl;
		return;
	}

	char* mmap_base =  (char*) mmap(
			NULL,
			header.num_entries * sizeof(challenge_entry) + sizeof(challenge_header),
			PROT_READ,
			MAP_PRIVATE | MAP_POPULATE,
			fd,
			0);

	if(mmap_base == MAP_FAILED){
		cerr << "mmap failed! errno: " << errno << endl;
		return;
	}

	challenge_entry* entries = (challenge_entry*) (mmap_base + sizeof(challenge_header));

	int failed = 0;
	int success = 0;

	LPM lpm(table);

	clock_t start = clock();
	for(unsigned int i=0; i<header.num_entries; i++){
		uint32_t res = (lpm.route(entries[i].addr)).next_hop;
		if(unlikely(res != entries[i].next_hop)){
			cout << "Failed IP: " << ip_to_str(entries[i].addr) << endl;
			cout << "Expected : " << ip_to_str(entries[i].next_hop) << endl;
			cout << "Got      : " << ip_to_str(res) << endl << endl;
			failed++;
		} else {
			success++;
		}
	}
	clock_t end = clock();

	float seconds = (1.0*(end-start)) / CLOCKS_PER_SEC;

	cerr << "Lookups took " << seconds << " seconds" << endl;
	cerr << "Rate: " << ((success + failed) / seconds) / 1000000 << " Mlps" << endl;

	cout << "Successful lookups: " << success << endl;
	cout << "Failed lookups: " << failed << endl;
};

void convert_challenge(string old_file, string new_file){
	// read the challenge file
	list<pair<uint32_t, uint32_t>> challenge;
	ifstream dump(old_file);
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

	ofstream challenge_file (new_file, ios::out | ios::binary);
	struct challenge_header header;
	header.version = CHALLENGE_VERSION;
	header.num_entries = challenge.size();
	header.reserved_1 = 0;
	header.reserved_2 = 0;
	challenge_file.write((char*) &header, sizeof(challenge_header));

	for(auto& e : challenge){
		challenge_entry entry;
		entry.addr = e.first;
		entry.next_hop = e.second;
		challenge_file.write((char*) &entry, sizeof(challenge_entry));
	}

	challenge_file.close();

}
#endif

int main(int argc, char** argv){
	enum {INVALID_MODE, DUMP_FIB, DUMP_CHALLENGE, RUN_CHALLENGE, CONVERT_CHALLENGE} mode
		= INVALID_MODE;
	enum {INVALID_ALGO, NAIVE, BASICTRIE} algo = INVALID_ALGO;
	enum {FILETABLE, KERNEL} table_mode = KERNEL;

	if(argc < 2){
		print_usage(string(argv[0]));
		return 0;
	}

	int cmd_pos = 1;
	string filename;
	string challenge_filename = "";

	while(cmd_pos < argc){
		if(strcmp(argv[cmd_pos], "--algo") == 0){
			if(strcmp(argv[cmd_pos+1], "Naive") == 0)
				algo = NAIVE;
			else if (strcmp(argv[cmd_pos+1], "BasicTrie") == 0)
				algo = BASICTRIE;
#if 0
			else if (strcmp(argv[cmd_pos+1], "PCTrie") == 0)
				algo = PCTRIE;
#endif
			cmd_pos += 2;
		} else if(strcmp(argv[cmd_pos], "--dump-fib") == 0){
			mode = DUMP_FIB;
			cmd_pos += 1;
#if 0
		} else if(strcmp(argv[cmd_pos], "--dump-challenge") == 0){
			mode = DUMP_CHALLENGE;
			challenge_filename = string(argv[cmd_pos+1]);
			cmd_pos += 2;
		} else if(strcmp(argv[cmd_pos], "--run-challenge") == 0){
			mode = RUN_CHALLENGE;
			challenge_filename = string(argv[cmd_pos+1]);
			cmd_pos += 2;
		} else if(strcmp(argv[cmd_pos], "--convert-challenge") == 0){
			mode = CONVERT_CHALLENGE;
			filename = string(argv[cmd_pos+1]);
			challenge_filename = string(argv[cmd_pos+2]);
			cmd_pos += 3;
#endif
		} else if(strcmp(argv[cmd_pos], "--fib-file") == 0){
			table_mode = FILETABLE;
			filename = argv[cmd_pos+1];
			cmd_pos += 2;
		} else {
			print_usage(string(argv[0]));
			return 0;
		}
	}

	RoutingTable* table;
	switch(table_mode){
		case KERNEL:
			table = new LinuxTable();
		break;
		case FILETABLE:
			table = new FileTable(filename);
		break;
	}

	switch(mode){
		case DUMP_FIB:
			table->print_table();
		break;
#if 0
		case DUMP_CHALLENGE:
			dump_challenge(*table, challenge_filename);
		break;
		case RUN_CHALLENGE:
			switch(algo){
				case NAIVE:
					run_challenge<Naive>(*table, challenge_filename);
				break;
				case BASICTRIE:
					run_challenge<BasicTrie>(*table, challenge_filename);
				break;
#if 0
				case PCTRIE:
					run_challenge<PCTrie>(*table, challenge_filename);
				break;
#endif
				default:
					run_challenge<BasicTrie>(*table, challenge_filename);
				break;
			}
		break;
		case CONVERT_CHALLENGE:
			convert_challenge(filename, challenge_filename);
		break;
#endif
		default:
			print_usage(string(argv[0]));
		break;
	}

	return 0;
}
#endif
