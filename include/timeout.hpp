#ifndef TIMEOUT_HPP
#define TIMEOUT_HPP

#include <iostream>
#include <ctime>
#include <ratio>
#include <chrono>
#include <vector>
#include <algorithm>
#include <mutex>
#include <thread>
#include <condition_variable>

/*! Time based event trigger.
 * This class triggers given functions at given points in time.
 */
class Timeout {
public:
	struct event {
		std::function<void()> func;
		std::chrono::time_point<std::chrono::steady_clock> time;
		bool operator>(const event& e) const{
			if(time < e.time){
				return true;
			} else {
				return false;
			}
		}
	};

private:
	std::vector<event> events;
	std::mutex mutex;
	std::thread thread;
	std::condition_variable cv;
	bool terminate = false;

	void waitForNextEvent(){
		while(true){
			std::unique_lock<std::mutex> lock(mutex);
			while (events.empty()){
				cv.wait(lock);
				if(terminate){
					return;
				}
			}

			if(events.front().time <= std::chrono::steady_clock::now()){
				events.front().func();
				std::pop_heap(events.begin(), events.end());
				events.pop_back();
			}
		}
	};

public:
	/*! Construct a new Timeout object
	 */
	Timeout() : thread(&Timeout::waitForNextEvent, this) {};

	~Timeout(){
		std::unique_lock<std::mutex> lock(mutex);
		terminate = true;
		cv.notify_all(); // Should only be one
	}

	/*! Add a new event to the Timeout
	 */
	void addEvent(event e){
		std::unique_lock<std::mutex> lock(mutex);
		events.push_back(e);
		std::push_heap(events.begin(), events.end());
		cv.notify_all(); // Should only be one
	}
};

#endif /* TIMEOUT_HPP */
