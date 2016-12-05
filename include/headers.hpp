#ifndef HEADERS_HPP
#define HEADERS_HPP

#include "stdint.h"
#include "array"

struct ether {
	std::array<uint8_t, 6> d_mac;
	std::array<uint8_t, 6> s_mac;
	uint16_t ethertype;
} __attribute__((packed));

/*! Representation of the IPv4 header.
 */
struct ipv4 {
	uint8_t version_ihl; //!< Version and IHL
#define IPv4_VERSION(x) ((x->version_ihl & 0xf0) >> 4)
#define IPv4_IHL(x) (x->version_ihl & 0x0f)
	uint8_t tos; //!< Type of Service
	uint16_t total_length; //!< L3-PDU length
	uint16_t id; //!< Identification
	uint16_t flags_fragmentation; //!< flags and fragmentation offset
#define IPv4_FRAGMENTATION(x) (x->flags_fragmentation & 0x1fff)
#define IPv4_FLAGS(x) (x->flags_fragmentation >> 18)
	uint8_t ttl; //!< Time to live
	uint8_t proto; //!< next protocol
	uint16_t checksum; //!< header checksum
	uint32_t s_ip; //!< source IPv4 address
	uint32_t d_ip; //!< destination IPv4 address
} __attribute__((packed));

/*! Representation of the IPv4 header.
 */
struct arp {
	uint16_t hw_type; //!< Hardware type (0x0001)
	uint16_t proto_type; //!< Protocol type (0x0800)
	uint8_t hw_len; //!< Length of hardware address (6)
	uint8_t proto_len; //!< Length of protocol address (4)
	uint8_t op; //! Operation
#define ARP_OP_REQUEST 0x0001
#define ARP_OP_REPLY 0x0002
	std::array<uint8_t, 6> s_hw_addr; //!< Sender hardware address
	uint32_t s_proto_addr; //!< Sender protocol address
	std::array<uint8_t, 6> t_hw_addr; //!< Target hardware address
	uint32_t t_proto_addr; //!< Target protocol address
} __attribute__((packed));

#endif /* HEADERS_HPP */
