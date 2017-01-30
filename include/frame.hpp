#ifndef FRAME_HPP
#define FRAME_HPP

#include <stdint.h>
#include "util.hpp"
#include "headers.hpp"

/*! One L2 Frame.
 * This datastructure represents one ethernet frame.
 */
struct frame {
	uint8_t* buf_ptr; //!< pointer to the buffer which holds the frame (must be at least MTU+14 sized)
	uint16_t len; //!< length of the L2-PDU
	uint16_t iface; //!< interface responsible for this frame
	/*
#define IFACE_ARP 0x2000
#define IFACE_HOST 0x4000
#define IFACE_DISCARD 0x8000
	*/
	static constexpr uint16_t IFACE_NOMAC = 0x1000;
	static constexpr uint16_t IFACE_ARP = 0x2000;
	static constexpr uint16_t IFACE_HOST = 0x4000;
	static constexpr uint16_t IFACE_DISCARD = 0x8000;
	static constexpr uint16_t IFACE_ID = 0x0fff;

	uint16_t vlan; //!< VLAN tag (unused as of now)
	uint16_t hash(); //!< Hash value of this frame

	frame() : buf_ptr(NULL), len(0), iface(0xffff), vlan(0) {};

	frame(uint8_t* buf_ptr, uint16_t len, uint16_t iface, uint16_t vlan = 0)
		: buf_ptr(buf_ptr), len(len), iface(iface), vlan(vlan) {};

	frame(char* buf_ptr, uint16_t len, uint16_t iface, uint16_t vlan = 0)
		: buf_ptr(reinterpret_cast<uint8_t*>(buf_ptr)), len(len), iface(iface), vlan(vlan) {};

	frame(const frame& f)
		: buf_ptr(f.buf_ptr), len(f.len), iface(f.iface), vlan(f.vlan) {};
};

#endif /* FRAME_HPP */
