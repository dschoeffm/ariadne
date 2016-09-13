#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PREFIX_MASK(len) ((uint32_t) (~((((uint64_t) 1) << (32-len)) -1)))

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

inline std::string ip_to_str(uint32_t ip){
	struct in_addr in_addr;
	in_addr.s_addr = htonl(ip);
	std::string str(inet_ntoa(in_addr));
	return str;
}

inline uint32_t extractBit(uint32_t addr, int pos) {
	uint32_t mask = ((uint32_t) 1) << (31-pos);
	uint32_t bit = addr & mask;
	return (bit >> (31-pos));
};

inline int mostSigOne(uint32_t num){
	for(int i=31; i>=0; i--){
		uint32_t test = 1 << i;
		if(num & test){
			return 31-i;
		}
	}

	return -1;
}

#endif /* UTIL_HPP */
