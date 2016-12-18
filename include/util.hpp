#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <algorithm>
#include <stdio.h>
#include <iostream>
#include <cstdlib>

#include "headers.hpp"

constexpr auto uint16_t_max = std::numeric_limits<uint16_t>::max();
constexpr auto uint32_t_max = std::numeric_limits<uint32_t>::max();

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

inline void parseMac(const char* str, uint8_t mac[6]){
	unsigned int copy[6] = {0};
	sscanf(str, "%x:%x:%x:%x:%x:%x",
			&copy[0], &copy[1], &copy[2], &copy[3], &copy[4], &copy[5]);
	mac[0] = copy[0]; mac[1] = copy[1]; mac[2] = copy[2];
	mac[3] = copy[3]; mac[4] = copy[4]; mac[5] = copy[5];
}

inline uint16_t IPv4HdrChecksum(headers::ipv4* header){
	uint16_t result = 0;
	uint16_t* hdr_cast = reinterpret_cast<uint16_t*>(header);

	uint16_t orig_checksum = header->checksum;
	header->checksum = 0;
	for(uint8_t i=0; i<(IPv4_IHL(header)*2); i++){
		result += ~(hdr_cast[i]);
	}

	header->checksum = orig_checksum;
	return (~result);
};

template<typename T>
inline T rotl(T in, int s){
	return (in << s) | (in >> (std::numeric_limits<T>::digits - s));
};

inline void logInfo(std::string str){
	std::cout << str << std::endl;
};

inline void logErr(std::string str){
	std::cerr << str << std::endl;
};

inline void fatal(std::string str){
	std::cerr << str << std::endl;
	std::exit(1);
};

#endif /* UTIL_HPP */
