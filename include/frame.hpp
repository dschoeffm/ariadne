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
#define IFACE_HOST std::numeric_limits<uint16_t>::max() -1;
#define IFACE_DISCARD std::numeric_limits<uint16_t>::max() -2;
#define IFACE_ARP std::numeric_limits<uint16_t>::max() -3;
	uint16_t vlan; //!< VLAN tag (unused as of now)
	uint16_t hash(); //!< Hash value of this frame
};

#endif /* FRAME_HPP */
