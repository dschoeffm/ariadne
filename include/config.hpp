#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "basicTrie.hpp"
#include "naive.hpp"

// Which LPM implmentation to use
using LPM=Naive;

// Fixed ring size
constexpr auto RING_SIZE = 1024;

// Maximum time to wait for ARP
constexpr auto ARP_WAIT_MILLIS = 1000;

// Maximum backlog in a worker
constexpr auto WORKER_MAX_BACKLOG = 256;

#endif /* FRAME_HPP */
