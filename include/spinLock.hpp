#ifndef SPINLOCK_HPP
#define SPINLOCK_HPP

// Adapted from:
// http://en.cppreference.com/w/cpp/atomic/atomic_flag
#include <atomic>

/*! Atomic Splin Lock class
 * A spinlock basec on std::atomic_flag
 */
class SpinLock {
private:
	std::atomic_flag lock_var = ATOMIC_FLAG_INIT;

public:
	/*! Initialize Spinlock
	 */
	SpinLock() {};

	/*! Lock the SpinLock
	 */
	void lock(){
		while (lock_var.test_and_set(std::memory_order_acquire)) // acquire lock
			; // spin
	};

	/*! Relase the Spinlock
	 */
	void unlock(){
		lock_var.clear(std::memory_order_release); // release lock
	};
};

#endif /* SPINLOCK_HPP */
