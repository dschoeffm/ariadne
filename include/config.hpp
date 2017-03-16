#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "basicTrie.hpp"
#include "naive.hpp"

// Which LPM implmentation to use
using LPM=BasicTrie;

// Fixed ring size
constexpr auto RING_SIZE = 1024;

// Maximum time to wait for ARP
constexpr auto ARP_WAIT_MILLIS = 1000;

// Maximum backlog in a worker
constexpr auto WORKER_MAX_BACKLOG = 256;

static struct GlobalConfig_t {
	unsigned int numWorkers = 1;
} GlobalConfig; // defined in main.cpp

#endif /* CONFIG_HPP */
