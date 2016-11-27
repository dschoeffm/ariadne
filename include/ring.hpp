#ifndef RING_HPP
#define RING_HPP

#include <vector>
#include "string.h"
#include "spinLock.hpp"

#include "config.hpp"

/*! Concurrent ring.
 * This is a ring, which is garanteed to be MT-safe
 * \param T datatype to built the ring from
 */
template <class T>
class Ring {
private:
	std::vector<T> vec;
	unsigned int head;
	unsigned int tail;
	SpinLock lock;

public:
	/*! Initialize a new ring
	 */
	Ring() : vec(RING_SIZE), head(0), tail(0), lock() {};

	/*! Push new elements into the ring.
	 * If the ring does not have enough space to hold all new elements,
	 * the last n elements of the input will be omitted
	 * \param in vector of elements to be pushed
	 * \return number of elements which were actually pushed
	 */
	unsigned int push(std::vector<T>& in){
		lock.lock();

		unsigned int count = in.size();
		if(count > (RING_SIZE - size())){
			count = size();
		}

		unsigned int first_chunk = (head + count) % RING_SIZE;
		unsigned int second_chunk = count - first_chunk;

		memcpy(vec.data() + head, in.data(), first_chunk * sizeof(T));
		memcpy(vec.data(), in.data() + first_chunk, second_chunk * sizeof(T));
		head += count;
		head %= RING_SIZE;

		lock.release();
		return count;
	};

	/*! Pop elements from the ring.
	 * Retrieve elements from the ring.
	 * All data inside "out" will be erased.
	 * \param out vector to put the elements into
	 * \param count number of elements to pop from the ring
	 * \return actual number of elements which were poped from the ring
	 */
	unsigned int pop(std::vector<T>& out, unsigned int count){
		lock.lock();
		if(count > size()){
			count = size();
		}
		out.resize(count);

		unsigned int first_chunk = (tail + count) % RING_SIZE;
		unsigned int second_chunk = count - first_chunk;

		memcpy(out.data(), vec.data() + tail, first_chunk * sizeof(T));
		memcpy(out.data() + first_chunk, vec.data(), second_chunk * sizeof(T));
		tail += count;
		tail %= RING_SIZE;

		lock.release();
		return count;
	}

	/*! Get the number of elements in the ring.
	 * This function may return a wrong result, in case another thread pushes or popes.
	 * \return number of elements
	 */
	unsigned int size(){
		return (head - tail + RING_SIZE) % RING_SIZE;
	};

	/*! Returns if the Ring is currently empty.
	 * \return state of the ring
	 */
	bool empty(){
		if(size())
			return false;
		return true;
	};
};

#endif /* RING_HPP */
