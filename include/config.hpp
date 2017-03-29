#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "basicTrie.hpp"
#include "naive.hpp"

// Which LPM implmentation to use
using LPM=BasicTrie;

#define MANAGER_BULK_SIZE 512
#define WORKER_BULK_SIZE 512

// Fixed ring size
constexpr auto RING_SIZE = 1024;

static struct GlobalConfig_t {
	unsigned int numWorkers = 1;
} GlobalConfig; // defined in main.cpp

#endif /* CONFIG_HPP */
