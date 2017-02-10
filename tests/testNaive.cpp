#include "sampleLPMtest.hpp"
#include "naive.hpp"

int main(int argc, char** argv){
	(void) argc;
	(void) argv;

	testLPM<Naive>();
};
