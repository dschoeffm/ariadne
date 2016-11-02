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
#include "table.hpp"

class FileTable : public Table {
private:
	std::vector<std::vector<Table::route>> entries;

public:
	FileTable(std::string filename);

	void aggregate();
	const std::vector<std::vector<route>>& getSortedRoutes();
};

#endif /* FILETABLE_HPP */
