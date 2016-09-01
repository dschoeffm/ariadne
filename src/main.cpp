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
#include "table.hpp"
#include "naive.hpp"
//#include "dxr.hpp"
#include "basicTrie.hpp"

#define CHALLENGE_VERSION 1

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
	cout << "\t --dump-fib <rib-file>" << endl;
	cout << "\t --dump-challenge <fib-file> <challenge-file>" << endl;
	cout << "\t --run-challenge <fib-file> <challenge-file>" << endl;
};

void dump_challenge(Table& table, string filename){
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
		entry.next_hop = lpm.route(entry.addr);
		challenge_file.write((char*) &entry, sizeof(challenge_entry));
	}

	challenge_file.close();
};

void run_challenge(Table& table, string challenge_filename){

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
			MAP_PRIVATE,
			fd,
			0);

	if(mmap_base == MAP_FAILED){
		cerr << "mmap failed! errno: " << errno << endl;
		return;
	}

	challenge_entry* entries = (challenge_entry*) (mmap_base + sizeof(challenge_header));

	int failed = 0;
	int success = 0;
	//DXR lpm(table);
	//Naive lpm(table);
	BasicTrie lpm(table);
	//lpm.print_expansion();
	//lpm.print_tables();
	clock_t start = clock();

#if 0
	// No batching
	for(unsigned int i=0; i<header.num_entries; i++){
		uint32_t res = lpm.route(entries[i].addr);
		if(res != entries[i].next_hop){
			cout << "Failed IP: " << ip_to_str(entries[i].addr) << endl;
			cout << "Expected : " << ip_to_str(entries[i].next_hop) << endl;
			cout << "Got      : " << ip_to_str(res) << endl << endl;
			failed++;
		} else {
			success++;
		}
	}
#else
	// Batching
#define BATCH_SIZE 32
	uint32_t in[BATCH_SIZE] = {0};
	uint32_t out[BATCH_SIZE] = {0};

	int num_batches = header.num_entries / BATCH_SIZE;

	for(int i=0; i<num_batches; i++){
		// Copy input
		for(int j=0; j<BATCH_SIZE; j++){
			in[j] = entries[i*BATCH_SIZE + j].addr;
		}

		// Do lookup
		lpm.routeBatch(in, out, BATCH_SIZE);

		// Check the result
		for(int j=0; j<BATCH_SIZE; j++){
			if(out[j] == entries[i*BATCH_SIZE + j].next_hop){
				success++;
			} else {
				//cout << "Failed IP: " << ip_to_str(in[j]) << endl;
				cout << "Failed IP: " << ip_to_str(entries[i*num_batches + j].addr) << endl;
				cout << "Expected : " << ip_to_str(entries[i*num_batches + j].next_hop) << endl;
				cout << "Got      : " << ip_to_str(out[j]) << endl << endl;
				failed++;
			}
		}
	}

#endif

	clock_t end = clock();

	float seconds = (1.0*(end-start)) / CLOCKS_PER_SEC;

	cerr << "Lookups took " << seconds << " seconds" << endl;
	cerr << "Rate: " << ((success + failed) / seconds) / 1000000 << " Mlps" << endl;

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
		challenge_filename = string(argv[3]);
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
			dump_challenge(table, challenge_filename);
		break;
		case RUN_CHALLENGE:
			run_challenge(table, challenge_filename);
		break;
	}

	return 0;
}
