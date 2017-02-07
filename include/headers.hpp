#ifndef HEADERS_HPP
#define HEADERS_HPP

#include "stdint.h"
#include "array"

namespace headers {

/*! Representation of the etherner header.
 */
struct ether {
	std::array<uint8_t, 6> d_mac; //!< Destination MAC
	std::array<uint8_t, 6> s_mac; //!< Source MAC
	uint16_t ethertype; //!< Ethertype
} __attribute__((packed));

/*! Representation of the IPv4 header.
 */
struct ipv4 {
	uint8_t version_ihl; //!< Version and IHL
/*
#define IPv4_VERSION(x) ((x->version_ihl & 0xf0) >> 4)
#define IPv4_IHL(x) (x->version_ihl & 0x0f)
*/
	uint8_t version() const {
		return (version_ihl & 0xf0) >> 4;
	}
	uint8_t ihl() const {
		return version_ihl & 0x0f;
	}

	uint8_t tos; //!< Type of Service
	uint16_t total_length; //!< L3-PDU length
	uint16_t id; //!< Identification
	uint16_t flags_fragmentation; //!< flags and fragmentation offset
/*
#define IPv4_FRAGMENTATION(x) (x->flags_fragmentation & 0x1fff)
#define IPv4_FLAGS(x) (x->flags_fragmentation >> 18)
*/
	uint16_t fragmentation() const {
		return flags_fragmentation & 0x1fff;
	}
	uint16_t flags() const {
		return flags_fragmentation >> 18;
	}

	uint8_t ttl; //!< Time to live
	uint8_t proto; //!< next protocol
/*
#define IPv4_PROTO_ICMP 1
#define IPv4_PROTO_TCP 6
#define IPv4_PROTO_UDP 17
*/
	static constexpr uint8_t PROTO_ICMP = 1;
	static constexpr uint8_t PROTO_TCP = 6;
	static constexpr uint8_t PROTO_UDP = 17;

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
	uint16_t op; //!< Operation
/*
#define ARP_OP_REQUEST 0x0001
#define ARP_OP_REPLY 0x0002
*/
	static constexpr uint8_t OP_REQUEST = 0x01;
	static constexpr uint8_t OP_REPLY = 0x02;

	std::array<uint8_t, 6> s_hw_addr; //!< Sender hardware address
	uint32_t s_proto_addr; //!< Sender protocol address
	std::array<uint8_t, 6> t_hw_addr; //!< Target hardware address
	uint32_t t_proto_addr; //!< Target protocol address
} __attribute__((packed));

/*! Representation of the IPv4 header.
 */
struct tcp {
	uint16_t s_port; //!< Source port
	uint16_t d_port; //!< Destination port
	uint32_t seq; //!< Sequence number
	uint32_t ack; //!< Acknowledgement number
	uint16_t offset_flags; //!< Data offset and flags
	uint16_t window; //!< Receive window
	uint16_t checksum; //!< Checksum
	uint16_t urgend_ptr; //!< Urgent pointer
} __attribute__((packed));

/*! Representation of the UDP header.
 */
struct udp {
	uint16_t s_port; //!< Source port
	uint16_t d_port; //!< Destination port
	uint16_t len; //!< Length
	uint16_t checksum; //!< Checksum
} __attribute__((packed));

/*! Representation of the ICMP header
 */
struct icmp {
	uint8_t type; //!< Type
	uint8_t code; //!< Code
	uint16_t checksum; //!< Checksum
} __attribute__((packed));

}

#endif /* HEADERS_HPP */
