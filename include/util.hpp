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
#include <string>
#include <sstream>
#include <array>

#include "headers.hpp"

constexpr auto uint16_t_max = std::numeric_limits<uint16_t>::max();
constexpr auto uint32_t_max = std::numeric_limits<uint32_t>::max();

#define PREFIX_MASK(len) ((uint32_t) (~((((uint64_t) 1) << (32-len)) -1)))

/* // Seems to be defined by cameron
#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif
*/

inline std::string ip_to_str(uint32_t ip){
	struct in_addr in_addr;
	in_addr.s_addr = htonl(ip);
	std::string str(inet_ntoa(in_addr));
	return str;
}

inline std::string mac_to_str(std::array<uint8_t, 6> mac){
	/*
	std::stringstream sstream;
	sstream << std::hex << mac[0];
	for(int i=1; i<6; i++){
		sstream << ":" << std::hex << mac[i];
	}
	return sstream.str();
	*/
	std::array<char, 18> mac_cstr;
	sprintf(mac_cstr.data(), "%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0], mac[1], mac[2],
		mac[3], mac[4], mac[5]);
	return std::string(mac_cstr.data());
}

inline std::string int2str(int i){
	std::stringstream sstream;
	sstream << i;
	return sstream.str();
}
/* // Same as uint32_t
inline std::string int2str(unsigned int i){
	std::stringstream sstream;
	sstream << i;
	return sstream.str();
}
*/
inline std::string int2str(uint16_t i){
	std::stringstream sstream;
	sstream << i;
	return sstream.str();
}
inline std::string int2str(uint32_t i){
	std::stringstream sstream;
	sstream << i;
	return sstream.str();
}
inline std::string int2str(uint64_t i){
	std::stringstream sstream;
	sstream << i;
	return sstream.str();
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
	uint32_t result = 0;
	uint16_t* hdr_cast = reinterpret_cast<uint16_t*>(header);

	uint16_t orig_checksum = header->checksum;
	header->checksum = 0;
	for(uint8_t i=0; i<(header->ihl() *2); i++){
		result += ntohs(hdr_cast[i]);
		if(result & (1<<16)){
			result &= 0xffff;
			result++;
		}
	}

	header->checksum = orig_checksum;
	return htons(~result);
};

template<typename T>
inline T rotl(T in, int s){
	return (in << s) | (in >> (std::numeric_limits<T>::digits - s));
};

inline void logInfo(std::string str){
	std::cout << str << std::endl;
};

inline void logDebug(std::string str){
#ifdef DEBUG
	std::cout << str << std::endl;
#else
	(void) str;
#endif
};

inline void logErr(std::string str){
	std::cerr << str << std::endl;
};

inline void fatal(std::string str){
	std::cerr << str << std::endl;
	std::abort();
};

#endif /* UTIL_HPP */
