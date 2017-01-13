#ifndef WORKER_HPP
#define WORKER_HPP

#include <memory>
#include <thread>
#include <atomic>
#include <queue>
#include <stdint.h>
#include <string.h>
#include <ctime>
#include <ratio>
#include <chrono>

class Worker;

#include "config.hpp"

#include "readerwriterqueue.h"
#include "basicTrie.hpp"
#include "routingTable.hpp"
#include "arpTable.hpp"
#include "headers.hpp"
#include "util.hpp"
#include "manager.hpp"

using namespace std;

/*! The packet processing is done here.
 * This class gets packets from an input ring,
 * applies some transformation (routing),
 * and outputs them through the output rings.
 */
class Worker {
private:
	// Shared data structures and their updates
	std::shared_ptr<LPM> cur_lpm;
	std::shared_ptr<ARPTable::table> cur_arp_table;
	std::shared_ptr<LPM> new_lpm;
	std::shared_ptr<ARPTable::table> new_arp_table;

	// Rings for frame transfer
	std::shared_ptr<moodycamel::BlockingReaderWriterQueue<frame>> ingressQ;
	std::shared_ptr<moodycamel::BlockingReaderWriterQueue<frame>> egressQ;

	// Interfaces to be used
	std::shared_ptr<std::vector<interface>> interfaces;

	// Manager responsible for this worker
	Manager& manager;

	// ARP table, used to enqueue MAC requests
	ARPTable& arpTable;

	// The thread this one worker works in
	std::thread thread;

	// The state of the thread
	enum enum_state {RUN, STOP};
	std::atomic<enum_state> state;

	// Big stuff happens here
	void process();

	// Run loop
	void run(){
		while(state.load() == RUN){
			process();
			cur_lpm = new_lpm;
			cur_arp_table = new_arp_table;
		}
	}

public:
	/*! Start a new Worker thread.
	 * All persistend arguments are given here, as well as the first set of updatables
	 * \param cur_lpm Current LPM datastructre
	 * \param cur_arp_table the Current ARP lookup table
	 * \param ingressQ Input ring of new packets
	 * \param egressQ Output ring of processed packets
	 * \param interfaces Interfaces of the router
	 * \param manager Manager responsible for this worker
	 */
	Worker(
		std::shared_ptr<LPM> cur_lpm,
		std::shared_ptr<ARPTable::table> cur_arp_table,
		std::shared_ptr<moodycamel::BlockingReaderWriterQueue<frame>> ingressQ,
		std::shared_ptr<moodycamel::BlockingReaderWriterQueue<frame>> egressQ,
		std::shared_ptr<std::vector<interface>> interfaces,
		Manager& manager,
		ARPTable& arpTable)
		: cur_lpm(cur_lpm), cur_arp_table(cur_arp_table), ingressQ(ingressQ), egressQ(egressQ),
		interfaces(interfaces), manager(manager), arpTable(arpTable),
		thread(&Worker::run, this), state(RUN) {};

	/*! Update Worker.
	 * This function updates the worker thread with new information
	 * \param lpm New LPM datastructure
	 * \param arp_table New ARP lookup table, corresponrint to lpm
	 */
	void update(std::shared_ptr<LPM> lpm, std::shared_ptr<ARPTable::table> arp_table){
		new_lpm = lpm;
		new_arp_table = arp_table;
	};

	/*! Stop this worker thread.
	 * This function blocks until the worker is actually stopped (joined)
	 */
	void stop(){
		state.store(STOP);
		thread.join();
	}
};

#endif /* WORKER_HPP */
