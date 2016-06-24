#ifndef __TABLE_HPP__
#define __TABLE_HPP__

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>

#include <fstream>
#include <regex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "util.hpp"

using namespace std;

class Table {
public:
	struct entry {
		uint32_t addr;
		uint32_t next_hop;
	};
private:
	vector<list<pair<uint32_t, uint32_t>>> entries;

public:
	Table(string filename);

	void aggregate();
	void print_table();
	const vector<list<pair<uint32_t, uint32_t>>>& get_sorted_entries();
};

#endif /*__TABLE_HPP__*/
