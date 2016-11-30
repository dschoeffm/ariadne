#ifndef HEADERS_HPP
#define HEADERS_HPP

#include "stdint.h"

struct ether {
	uint8_t destination_mac[6];
	uint8_t source_mac[6];
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
	uint32_t source_ip; //!< source IPv4 address
	uint32_t destination_ip; //!< destination IPv4 address
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
	uint8_t sender_hw_addr[6]; //!< Sender hardware address
	uint32_t sender_proto_addr; //!< Sender protocol address
	uint8_t target_hw_addr[6]; //!< Target hardware address
	uint32_t target_proto_addr; //!< Target protocol address
} __attribute__((packed));

#endif /* HEADERS_HPP */
