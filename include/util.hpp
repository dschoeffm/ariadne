#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


inline std::string ip_to_str(uint32_t ip){
	struct in_addr in_addr;
	in_addr.s_addr = htonl(ip);
	std::string str(inet_ntoa(in_addr));
	return str;
}

#endif /* UTIL_HPP */
