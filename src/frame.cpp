#include "frame.hpp"

#define MAGIC_1 3
#define MAGIC_2 7
#define MAGIC_3 12

using namespace headers;

static uint16_t ipv4_hash(uint8_t* addr);
static uint16_t tcp_hash(uint8_t* addr);
static uint16_t udp_hash(uint8_t* addr);
static uint16_t icmp_hash(uint8_t* addr);


static uint16_t ipv4_hash(uint8_t* addr){
	ipv4* ipv4 = reinterpret_cast<struct ipv4*>(addr);
	uint32_t res = 0;
	res |= rotl(ipv4->d_ip, MAGIC_1);
	res	|= rotl(ipv4->s_ip, MAGIC_2);
	res |= rotl(ipv4->proto, MAGIC_3);

	switch(ipv4->proto){
		case IPv4_PROTO_ICMP:
			res |= icmp_hash(addr + IPv4_IHL(ipv4)*4);
			break;
		case IPv4_PROTO_TCP:
			res |= tcp_hash(addr + IPv4_IHL(ipv4)*4);
		break;
		case IPv4_PROTO_UDP:
			res |= udp_hash(addr + IPv4_IHL(ipv4)*4);
		break;
	}

	return (res & 0x0000ffff) | (res & 0xffff0000);
};

static uint16_t tcp_hash(uint8_t* addr){
	tcp* tcp = reinterpret_cast<struct tcp*>(addr);
	return rotl(tcp->d_port, MAGIC_1) | rotl(tcp->s_port, MAGIC_2);
};

static uint16_t udp_hash(uint8_t* addr){
	udp* udp = reinterpret_cast<struct udp*>(addr);
	return rotl(udp->d_port, MAGIC_1) | rotl(udp->s_port, MAGIC_2);
};

static uint16_t icmp_hash(uint8_t* addr){
	icmp* icmp = reinterpret_cast<struct icmp*>(addr);
	return (icmp->code << 8) | icmp->type;
};

uint16_t frame::hash(){
	ether* ether = reinterpret_cast<struct ether*>(this->buf_ptr);
	uint16_t result = ether->ethertype;
	if(ether->ethertype == htons(0x0800)){
		result |= ipv4_hash(this->buf_ptr);
	}
	return result;
}
