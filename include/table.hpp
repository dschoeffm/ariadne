#ifndef TABLE_HPP
#define TABLE_HPP

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

class Table {
public:
	struct entry {
		uint32_t addr;
		uint32_t next_hop;
	};
private:
	std::vector<std::map<uint32_t, uint32_t>> entries;

public:
	Table(std::string filename);

	void aggregate();
	void print_table();
	const std::vector<std::map<uint32_t, uint32_t>>& get_sorted_entries();
};

#endif /* TABLE_HPP */
