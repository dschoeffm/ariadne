#ifndef FRAME_HPP
#define FRAME_HPP

#include <stdint.h>

/*! One L2 Frame.
 * This datastructure represents one ethernet frame.
 */
struct frame {
	uint8_t* buf_ptr; //!< pointer to the buffer which holds the frame (must be at least MTU+14 sized)
	uint16_t len; //!< length of the L2-PDU
	uint16_t iface; //!< interface responsible for this frame
};

#endif /* FRAME_HPP */
