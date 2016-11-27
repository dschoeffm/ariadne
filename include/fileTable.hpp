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

/*! File based routing table.
 * The routing table is read from a file given to the constructor
 */
class FileTable : public RoutingTable {
public:
	/*! Read routing table from a file.
	 * \param filename file to read in
	 */
	FileTable(std::string filename);
};

#endif /* FILETABLE_HPP */
