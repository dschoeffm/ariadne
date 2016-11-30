#ifndef WORKER_HPP
#define WORKER_HPP

#include <memory>
#include <thread>
#include <atomic>
#include <queue>
#include <stdint.h>
#include <string.h>
#include "basicTrie.hpp"
#include "ring.hpp"
#include "routingTable.hpp"
#include "arpTable.hpp"
#include "headers.hpp"

#include "config.hpp"

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
	Ring<frame>& ingressQ;
	Ring<frame>& egressQ;
	Ring<frame>& hostQ;

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
	 * \param hostQ Ring to the kernel
	 */
	Worker(
		std::shared_ptr<LPM> cur_lpm,
		std::shared_ptr<ARPTable::table> cur_arp_table,
		Ring<frame>& ingressQ,
		Ring<frame>& egressQ,
		Ring<frame>& hostQ)
		: cur_lpm(cur_lpm), cur_arp_table(cur_arp_table), ingressQ(ingressQ), egressQ(egressQ),
		hostQ(hostQ), thread(&Worker::run, this), state(RUN) {};

	/*! Update Worker
	 * This function updates the worker thread with new information
	 * \param lpm New LPM datastructure
	 * \param arp_table New ARP lookup table, corresponrint to lpm
	 */
	void update(std::shared_ptr<LPM> lpm, std::shared_ptr<ARPTable::table> arp_table){
		new_lpm = lpm;
		new_arp_table = arp_table;
	};

	/*! Stop this worker thread
	 * This function blocks until the worker is actually stopped (joined)
	 */
	void stop(){
		state.store(STOP);
		thread.join();
	}
};

#endif /* WORKER_HPP */
