#ifndef LPM_HPP
#define LPM_HPP

#include <string>
#include <exception>

#include "table.hpp"

class LPM {
protected:
	Table& table;
	bool texOutput;
public:
	LPM(Table& table, bool texOutput = false) : table(table), texOutput(texOutput) {};

	virtual uint32_t route(uint32_t) { throw std::logic_error("Not Implemented"); };
	virtual std::string getTex() { throw std::logic_error("Not Implemented"); };
};

#endif /* LPM_HPP */
