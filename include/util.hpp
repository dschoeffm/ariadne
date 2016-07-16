#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

// avoids function not used warnings when using static
inline string ip_to_str(uint32_t ip){
	struct in_addr in_addr;
	in_addr.s_addr = htonl(ip);
	string str(inet_ntoa(in_addr));
	return str;
}

#endif /*__UTIL_HPP__*/
