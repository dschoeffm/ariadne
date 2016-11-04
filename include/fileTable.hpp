#ifndef FILE_TABLE_HPP
#define FILE_TABLE_HPP

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
#include "routingTable.hpp"

class FileTable : public RoutingTable {
public:
	FileTable(std::string filename);

	void aggregate();
};

#endif /* FILETABLE_HPP */
